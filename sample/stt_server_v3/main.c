/*
 * Copyright (c) Katsuya Owari
 */

/*
 * stt_server_v3
 *
 * ブラウザからWebSocket で 16kHz / mono / int16 PCM を逐次受信し、
 * Silero VAD (whisper.cpp 組み込み) で発話区間を切り出して、
 * 発話が終わるたびにその区間だけを whisper_full() で文字起こしするサンプル
 * 
 * スレッド構成:
 *  main thread: api_qs_update() によるネットワーク処理。PCMを接続ごとの inbox に積むだけ。
 *               worker が作った送信メッセージを取り出して WebSocket で返す。
 *  worker thread: inbox の PCM を取り出して VAD -> 発話終了で whisper 推論。
 *                 whisper / VAD のコンテキストはスレッドだけが触る。
 * 
 * WebSocket プロトコル:
 *  client -> server  text  {"type":"stt_start","sample_rate":16000,"channels":1,"bits_per_sample":16}
 *  client -> server  binary int 16 little endian PCM
 *  client -> server  text  {"type":"stt_stop"}
 *  server -> client  binary UTF-8 JSON
 *                    {"type":"ready"} / {"type":"speech_start","t0":..}
 *                    {"type":"speech_end","t0":..,"t1":..,"reason":"silence|max_length|stop"}
 *                    {"type":"result","t0":..,"t1":..,"text":"..","infer_ms":..}
 *                    {"type":"stopped"} / {"type":"error","message":".."}
 *   (libqsの接続オフセット指定送信 API が binary のみのため、JSON も binary フレームで返す)
 * 
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "qs_api.h"
#include "whisper.h"

#define SVS_SAMPLE_RATE 16000
#define SVS_CONNECTION_MAX 32
#define SVS_MS_TO_SAMPLES(ms) ((int32_t)((int64_t)(ms) * SVS_SAMPLE_RATE / 1000))
#define SVS_SAMPLES_TO_MS(n)  ((int64_t)(n) * 1000 / SVS_SAMPLE_RATE)

/* Silero VAD は 512 サンプル(32ms) 単位で確率を出す*/
#define SVS_VAD_FRAME 512

/* whisper_vad_detect_speech() は呼び出しごとに LSTM 状態をリセットするため、
 * 直前の音声を一緒に渡して状態を立ち上げなおす(32 frame = 約１秒)
 */
#define SVS_VAD_HISTORY_SAMPLES (SVS_VAD_FRAME * 32)

/* 受信済み・未処理 PCM の上限 (worker が推論中に溜まる分)*/
#define SVS_INBOX_SAMPLES (SVS_SAMPLE_RATE * 30)

/* 1 発話の最大長。超えたら強制的に区切って推論する*/
#define SVS_UTTER_MAX_SAMPLES (SVS_SAMPLE_RATE * 20)
#define SVS_OUT_QUEUE_MAX 1024

enum {
   SVS_SLOT_FREE = 0,
   SVS_SLOT_ACTIVE,
   SVS_SLOT_CLOSING,
};

typedef struct SVS_CONFIG_STRUCT
{
    int server_port;
    int scheduler_mode;
    int32_t max_connection;
    char whisper_model[512];
    char vad_model[512];
    char language[16];
    int use_gpu;
    int n_threads;
    float vad_threshold; /* 発話開始とみなす確率 */
    int silence_end_ms;  /* この長さ無音が続いたら発話終了 */
    int min_speech_ms;   /* これより短い発話は捨てる(クリック音対策) */
    int pre_roll_ms;     /* 発話開始前に遡って含める長さ(語頭欠け対策) */
    int post_pad_ms;     /* 発話終了後に残す無音の長さ */
    float no_speech_thold;
} SVS_CONFIG;

/* main thread と sorker thread で共有する接続スロット(g_lock で保護)*/
typedef struct SVS_SLOT_STRUCT
{
    int state;
    uint32_t generation;
    uint32_t connection_offset;
    int is_streaming;
    int stop_requested;
    int16_t* inbox;
    int32_t inbox_len;
    int inbox_overflow_logged;
} SVS_SLOT;

/* worker thread だけが触る発話検出の状態 */
typedef struct SVS_WORKER_STATE_STRUCT
{
    uint32_t generation;
    float* pending;
    int32_t pending_len;
    float* history;
    int32_t history_len;
    float* vad_input;
    float* utter;
    int32_t utter_len;
    int in_speech;
    int32_t speech_samples;
    int32_t silence_samples;
    uint64_t stream_pos;
    uint64_t utter_start;
} SVS_WORKER_STATE;

typedef struct SVS_OUT_MSG_STRUCT
{
    int slot_index;
    uint32_t generation;
    char* data;
    size_t size;
} SVS_OUT_MSG;

int on_connect(QS_EVENT_PARAMETER params);
int on_http_event(QS_EVENT_PARAMETER params);
int on_ws_event(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

QS_MEMORY_CONTEXT g_temporary_memory;

static SVS_CONFIG g_config;
static SVS_SLOT g_slots[SVS_CONNECTION_MAX];
static SVS_WORKER_STATE g_workers[SVS_CONNECTION_MAX];
static SVS_OUT_MSG g_out_queue[SVS_OUT_QUEUE_MAX];
static int32_t g_out_head = 0;
static int32_t g_out_count = 0;
static uint32_t g_generation_counter = 0;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t g_running = 1;

static struct whisper_context* g_whisper_ctx = NULL;
static struct whisper_vad_context* g_vad_ctx = NULL;

/* -------------------------------------------------- */
/* utility                                            */
/* -------------------------------------------------- */

static int64_t svs_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000LL + (int64_t)(ts.tv_nsec / 1000000LL);
}

static void svs_on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

/* whisper / ggml の INFO ログは VAD 呼び出しごとに大量に出るので WARN 以上だけ表示する */
static void svs_whisper_log(enum ggml_log_level level, const char* text, void* user_data)
{
    static enum ggml_log_level last_level = GGML_LOG_LEVEL_NONE;
    (void)user_data;
    if(level != GGML_LOG_LEVEL_CONT) 
    {
        last_level = level;
    }
    if(last_level >= GGML_LOG_LEVEL_WARN)
    {
        fputs(text, stderr);
    }
}

static void svs_json_escape(const char* src, char* dst, size_t dst_size)
{
    size_t pos = 0;
    if(dst_size == 0){
        return;
    }
    for(; src && *src != '\0' && pos + 7 < dst_size; src++)
    {
        unsigned char c = (unsigned char)*src;
        if(c == '"' || c == '\\'){
            dst[pos++] = '\\';
            dst[pos++] = (char)c;
        } else if (c == '\n'){
            dst[pos++] = '\\';
            dst[pos++] = 'n';
        } else if (c == '\r'){
            dst[pos++] = '\\';
            dst[pos++] = 'r';
        } else if (c == '\t'){
            dst[pos++] = '\\';
            dst[pos++] = 't';
        } else if (c < 0x20){
            pos += (size_t)snprintf(dst + pos, dst_size - pos, "\\u%04x", c);
        } else {
            dst[pos++] = (char)c;
        }
    }
    dst[pos] = '\0';
}

/* 送信キューにJSONを積む。どのスレッドからでも呼べる */
static void svs_post_message(int slot_index, uint32_t generation, const char* fmt, ...)
{
    char buf[8192];
    va_list ap;
    int len;
    char* data;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if(len <= 0)
    {
        return;
    }
    if((size_t)len >= sizeof(buf))
    {
        len = (int)sizeof(buf) - 1;
    }
    data = (char*)malloc((size_t)len);
    if(!data){
        return;
    }
    memcpy(data, buf, (size_t)len);

    pthread_mutex_lock(&g_lock);
    if(g_out_count >= SVS_OUT_QUEUE_MAX){
        pthread_mutex_unlock(&g_lock);
        printf("[SVS][queur] out queue full, drop message slot=%d\n", slot_index);
        free(data);
        return;
    }
    {
        SVS_OUT_MSG* msg = &g_out_queue[(g_out_head + g_out_count) % SVS_OUT_QUEUE_MAX];
        msg->slot_index = slot_index;
        msg->generation = generation;
        msg->data = data;
        msg->size = (size_t)len;
        g_out_count++;
    }
    pthread_mutex_unlock(&g_lock);
}

/* main thread: 送信キューを WebSocket に流す */
static void svs_flush_out_queue(QS_SERVER_CONTEXT* context)
{
    for(;;){
        SVS_OUT_MSG msg;
        int deliver = 0;
        uint32_t connection_offset = 0;

        pthread_mutex_lock(&g_lock);
        if(g_out_count == 0){
            pthread_mutex_unlock(&g_lock);
            break;
        }
        msg = g_out_queue[g_out_head];
        g_out_head = (g_out_head + 1) % SVS_OUT_QUEUE_MAX;
        g_out_count--;
        if(msg.slot_index >= 0 && msg.slot_index < SVS_CONNECTION_MAX){
            SVS_SLOT* slot = &g_slots[msg.slot_index];
            if(slot->state == SVS_SLOT_ACTIVE && slot->generation == msg.generation){
                deliver = 1;
                connection_offset = slot->connection_offset;
            }
        }
        pthread_mutex_unlock(&g_lock);

        if(deliver){
            api_qs_send_ws_binary_by_connection_offset(context, connection_offset, msg.data, msg.size);
        }
        free(msg.data);
    }
}

/* -------------------------------------------------- */
/* config                                            */
/* -------------------------------------------------- */

static void svs_config_default(SVS_CONFIG* config)
{
    memset(config, 0, sizeof(*config));
    config->server_port = 8089;
    config->scheduler_mode = QS_SCHEDULER_MODE_MIDDLE;
    config->max_connection = 100;
    snprintf(config->whisper_model, sizeof(config->whisper_model), "../../stt/models/ggml-medium.bin");
    snprintf(config->vad_model, sizeof(config->vad_model), "../../stt/models/ggml-silero-v5.1.2.bin"); // 正常動作確認した環境ではv6.2.0
    snprintf(config->language, sizeof(config->language), "ja");
    config->use_gpu = 1;
    config->n_threads = 4;
    config->vad_threshold = 0.5f;
    config->silence_end_ms = 600;
    config->min_speech_ms = 250;
    config->pre_roll_ms = 300;
    config->post_pad_ms = 200;
    config->no_speech_thold = 0.6f;
}

static int svs_config_load(SVS_CONFIG* config, const char* path)
{
    QS_SERVER_SCRIPT_CONTEXT script;
    char* v;

    if(-1 == api_qs_script_read_file(&g_temporary_memory, &script, path)) {
        return -1;
    }
    if( -1 == api_qs_script_run(&script)) {
        return -1;
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "server_port"))){
        config->server_port = atoi(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "scheduler_mode"))){
        if(!strcmp(v, "high")){config->scheduler_mode = QS_SCHEDULER_MODE_HIGH;}
        else if(!strcmp(v, "middle")){config->scheduler_mode = QS_SCHEDULER_MODE_MIDDLE;}
        else {config->scheduler_mode = QS_SCHEDULER_MODE_LOW;}
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "max_connection"))){
        int n = atoi(v);
        if( n < 1) n = 1;
        if( n > 1000) n = 1000;
        config->max_connection = (int32_t)n;
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "whisper_model"))){
        snprintf(config->whisper_model, sizeof(config->whisper_model), "%s", v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "vad_model"))){
        snprintf(config->vad_model, sizeof(config->vad_model), "%s", v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "language"))){
        snprintf(config->language, sizeof(config->language), "%s", v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "use_gpu"))){
        config->use_gpu = atoi(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "n_threads"))){
        config->n_threads = atoi(v) > 0 ? atoi(v) : 4;
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "vad_threshold"))){
        config->vad_threshold = (float)atof(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "silence_end_ms"))){
        config->silence_end_ms = atoi(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "min_speech_ms"))){
        config->min_speech_ms = atoi(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "pre_roll_ms"))){
        config->pre_roll_ms = atoi(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "post_pad_ms"))){
        config->post_pad_ms = atoi(v);
    }
    if( 0 != (v = api_qs_script_get_parameter(&script, "no_speech_thold"))){
        config->no_speech_thold = (float)atof(v);
    }
    api_qs_memory_clean(&g_temporary_memory);

    /* pre-roll は VAD history から切り出すので history 長を超えられない */
    if( SVS_MS_TO_SAMPLES(config->pre_roll_ms) > SVS_VAD_HISTORY_SAMPLES){
        config->pre_roll_ms = (int)SVS_SAMPLES_TO_MS(SVS_VAD_HISTORY_SAMPLES);
    }
    return 0;
}

/* -------------------------------------------------- */
/* whisper / VAD                                      */
/* -------------------------------------------------- */
static int svs_init_models(void)
{
    struct whisper_context_params cparams;
    struct whisper_vad_context_params vparams;

    whisper_log_set(svs_whisper_log, NULL);

    cparams = whisper_context_default_params();
    cparams.use_gpu = g_config.use_gpu ? true : false;
    cparams.flash_attn = g_config.use_gpu ? true : false;
    g_whisper_ctx = whisper_init_from_file_with_params(g_config.whisper_model, cparams);
    if(!g_whisper_ctx){
        printf("[SVS][whisper] init failed: %s (gpu=%d)\n", g_config.whisper_model, g_config.use_gpu);
        return -1;
    }
    printf("[SVS][whisper] loaded: %s (gpu=%d)\n", g_config.whisper_model, g_config.use_gpu);

    /* Silero VAD は小さいモデルなので CPUで十分 */
    vparams = whisper_vad_default_context_params();
    vparams.n_threads = 1;
    vparams.use_gpu = false;
    g_vad_ctx = whisper_vad_init_from_file_with_params(g_config.vad_model, vparams);
    if(!g_vad_ctx){
        printf("[SVS][VAD] init failed: %s\n", g_config.vad_model);
        whisper_free(g_whisper_ctx);
        g_whisper_ctx = NULL;
        return -1;
    }
    printf("[SVS][VAD] loaded: %s\n", g_config.vad_model);
    return 0;
}

static void svs_free_models(void)
{
    if(g_vad_ctx){
        whisper_vad_free(g_vad_ctx);
        g_vad_ctx = NULL;
    }
    if(g_whisper_ctx){
        whisper_free(g_whisper_ctx);
        g_whisper_ctx = NULL;
    }
}

/* 無音や BGM に対して Whisper が良く出す定型文 */
static int svs_is_hallucination(const char* text)
{
    static const char* patterns[] = {
        "ご視聴ありがとうございました",
        "チャンネル登録",
        "(音楽)",
        "[音楽]",
        "(拍手)",
        "[拍手]",
        "♪",
        NULL
    };
    int i;
    for(i=0; patterns[i] != NULL; i++){
        if(strstr(text, patterns[i]) != NULL){
            return 1;
        }
    }
    return 0;
}

static void svs_transcribe(int slot_index, uint32_t generation, const float* pcm, int32_t n_samples, int64_t t0_ms, int64_t t1_ms)
{
    struct whisper_full_params wparams;
    char text[4096];
    char escaped[8000];
    float max_no_speech_prob = 0.0f;
    int64_t start_ms;
    int64_t infer_ms;
    int n_segments;
    int i;
    const char* p;

    wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.language = g_config.language;
    wparams.translate = false;
    wparams.n_threads = g_config.n_threads;
    wparams.print_progress = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.print_special = false;
    wparams.no_timestamps = true;
    wparams.no_context = true; /* 発話ごとに独立して推論する (前回の誤認識を引きずらない) */
    wparams.suppress_blank = true;
    wparams.suppress_nst = true;
    wparams.no_speech_thold = g_config.no_speech_thold;

    start_ms = svs_now_ms();
    if(0 != whisper_full(g_whisper_ctx, wparams, pcm, n_samples))
    {
        printf("[SVS][infer] whisper_full faild slot=%d samples=%d\n", slot_index, n_samples);
        svs_post_message(slot_index,generation, "{\"type\":\"error\",\"message\":\"whisper_full failed\"}");
        return;
    }
    infer_ms = svs_now_ms() - start_ms;

    text[0] = '\0';
    n_segments = whisper_full_n_segments(g_whisper_ctx);
    for(i=0; i < n_segments; i++){
        const char* seg = whisper_full_get_segment_text(g_whisper_ctx, i);
        float prob = whisper_full_get_segment_no_speech_prob(g_whisper_ctx, i);
        if(prob > max_no_speech_prob){
            max_no_speech_prob = prob;
        }
        if(seg){
            strncat(text, seg, sizeof(text) - strlen(text) - 1);
        }
    }
    p = text;
    while(*p == ' '){
        p++;
    }

    if(*p == '\0' || max_no_speech_prob >= g_config.no_speech_thold || svs_is_hallucination(p)){
        printf("[SVS][infer] slot=%d %lld-%lldms filtered (no_speech=%.2f) text=%s\n", 
            slot_index, (long long)t0_ms, (long long)t1_ms, max_no_speech_prob, *p ? p : "[empty]");
        svs_post_message(slot_index,generation,
            "{\"type\":\"result\",\"t0\":%lld,\"t1\":%lld,\"text\":\"\",\"filtered\":true,\"infer_ms\":%lld}",
            (long long)t0_ms, (long long)t1_ms, (long long)infer_ms);
        return;
    }

    printf("[SVS][infer] slot=%d %lld-%lldms infer_ms=%lld text=%s\n", slot_index, (long long)t0_ms, (long long)t1_ms, (long long)infer_ms, p);
    svs_json_escape(p, escaped, sizeof(escaped));
    svs_post_message(slot_index,generation, "{\"type\":\"result\",\"t0\":%lld,\"t1\":%lld,\"text\":\"%s\",\"infer_ms\":%lld}",
        (long long)t0_ms, (long long)t1_ms, escaped, (long long)infer_ms);
}

/* -------------------------------------------------- */
/* worker: VAD による発話区間の切り出し                 */
/* -------------------------------------------------- */

static void svs_worker_free(SVS_WORKER_STATE* w)
{
    free(w->pending);
    free(w->history);
    free(w->vad_input);
    free(w->utter);
    memset(w, 0, sizeof(*w));
}

static int svs_worker_prepare(SVS_WORKER_STATE* w)
{
    if(!w->pending){
        w->pending = (float*)malloc(sizeof(float) * (SVS_INBOX_SAMPLES + SVS_VAD_FRAME));
        w->history = (float*)malloc(sizeof(float) * SVS_VAD_HISTORY_SAMPLES);
        w->vad_input = (float*)malloc(sizeof(float) * (SVS_VAD_HISTORY_SAMPLES + SVS_INBOX_SAMPLES + SVS_VAD_FRAME));
        w->utter = (float*)malloc(sizeof(float) * SVS_UTTER_MAX_SAMPLES);
        if(!w->pending || !w->history || !w->vad_input || !w->utter){
            svs_worker_free(w);
            return -1;
        }
    }
    return 0;
}

static void svs_worker_reset(SVS_WORKER_STATE* w, uint32_t generation)
{
    w->generation = generation;
    w->pending_len = 0;
    w->history_len = 0;
    w->utter_len = 0;
    w->in_speech = 0;
    w->speech_samples = 0;
    w->silence_samples = 0;
    w->stream_pos = 0;
    w->utter_start = 0;
}

static void svs_history_push(SVS_WORKER_STATE* w, const float* frame, int32_t n)
{
    if(w->history_len + n > SVS_VAD_HISTORY_SAMPLES){
        int32_t drop = w->history_len + n - SVS_VAD_HISTORY_SAMPLES;
        memmove(w->history, w->history + drop, sizeof(float) * (size_t)(w->history_len - drop));
        w->history_len -= drop;
    }
    memcpy(w->history + w->history_len, frame, sizeof(float)* (size_t)n);
    w->history_len += n;
}

static void svs_utterance_end(int slot_index, SVS_WORKER_STATE* w, const char* reason)
{
    int32_t n = w->utter_len;
    int64_t t0_ms;
    int64_t t1_ms;

    /* 末尾の無音は post_pad_ms だけ残して削る */
    {
        int32_t pad = SVS_MS_TO_SAMPLES(g_config.post_pad_ms);
        if(w->silence_samples > pad){
            n -= (w->silence_samples - pad);
        }
    }
    t0_ms = SVS_SAMPLES_TO_MS(w->utter_start);
    t1_ms = SVS_SAMPLES_TO_MS(w->utter_start + (uint64_t)n);

    if(w->speech_samples < SVS_MS_TO_SAMPLES(g_config.min_speech_ms)){
        printf("[SVS][vad] slot=%d %lld-%lldms too short, discarded\n", slot_index, (long long)t0_ms, (long long)t1_ms);
        svs_post_message(slot_index,w->generation, "{\"type\":\"speech_end\",\"t0\":%lld,\"t1\":%lld,\"reason\":\"%s\",\"discarded\":true}",
                (long long)t0_ms, (long long)t1_ms, reason);
    } else {
        svs_post_message(slot_index,w->generation, "{\"type\":\"speech_end\",\"t0\":%lld,\"t1\":%lld,\"reason\":\"%s\"}",
                (long long)t0_ms, (long long)t1_ms, reason);
        svs_transcribe(slot_index, w->generation, w->utter, n, t0_ms, t1_ms);
    }

    w->in_speech = 0;
    w->utter_len = 0;
    w->speech_samples = 0;
    w->silence_samples = 0;
}

/* 1 frame (512 samples) ごとの発話状態遷移
 * Silero の推奨に倣い、開始は threshold 以上、終了判定は threshold - 0.15 未満でヒステリシスを付ける */
static void svs_vad_step(int slot_index, SVS_WORKER_STATE* w, const float* frame, float prob)
{
    const float on_thold = g_config.vad_threshold;
    const float off_thold = g_config.vad_threshold - 0.15f;

    if(!w->in_speech){
        if(prob >= on_thold){
            int32_t pre = SVS_MS_TO_SAMPLES(g_config.pre_roll_ms);
            if(pre > w->history_len){
                pre = w->history_len;
            }
            memcpy(w->utter, w->history + (w->history_len - pre), sizeof(float) * (size_t)pre);
            w->utter_len = pre;
            w->utter_start = w->stream_pos - (uint64_t)pre;
            w->in_speech = 1;
            w->speech_samples = 0;
            w->silence_samples = 0;
            svs_post_message(slot_index, w->generation, "{\"type\":\"speech_start\",\"t0\":%lld}",
                (long long)SVS_SAMPLES_TO_MS(w->utter_start));
        }
    }

    if(w->in_speech){
        memcpy(w->utter + w->utter_len, frame, sizeof(float) * SVS_VAD_FRAME);
        w->utter_len += SVS_VAD_FRAME;
        if(prob >= off_thold){
            w->silence_samples = 0;
            if(prob >= on_thold){
                w->speech_samples += SVS_VAD_FRAME;
            }
        } else {
            w->silence_samples += SVS_VAD_FRAME;
        }
    }

    svs_history_push(w, frame, SVS_VAD_FRAME);
    w->stream_pos += SVS_VAD_FRAME;

    if(w->in_speech){
        if(w->silence_samples >= SVS_MS_TO_SAMPLES(g_config.silence_end_ms)){
            svs_utterance_end(slot_index, w, "silence");
        } else if(w->utter_len + SVS_VAD_FRAME > SVS_UTTER_MAX_SAMPLES){
            svs_utterance_end(slot_index, w, "max_length");
        }
    }
}

/* pending にある完全な frame をまとめて VAD にかけ、frame ごとに状態遷移させる */
static void svs_worker_run_vad(int slot_index, SVS_WORKER_STATE* w)
{
    int32_t n_frames = w->pending_len / SVS_VAD_FRAME;
    int32_t n_new = n_frames * SVS_VAD_FRAME;
    int32_t hist_frames = w->history_len / SVS_VAD_FRAME;
    int32_t hist_len = hist_frames * SVS_VAD_FRAME;
    const float* probs;
    int32_t n_probs;
    int32_t i;

    if(n_frames <= 0){
        return;
    }

    memcpy(w->vad_input, w->history + (w->history_len - hist_len), sizeof(float) * (size_t)hist_len);
    memcpy(w->vad_input + hist_len, w->pending, sizeof(float) * (size_t)n_new);
    if(!whisper_vad_detect_speech(g_vad_ctx, w->vad_input, hist_len + n_new)){
        printf("[SVS][vad] detect_speech faild slot=%d\n", slot_index);
        return;
    }
    probs = whisper_vad_probs(g_vad_ctx);
    n_probs = whisper_vad_n_probs(g_vad_ctx);
    if(n_probs < hist_frames + n_frames){
        printf("[SVS][vad] unexpected probs count=%d expected=%d\n", n_probs, hist_frames + n_frames);
        return;
    }

    /* history 部分は LSTM の立ち上げ用なので、新しい frame の確率だけを使う */
    for(i = 0; i < n_frames; i++){
        svs_vad_step(slot_index, w, w->pending + (size_t)i * SVS_VAD_FRAME, probs[hist_frames + i]);
    }

    w->pending_len -= n_new;
    memmove(w->pending, w->pending + n_new, sizeof(float) * (size_t)w->pending_len);
}

/* 1 スロット分の処理。何か処理したら 1 を返す */
static int svs_worker_process_slot(int slot_index)
{
    SVS_SLOT* slot = &g_slots[slot_index];
    SVS_WORKER_STATE* w = &g_workers[slot_index];
    uint32_t generation;
    int stop_requested;
    int32_t i;

    pthread_mutex_lock(&g_lock);
    if(slot->state == SVS_SLOT_CLOSING){
        /* 接続が閉じられた。workerがバッファを解放してスロットを開ける */
        free(slot->inbox);
        memset(slot, 0, sizeof(*slot));
        slot->state = SVS_SLOT_FREE;
        pthread_mutex_unlock(&g_lock);
        svs_worker_free(w);
        return 1;
    }
    if(slot->state != SVS_SLOT_ACTIVE || (!slot->is_streaming && !slot->stop_requested)){
        pthread_mutex_unlock(&g_lock);
        return 0;
    }
    if(w->pending == NULL || w->generation != slot->generation){
        if(-1 == svs_worker_prepare(w)){
            pthread_mutex_unlock(&g_lock);
            printf("[SVS] buffer alloc failed slot=%d\n", slot_index);
            return 0;
        }
        svs_worker_reset(w, slot->generation);
    }
    generation = slot->generation;
    stop_requested = slot->stop_requested;
    slot->stop_requested = 0;
    if(slot->inbox_len == 0 && !stop_requested){
        pthread_mutex_unlock(&g_lock);
        return 0;
    }
    for(i = 0; i < slot->inbox_len; i++){
        w->pending[w->pending_len + i] = (float)slot->inbox[i] / 32768.0f;
    }
    w->pending_len += slot->inbox_len;
    slot->inbox_len = 0;
    pthread_mutex_unlock(&g_lock);

    svs_worker_run_vad(slot_index,w);

    if(stop_requested){
        if(w->in_speech){
            svs_utterance_end(slot_index, w, "stop");
        }
        svs_worker_reset(w, generation);
        svs_post_message(slot_index, generation, "{\"type\":\"stopped\"}");
    }
    return 1;
}

static void* svs_worker_main(void* arg)
{
    (void)arg;
    while(g_running){
        int did_work = 0;
        int i;
        for(i = 0; i < SVS_CONNECTION_MAX; i++){
            did_work |= svs_worker_process_slot(i);
        }
        if(!did_work){
            usleep(10 * 1000);
        }
    }
    return NULL;
}

/* -------------------------------------------------- */
/* slot management (main thread)                      */
/* -------------------------------------------------- */

/* g_lock を保持した状態で呼ぶ */
static int svs_find_slot_locked(uint32_t connection_offset)
{
    int i;
    for(i = 0; i < SVS_CONNECTION_MAX; i++){
        if(g_slots[i].state == SVS_SLOT_ACTIVE && g_slots[i].connection_offset == connection_offset){
            return i;
        }
    }
    return -1;
}

static int svs_start_session(uint32_t connection_offset)
{
    int slot_index;
    SVS_SLOT* slot;

    pthread_mutex_lock(&g_lock);
    slot_index = svs_find_slot_locked(connection_offset);

    if(slot_index < 0){
        int i;
        for(i = 0; i < SVS_CONNECTION_MAX; i++){
            if(g_slots[i].state == SVS_SLOT_FREE){
                slot_index = i;
                break;
            }
        }
        if(slot_index < 0){
            pthread_mutex_unlock(&g_lock);
            printf("[SVS][slot] no free slot connection_offset=%u\n", connection_offset);
            return -1;
        }
        slot = &g_slots[slot_index];
        slot->inbox = (int16_t*)malloc(sizeof(int16_t) * SVS_INBOX_SAMPLES);
        if(!slot->inbox){
            pthread_mutex_unlock(&g_lock);
            return -1;
        }
        slot->connection_offset = connection_offset;
        slot->state = SVS_SLOT_ACTIVE;
    }

    slot = &g_slots[slot_index];
    slot->generation = ++g_generation_counter;
    slot->is_streaming = 1;
    slot->stop_requested = 0;
    slot->inbox_len = 0;
    slot->inbox_overflow_logged = 0;
    pthread_mutex_unlock(&g_lock);

    printf("[SVS][session] start slot=%d connection_offset=%u generation=%d\n", slot_index, connection_offset, slot->generation);
    svs_post_message(slot_index, slot->generation, "{\"type\":\"ready\"}");
    return slot_index;
}

static void svs_stop_session(uint32_t connection_offset)
{
    int slot_index;
    pthread_mutex_lock(&g_lock);
    slot_index = svs_find_slot_locked(connection_offset);
    if(slot_index >= 0 && g_slots[slot_index].is_streaming){
        g_slots[slot_index].is_streaming = 0;
        g_slots[slot_index].stop_requested = 1;
    }
    pthread_mutex_unlock(&g_lock);
    if(slot_index >= 0){
        printf("[SVS][session] stop slot=%d connection_offset=%u\n", slot_index, connection_offset);
    }
}

static void svs_push_pcm(uint32_t connection_offset, const int16_t* samples, int32_t n_samples)
{
    int slot_index;
    SVS_SLOT* slot;
    int32_t space;

    pthread_mutex_lock(&g_lock);
    slot_index = svs_find_slot_locked(connection_offset);
    if(slot_index < 0 || !g_slots[slot_index].is_streaming){
        pthread_mutex_unlock(&g_lock);
        return;
    }
    slot = &g_slots[slot_index];
    space = SVS_INBOX_SAMPLES - slot->inbox_len;
    if(n_samples > space){
        if(!slot->inbox_overflow_logged){
            printf("[SVS][slot] inbox overflow slot=%d connection_offset=%u, dropping audio\n", slot_index, connection_offset);
            slot->inbox_overflow_logged = 1;
        }
        n_samples = space;
    }
    memcpy(slot->inbox + slot->inbox_len, samples, sizeof(int16_t) * (size_t)n_samples);
    slot->inbox_len += n_samples;
    pthread_mutex_unlock(&g_lock);
}

/* -------------------------------------------------- */
/* main / event handlers                              */
/* -------------------------------------------------- */

int main(int argc, char* argv[], char* envp[])
{
    QS_SERVER_CONTEXT* context = 0;
    pthread_t worker;
    int i;

    /* ログをファイルにリダイレクトしても逐次出るように行バッファにする */
    setvbuf(stdout, NULL, _IOLBF, 0);
    if(-1 == api_qs_memory_alloc(&g_temporary_memory, 1024 * 1024 * 4)){
        printf("[SVS][main] failed to allocate temporary memory\n");
        return -1;
    }
    svs_config_default(&g_config);
    if(-1 == svs_config_load(&g_config, "./server.conf")){
        printf("[SVS][main] failed to load configuration %s\n", "./server.conf");
        return -1;
    }
    printf("[SVS][main] language=%s vad_threshold=%.2f silence_end_ms=%d min_speech_ms=%d pre_roll_ms%d\n",
           g_config.language,
           g_config.vad_threshold,
           g_config.silence_end_ms,
           g_config.min_speech_ms,
           g_config.pre_roll_ms);
    if(-1==svs_init_models()){
        printf("[SVS][main] failed to initialize models\n");
        return -1;
    }

    if(0 > api_qs_server_init(&context, g_config.server_port, g_config.max_connection, QS_SERVER_TYPE_HTTP)){ return -1; }
    if(-1==api_qs_set_scheduler(context, g_config.scheduler_mode)){ return -1; }
    api_qs_set_on_connect_event(context, on_connect);
    api_qs_set_on_http_event(context, on_http_event);
    api_qs_set_on_websocket_event(context, on_ws_event);
    api_qs_set_on_close_event(context, on_close);

    signal(SIGINT, svs_on_signal);
    signal(SIGTERM, svs_on_signal);
    if(0 != pthread_create(&worker, NULL, svs_worker_main, NULL)){
        printf("[SVS][main] failed to create worker thread\n");
        return -1;
    }
    printf("[SVS][main] server started . listening on http://127.0.0.1:%d\n", g_config.server_port);

    while(g_running){
        api_qs_update(context);
        svs_flush_out_queue(context);
        api_qs_sleep(context);
    }

    printf("[SVS][main] shutting down\n");
    pthread_join(worker, NULL);
    api_qs_free(context);
    for(i=0; i<SVS_CONNECTION_MAX; i++){
        free(g_slots[i].inbox);
        svs_worker_free(&g_workers[i]);
    }
    while(g_out_count > 0){
        free(g_out_queue[g_out_head].data);
        g_out_head = (g_out_head + 1) % SVS_OUT_QUEUE_MAX;
        g_out_count--;
    }
    svs_free_models();
    api_qs_memory_free(&g_temporary_memory);
    return 0;
}

int on_connect(QS_EVENT_PARAMETER params)
{
    return 0;
}

int on_http_event(QS_EVENT_PARAMETER params)
{
    /* 内部の標準の静的ファイル配信に任せる */
    return 404;
}

int on_ws_event(QS_EVENT_PARAMETER params)
{
    uint32_t connection_offset = api_qs_get_connection_offset(params);
    char* message = api_qs_get_ws_message(params);
    ssize_t size = api_qs_get_ws_message_size(params);
    uint8_t opcode = api_qs_get_ws_opcode(params);

    if(!message || size <= 0){
        return 0;
    }

    if(opcode == 2){
        if((size % (ssize_t)sizeof(int16_t)) != 0){
            printf("[SVS][ws] odd pcm byte count=%zd connection_offset=%u\n", size, connection_offset);
            return 0;
        }
        svs_push_pcm(connection_offset, (const int16_t*)message, (int32_t)(size / (ssize_t)sizeof(int16_t)));
        return 0;
    }

    if(opcode == 1){
        QS_JSON_ELEMENT_OBJECT object;
        char* type;

        api_qs_memory_clean(&g_temporary_memory);
        if(-1 == api_qs_json_decode_object(&g_temporary_memory, &object, message)){
            api_qs_memory_clean(&g_temporary_memory);
            return 0;
        }
        type = api_qs_object_get_string(&object, "type");
        if(type && !strcmp(type, "stt_start")){
            int32_t sample_rate = api_qs_object_get_integer_val(&object, "sample_rate");
            int32_t channels = api_qs_object_get_integer_val(&object, "channels");
            int32_t bits = api_qs_object_get_integer_val(&object, "bits_per_sample");
            if(sample_rate != SVS_SAMPLE_RATE || channels != 1 || bits != 16){
                printf("[SVS][ws] unsupported format rate=%d ch=%d bits=%d\n", sample_rate, channels, bits);
            } else {
                svs_start_session(connection_offset);
            }
        } else if(type && !strcmp(type, "stt_stop")){
            svs_stop_session(connection_offset);
        }
        api_qs_memory_clean(&g_temporary_memory);
    }
    return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
    uint32_t connection_offset = api_qs_get_connection_offset(params);
    int slot_index;

    pthread_mutex_lock(&g_lock);
    slot_index = svs_find_slot_locked(connection_offset);
    if(slot_index >= 0){
        /* 解放は workerに任せる(推論中の可能性があるため)*/
        g_slots[slot_index].state = SVS_SLOT_CLOSING;    
    }
    pthread_mutex_unlock(&g_lock);
    if(slot_index >= 0){
        printf("[SVS][slot] close slot=%d connection_offset=%u\n", slot_index, connection_offset);
    }
    return 0;
}