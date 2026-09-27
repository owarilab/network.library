/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include "qs_api.h"
#include "qs_io.h"
#include "whisper.h"

typedef struct STT_CONNECTION_DATA_STRUCT
{
	char connection_id[256];
	int16_t* ring_buffer;
	int32_t ring_capacity_samples;
	int32_t write_pos;
	int32_t read_pos;
	int32_t samples_count;
	uint64_t total_samples_received;
	int16_t overlap_buffer[(16000 * 200) / 1000];
	int32_t overlap_samples;
	uint32_t window_count;
	int64_t last_inference_time_ms;
	uint64_t last_probe_total_samples;
	uint64_t processed_samples;
	uint64_t last_emitted_sample;
	char last_emitted_text[1024];
	uint64_t last_emitted_text_start_sample;
	FILE* wav_file;
	FILE* txt_file;
	FILE* debug_file;
	uint32_t sample_rate;
	uint16_t channels;
	uint16_t bits_per_sample;
	uint32_t pcm_data_bytes;
	uint32_t chunk_count;
	uint32_t session_id;
	uint32_t partial_revision;
	uint64_t partial_last_inference_samples;
	int partial_is_clear;
	int is_recording;
	QS_SERVER_CONTEXT* server_context;
	uint32_t connection_offset;
	char wav_path[256];
	char txt_path[256];
} STT_CONNECTION_DATA;

#define STT_CONNECTION_MAX 128
#define STT_TARGET_SAMPLE_RATE 16000
#define STT_RING_BUFFER_SECONDS 30
#define STT_RING_BUFFER_SAMPLES (STT_TARGET_SAMPLE_RATE * STT_RING_BUFFER_SECONDS)
#define STT_STEP_MS 500
#define STT_STEP_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_STEP_MS) / 1000)
#define STT_PARTIAL_INTERVAL_MS 2000
#define STT_PARTIAL_INTERVAL_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_PARTIAL_INTERVAL_MS) / 1000)
#define STT_PARTIAL_WINDOW_SECONDS 8
#define STT_PARTIAL_WINDOW_SAMPLES (STT_TARGET_SAMPLE_RATE * STT_PARTIAL_WINDOW_SECONDS)
#define STT_VAD_PROBE_LENGTH_MS 2000
#define STT_VAD_PROBE_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_VAD_PROBE_LENGTH_MS) / 1000)
#define STT_VAD_ANALYSIS_LENGTH_MS 5000
#define STT_VAD_ANALYSIS_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_VAD_ANALYSIS_LENGTH_MS) / 1000)
#define STT_WHISPER_CONTEXT_SECONDS 8
#define STT_WHISPER_CONTEXT_SAMPLES (STT_TARGET_SAMPLE_RATE * STT_WHISPER_CONTEXT_SECONDS)
#define STT_WHISPER_MAX_WINDOW_SECONDS 30
#define STT_WHISPER_MAX_WINDOW_SAMPLES (STT_TARGET_SAMPLE_RATE * STT_WHISPER_MAX_WINDOW_SECONDS)
#define STT_VAD_THRESHOLD 0.5f
#define STT_VAD_MIN_SPEECH_MS 250
#define STT_VAD_MIN_SILENCE_MS 500
#define STT_VAD_SPEECH_PAD_MS 150
#define STT_WHISPER_NO_SPEECH_THOLD 0.60f
#define STT_WHISPER_LOGPROB_THOLD -0.80f

int on_connect(QS_EVENT_PARAMETER params);
int on_http_event(QS_EVENT_PARAMETER params);
int on_ws_event(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

static void stt_reset_connection_data(STT_CONNECTION_DATA* con_data);
static void stt_clear_connection_slot(STT_CONNECTION_DATA* con_data);
static STT_CONNECTION_DATA* stt_get_or_create_connection_data(const char* connection_id);
static STT_CONNECTION_DATA* stt_find_connection_data(const char* connection_id);
static void stt_remove_connection_data(const char* connection_id);
static int stt_ensure_connection_buffers(STT_CONNECTION_DATA* con_data);
static int stt_write_wav_header(FILE* fp, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample, uint32_t pcm_data_bytes);
static int stt_begin_recording(STT_CONNECTION_DATA* con_data, const char* connection_id, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample);
static int stt_append_pcm_chunk(STT_CONNECTION_DATA* con_data, const void* data, size_t size);
static int stt_finalize_recording(STT_CONNECTION_DATA* con_data, const char* connection_id, int discard_empty);
static int stt_append_pcm_to_ring_buffer(STT_CONNECTION_DATA* con_data, const int16_t* samples, int32_t sample_count);
static void stt_process_connections(void);
static int stt_init_whisper(void);
static void stt_shutdown_whisper(void);
static int stt_is_non_speech_text(const char* text);
static size_t stt_trim_recent_text_overlap(const STT_CONNECTION_DATA* con_data, const char* text, char* output, size_t output_size);
static size_t stt_trim_leading_text_overlap(const char* previous, const char* text, char* output, size_t output_size);
static void stt_debug_log(STT_CONNECTION_DATA* con_data, const char* format, ...);
static void stt_run_inference_window(STT_CONNECTION_DATA* con_data, const int16_t* samples, int32_t sample_count, int64_t window_start_samples);
static void stt_run_partial_inference(STT_CONNECTION_DATA* con_data);
static void stt_copy_recent_samples(const STT_CONNECTION_DATA* con_data, int16_t* dst, int32_t sample_count);
static void stt_send_json_message(QS_EVENT_PARAMETER params, const char* type, const char* path, uint32_t bytes, uint32_t chunks);
static void stt_send_partial_message(STT_CONNECTION_DATA* con_data, const char* text, uint32_t revision);

QS_MEMORY_CONTEXT g_temporary_memory;
QS_MEMORY_CONTEXT g_kvs_memory;
QS_KVS_CONTEXT g_kvs;

static uint32_t g_stt_session_counter = 0;
static STT_CONNECTION_DATA g_stt_connections[STT_CONNECTION_MAX];
static struct whisper_context* g_whisper_ctx = NULL;
static struct whisper_vad_context* g_whisper_vad_ctx = NULL;

static void stt_debug_log(STT_CONNECTION_DATA* con_data, const char* format, ...)
{
	va_list args;
	if (!con_data || !con_data->debug_file || !format) return;
	va_start(args, format);
	vfprintf(con_data->debug_file, format, args);
	va_end(args);
	fflush(con_data->debug_file);
}

static int stt_init_whisper(void)
{
	struct whisper_context_params cparams;
	struct whisper_vad_context_params vad_cparams;
	char model_path[256];
	char vad_model_path[256];
	if (g_whisper_ctx) {
		return 0;
	}
	snprintf(model_path, sizeof(model_path), "../../stt/models/ggml-large-v3.bin");
	cparams = whisper_context_default_params();
	cparams.dtw_token_timestamps = true;
	cparams.dtw_aheads_preset = WHISPER_AHEADS_LARGE_V3;
	g_whisper_ctx = whisper_init_from_file_with_params(model_path, cparams);
	if (!g_whisper_ctx) {
		printf("[STT][whisper] init failed: %s\n", model_path);
		return -1;
	}
	snprintf(vad_model_path, sizeof(vad_model_path), "../../stt/models/ggml-silero-v5.1.2.bin");
	vad_cparams = whisper_vad_default_context_params();
	vad_cparams.n_threads = 1;
	g_whisper_vad_ctx = whisper_vad_init_from_file_with_params(vad_model_path, vad_cparams);
	if (!g_whisper_vad_ctx) {
		printf("[STT][vad] init failed: %s\n", vad_model_path);
		whisper_free(g_whisper_ctx);
		g_whisper_ctx = NULL;
		return -1;
	}
	printf("[STT][vad] initialized: %s\n", vad_model_path);
	printf("[STT][whisper] initialized: %s\n", model_path);
	return 0;
}

static int stt_is_non_speech_text(const char* text)
{
	const char* normalized;
	if (!text) {
		return 1;
	}
	while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r') {
		text++;
	}
	if (*text == '\0') {
		return 1;
	}
	normalized = text;
	if (strstr(normalized, "ご視聴ありがとうございました") != NULL ||
		strstr(normalized, "ありがとうございました") != NULL ||
		strstr(normalized, "(音楽)") != NULL ||
		strstr(normalized, "[音楽]") != NULL ||
		strstr(normalized, "(拍手)") != NULL ||
		strstr(normalized, "[拍手]") != NULL ||
		strstr(normalized, "♪") != NULL) {
		return 1;
	}
	return 0;
}

static size_t stt_trim_recent_text_overlap(const STT_CONNECTION_DATA* con_data, const char* text, char* output, size_t output_size)
{
	if (!text || !output || output_size == 0) return 0;
	return stt_trim_leading_text_overlap(con_data ? con_data->last_emitted_text : NULL,
		text, output, output_size);
}

static int stt_has_non_punctuation_text(const char* text)
{
	const unsigned char* current = (const unsigned char*)text;
	if (!current) return 0;
	while (*current) {
		if (*current < 0x80u) {
			if ((*current >= '0' && *current <= '9') || (*current >= 'A' && *current <= 'Z') ||
				(*current >= 'a' && *current <= 'z')) return 1;
			current++;
			continue;
		}
		if ((*current & 0xe0u) == 0xc0u && current[1] != '\0') {
			if (current[0] != 0xc2u ||
				(current[1] < 0xa1u || current[1] > 0xbfu) ||
				(current[1] >= 0xa1u && current[1] <= 0xa6u) ||
				current[1] == 0xa8u || current[1] == 0xa9u || current[1] == 0xabu ||
				(current[1] >= 0xadu && current[1] <= 0xb1u) || current[1] == 0xbbu ||
				current[1] == 0xbfu) return 1;
			current += 2;
			continue;
		}
		if ((*current & 0xf0u) == 0xe0u && current[1] != '\0' && current[2] != '\0') {
			if (!(current[0] == 0xe3u && current[1] == 0x80u &&
				(current[2] >= 0x80u && current[2] <= 0x81u))) return 1;
			current += 3;
			continue;
		}
		return 1;
	}
	return 0;
}

static size_t stt_trim_leading_text_overlap(const char* previous, const char* text, char* output, size_t output_size)
{
	size_t previous_length;
	size_t text_length;
	size_t overlap = 0;
	size_t index;
	if (!text || !output || output_size == 0) return 0;
	text_length = strlen(text);
	if (!previous || !*previous) {
		if (text_length >= output_size) text_length = output_size - 1;
		memcpy(output, text, text_length);
		output[text_length] = '\0';
		return text_length;
	}
	previous_length = strlen(previous);
	for (index = 0; index < previous_length; index++) {
		size_t candidate = previous_length - index;
		if (candidate < 6 || candidate > text_length) continue;
		if (memcmp(previous + index, text, candidate) == 0) {
			overlap = candidate;
			break;
		}
	}
	while (overlap > 0 && overlap < text_length && (((unsigned char)text[overlap] & 0xc0u) == 0x80u)) overlap--;
	if (overlap < 6) overlap = 0;
	text += overlap;
	text_length -= overlap;
	while (text_length > 0 && (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r')) {
		text++;
		text_length--;
	}
	if (text_length >= output_size) text_length = output_size - 1;
	memcpy(output, text, text_length);
	output[text_length] = '\0';
	return text_length;
}

static void stt_copy_recent_samples(const STT_CONNECTION_DATA* con_data, int16_t* dst, int32_t sample_count)
{
	int32_t start_pos;
	int32_t i;

	if (!con_data || !dst || !con_data->ring_buffer || sample_count <= 0 || sample_count > con_data->ring_capacity_samples) {
		return;
	}
	start_pos = con_data->write_pos - sample_count;
	while (start_pos < 0) {
		start_pos += con_data->ring_capacity_samples;
	}
	for (i = 0; i < sample_count; i++) {
		dst[i] = con_data->ring_buffer[(start_pos + i) % con_data->ring_capacity_samples];
	}
}

static void stt_copy_absolute_samples(const STT_CONNECTION_DATA* con_data, int64_t start_sample, int16_t* dst, int32_t sample_count)
{
	int32_t start_pos;
	int32_t i;
	if (!con_data || !dst || !con_data->ring_buffer || sample_count <= 0 || sample_count > con_data->ring_capacity_samples) return;
	start_pos = (int32_t)(start_sample % con_data->ring_capacity_samples);
	for (i = 0; i < sample_count; i++) {
		dst[i] = con_data->ring_buffer[(start_pos + i) % con_data->ring_capacity_samples];
	}
}

static void stt_shutdown_whisper(void)
{
	if (g_whisper_vad_ctx) {
		whisper_vad_free(g_whisper_vad_ctx);
		g_whisper_vad_ctx = NULL;
		printf("[STT][vad] freed\n");
	}
	if (g_whisper_ctx) {
		whisper_free(g_whisper_ctx);
		g_whisper_ctx = NULL;
		printf("[STT][whisper] freed\n");
	}
}

static void stt_run_inference_window(STT_CONNECTION_DATA* con_data, const int16_t* samples, int32_t sample_count, int64_t window_start_samples)
{
	float* pcmf32;
	struct whisper_full_params wparams;
	float max_no_speech_prob = 0.0f;
	int i;
	int ret;
	int n_segments;
	int64_t max_emitted_sample;

	if (!con_data || !samples || sample_count <= 0 || !g_whisper_ctx) {
		return;
	}

	pcmf32 = (float*)malloc(sizeof(float) * (size_t)sample_count);
	if (!pcmf32) {
		printf("[STT][infer] float buffer alloc failed connection_id=%s samples=%d\n", con_data->connection_id, sample_count);
		return;
	}
	for (i = 0; i < sample_count; i++) {
		pcmf32[i] = (float)samples[i] / 32768.0f;
	}

	wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	wparams.language = "ja";
	wparams.translate = false;
	wparams.print_progress = false;
	wparams.print_realtime = false;
	wparams.print_timestamps = true;
	wparams.no_context = true;
	wparams.single_segment = false;
	wparams.suppress_blank = true;
	wparams.suppress_nst = true;
	wparams.temperature = 0.0f; // default 0.0f
	wparams.temperature_inc = 0.0f;
	wparams.logprob_thold = STT_WHISPER_LOGPROB_THOLD;
	wparams.no_speech_thold = STT_WHISPER_NO_SPEECH_THOLD;

	ret = whisper_full(g_whisper_ctx, wparams, pcmf32, sample_count);
	if (ret != 0) {
		printf("[STT][infer] whisper_full failed connection_id=%s ret=%d samples=%d\n", con_data->connection_id, ret, sample_count);
		stt_debug_log(con_data, "INFER_ERROR start_sample=%lld sample_count=%d ret=%d\n",
			(long long)window_start_samples, sample_count, ret);
		free(pcmf32);
		return;
	}

	n_segments = whisper_full_n_segments(g_whisper_ctx);
	stt_debug_log(con_data, "INFER start_sample=%lld end_sample=%lld duration_ms=%lld segments=%d processed_before=%llu emitted_before=%llu\n",
		(long long)window_start_samples, (long long)(window_start_samples + sample_count),
		(long long)((int64_t)sample_count * 1000 / STT_TARGET_SAMPLE_RATE), n_segments,
		(unsigned long long)con_data->processed_samples, (unsigned long long)con_data->last_emitted_sample);
	max_emitted_sample = con_data->last_emitted_sample;
	for (i = 0; i < n_segments; i++) {
		const char* seg_text = whisper_full_get_segment_text(g_whisper_ctx, i);
		float no_speech_prob = whisper_full_get_segment_no_speech_prob(g_whisper_ctx, i);
		int64_t seg_t0 = window_start_samples + whisper_full_get_segment_t0(g_whisper_ctx, i) * 10 * STT_TARGET_SAMPLE_RATE / 1000;
		int64_t seg_t1 = window_start_samples + whisper_full_get_segment_t1(g_whisper_ctx, i) * 10 * STT_TARGET_SAMPLE_RATE / 1000;
		char segment_text[1024];
		char output_text[1024];
		const char* text = seg_text;
		const char* discard_reason = NULL;
		while (text && (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r')) text++;
		if (!text || !*text) discard_reason = "empty";
		else if (no_speech_prob >= STT_WHISPER_NO_SPEECH_THOLD) discard_reason = "no_speech";
		else if (seg_t1 <= (int64_t)con_data->last_emitted_sample) discard_reason = "already_emitted";
		else if (stt_is_non_speech_text(text)) discard_reason = "filtered_text";
		if (discard_reason) {
			stt_debug_log(con_data, "SEGMENT index=%d t0_sample=%lld t1_sample=%lld no_speech=%.4f decision=discard reason=%s text=%s\n",
				i, (long long)seg_t0, (long long)seg_t1, no_speech_prob, discard_reason, text ? text : "");
			continue;
		}
		snprintf(segment_text, sizeof(segment_text), "%s", text);
		if (seg_t0 < (int64_t)con_data->last_emitted_sample) {
			size_t output_length = stt_trim_recent_text_overlap(con_data, segment_text, output_text, sizeof(output_text));
			if (output_length == 0 ||
				(strcmp(segment_text, output_text) != 0 && !stt_has_non_punctuation_text(output_text))) {
				stt_debug_log(con_data, "SEGMENT index=%d t0_sample=%lld t1_sample=%lld no_speech=%.4f decision=discard reason=duplicate_text_overlap text=%s\n",
					i, (long long)seg_t0, (long long)seg_t1, no_speech_prob, text);
				continue;
			}
			if (strcmp(segment_text, output_text) != 0) {
				stt_debug_log(con_data, "SEGMENT index=%d overlap_trim original=%s output=%s\n", i, segment_text, output_text);
				snprintf(segment_text, sizeof(segment_text), "%s", output_text);
			}
		}
		stt_debug_log(con_data, "SEGMENT index=%d t0_sample=%lld t1_sample=%lld no_speech=%.4f decision=emit text=%s\n",
			i, (long long)seg_t0, (long long)seg_t1, no_speech_prob, segment_text);
		printf("[STT][infer] connection_id=%s window=%u t0=%lld t1=%lld text=%s\n",
			con_data->connection_id, con_data->window_count,
			(long long)seg_t0, (long long)seg_t1, segment_text);
		if (con_data->server_context != NULL) {
			char linebuf[1024];
			int len = snprintf(linebuf, sizeof(linebuf), "%s\n", segment_text);
			api_qs_send_ws_binary_by_connection_offset(con_data->server_context, con_data->connection_offset, linebuf, (size_t)len);
			if (con_data->txt_file) {
				fprintf(con_data->txt_file, "%s\n", segment_text);
				fflush(con_data->txt_file);
			}
		}
		snprintf(con_data->last_emitted_text, sizeof(con_data->last_emitted_text), "%s", segment_text);
		con_data->last_emitted_text_start_sample = (uint64_t)seg_t0;
		if (seg_t1 > max_emitted_sample) max_emitted_sample = seg_t1;
		if (no_speech_prob > max_no_speech_prob) max_no_speech_prob = no_speech_prob;
	}
	if (max_emitted_sample > (int64_t)con_data->last_emitted_sample) {
		con_data->last_emitted_sample = (uint64_t)max_emitted_sample;
	}
	stt_debug_log(con_data, "INFER_DONE processed=%llu emitted=%llu total=%llu\n",
		(unsigned long long)con_data->processed_samples, (unsigned long long)con_data->last_emitted_sample,
		(unsigned long long)con_data->total_samples_received);
	// printf("[STT][debug] connection_id=%s window=%u total=%llu old_processed=%lu new_processed=%lu advance=%ld last_seg_t1_ms=%lld\n",
	// 	con_data->connection_id, con_data->window_count,
	// 	(unsigned long long)con_data->total_samples_received,
	// 	(unsigned long)old_processed,
	// 	(unsigned long)con_data->processed_samples,
	// 	(long)(con_data->processed_samples - old_processed),
	// 	(long long)last_seg_t1_ms);
	free(pcmf32);
}

static void stt_run_partial_inference(STT_CONNECTION_DATA* con_data)
{
	int16_t* samples;
	float* pcmf32;
	struct whisper_full_params wparams;
	char partial_text[8192];
	int sample_count;
	int i;
	int ret;
	int n_segments;
	int text_length = 0;
	int64_t window_start_samples;
	uint64_t inference_end_samples;
	struct timespec infer_start_time;
	struct timespec infer_end_time;
	int64_t infer_elapsed_ms;

	if (!con_data || !con_data->is_recording || !g_whisper_ctx ||
		con_data->samples_count < STT_PARTIAL_WINDOW_SAMPLES) return;

	sample_count = STT_PARTIAL_WINDOW_SAMPLES;
	samples = (int16_t*)malloc(sizeof(int16_t) * (size_t)sample_count);
	pcmf32 = (float*)malloc(sizeof(float) * (size_t)sample_count);
	if (!samples || !pcmf32) {
		free(samples);
		free(pcmf32);
		return;
	}
	stt_copy_recent_samples(con_data, samples, sample_count);
	for (i = 0; i < sample_count; i++) {
		pcmf32[i] = (float)samples[i] / 32768.0f;
	}
	inference_end_samples = con_data->total_samples_received;
	window_start_samples = (int64_t)(inference_end_samples - (uint64_t)sample_count);
	con_data->partial_last_inference_samples = inference_end_samples;

	wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	wparams.language = "ja";
	wparams.translate = false;
	wparams.print_progress = false;
	wparams.print_realtime = false;
	wparams.print_timestamps = false;
	wparams.no_context = true;
	wparams.single_segment = false;
	wparams.suppress_blank = true;
	wparams.suppress_nst = true;
	wparams.temperature = 0.0f;
	wparams.temperature_inc = 0.0f;
	wparams.logprob_thold = STT_WHISPER_LOGPROB_THOLD;
	wparams.no_speech_thold = STT_WHISPER_NO_SPEECH_THOLD;

	clock_gettime(CLOCK_MONOTONIC, &infer_start_time);
	ret = whisper_full(g_whisper_ctx, wparams, pcmf32, sample_count);
	clock_gettime(CLOCK_MONOTONIC, &infer_end_time);
	infer_elapsed_ms = (int64_t)(infer_end_time.tv_sec - infer_start_time.tv_sec) * 1000 +
		(infer_end_time.tv_nsec - infer_start_time.tv_nsec) / 1000000;
	if (ret != 0) {
		stt_debug_log(con_data, "PARTIAL_ERROR end_sample=%llu ret=%d\n",
			(unsigned long long)inference_end_samples, ret);
		free(samples);
		free(pcmf32);
		return;
	}

	partial_text[0] = '\0';
	n_segments = whisper_full_n_segments(g_whisper_ctx);
	for (i = 0; i < n_segments; i++) {
		const char* segment = whisper_full_get_segment_text(g_whisper_ctx, i);
		float no_speech_prob = whisper_full_get_segment_no_speech_prob(g_whisper_ctx, i);
		int segment_length;
		while (segment && (*segment == ' ' || *segment == '\t' || *segment == '\n' || *segment == '\r')) segment++;
		if (!segment || !*segment || no_speech_prob >= STT_WHISPER_NO_SPEECH_THOLD || stt_is_non_speech_text(segment)) continue;
		segment_length = (int)strlen(segment);
		if (text_length + segment_length >= (int)sizeof(partial_text) - 1) break;
		memcpy(partial_text + text_length, segment, (size_t)segment_length);
		text_length += segment_length;
		partial_text[text_length] = '\0';
	}
	stt_send_partial_message(con_data, partial_text, ++con_data->partial_revision);
	stt_debug_log(con_data, "PARTIAL end_sample=%llu window_start=%lld input_ms=%d duration_ms=%lld revision=%u text=%s\n",
		(unsigned long long)inference_end_samples, (long long)window_start_samples,
		sample_count * 1000 / STT_TARGET_SAMPLE_RATE, (long long)infer_elapsed_ms,
		con_data->partial_revision, partial_text);
	free(samples);
	free(pcmf32);
}

static void stt_reset_connection_data(STT_CONNECTION_DATA* con_data)
{
	if (!con_data) {
		return;
	}
	con_data->wav_file = NULL;
	con_data->sample_rate = STT_TARGET_SAMPLE_RATE;
	con_data->channels = 1;
	con_data->bits_per_sample = 16;
	con_data->pcm_data_bytes = 0;
	con_data->chunk_count = 0;
	con_data->session_id = 0;
	con_data->partial_revision = 0;
	con_data->partial_last_inference_samples = 0;
	con_data->partial_is_clear = 1;
	con_data->is_recording = 0;
	con_data->write_pos = 0;
	con_data->read_pos = 0;
	con_data->samples_count = 0;
	con_data->total_samples_received = 0;
	con_data->overlap_samples = 0;
	con_data->window_count = 0;
	con_data->last_inference_time_ms = 0;
	con_data->last_probe_total_samples = 0;
	con_data->processed_samples = 0;
	con_data->last_emitted_sample = 0;
	memset(con_data->last_emitted_text, 0, sizeof(con_data->last_emitted_text));
	con_data->last_emitted_text_start_sample = 0;
	memset(con_data->wav_path, 0, sizeof(con_data->wav_path));
	memset(con_data->txt_path, 0, sizeof(con_data->txt_path));
	con_data->txt_file = NULL;
	con_data->debug_file = NULL;
	memset(con_data->overlap_buffer, 0, sizeof(con_data->overlap_buffer));
	con_data->server_context = NULL;
	con_data->connection_offset = 0;
}

static void stt_clear_connection_slot(STT_CONNECTION_DATA* con_data)
{
	if (!con_data) {
		return;
	}
	if (con_data->wav_file) {
		fclose(con_data->wav_file);
	}
	if (con_data->txt_file) {
		fclose(con_data->txt_file);
	}
	if (con_data->debug_file) {
		fclose(con_data->debug_file);
	}
	if (con_data->ring_buffer) {
		free(con_data->ring_buffer);
	}
	memset(con_data, 0, sizeof(*con_data));
}

static int stt_ensure_connection_buffers(STT_CONNECTION_DATA* con_data)
{
	if (!con_data) {
		return -1;
	}
	if (!con_data->ring_buffer) {
		con_data->ring_buffer = (int16_t*)malloc(sizeof(int16_t) * STT_RING_BUFFER_SAMPLES);
		if (!con_data->ring_buffer) {
			printf("[STT][ring] alloc failed samples=%d\n", STT_RING_BUFFER_SAMPLES);
			return -1;
		}
		con_data->ring_capacity_samples = STT_RING_BUFFER_SAMPLES;
		memset(con_data->ring_buffer, 0, sizeof(int16_t) * STT_RING_BUFFER_SAMPLES);
	}
	return 0;
}

static STT_CONNECTION_DATA* stt_find_connection_data(const char* connection_id)
{
	int i;
	if (!connection_id || connection_id[0] == '\0') {
		return NULL;
	}
	for (i = 0; i < STT_CONNECTION_MAX; i++) {
		if (g_stt_connections[i].connection_id[0] != '\0' && !strcmp(g_stt_connections[i].connection_id, connection_id)) {
			return &g_stt_connections[i];
		}
	}
	return NULL;
}

static STT_CONNECTION_DATA* stt_get_or_create_connection_data(const char* connection_id)
{
	int i;
	STT_CONNECTION_DATA* con_data;

	con_data = stt_find_connection_data(connection_id);
	if (con_data) {
		return con_data;
	}
	if (!connection_id || connection_id[0] == '\0') {
		return NULL;
	}
	for (i = 0; i < STT_CONNECTION_MAX; i++) {
		if (g_stt_connections[i].connection_id[0] == '\0') {
			if (-1 == stt_ensure_connection_buffers(&g_stt_connections[i])) {
				return NULL;
			}
			stt_reset_connection_data(&g_stt_connections[i]);
			strncpy(g_stt_connections[i].connection_id, connection_id, sizeof(g_stt_connections[i].connection_id) - 1);
			printf("[STT][map] create slot=%d connection_id=%s\n", i, connection_id);
			return &g_stt_connections[i];
		}
	}
	printf("[STT][map] no free slot for connection_id=%s\n", connection_id);
	return NULL;
}

static void stt_remove_connection_data(const char* connection_id)
{
	STT_CONNECTION_DATA* con_data = stt_find_connection_data(connection_id);
	if (!con_data) {
		return;
	}
	stt_clear_connection_slot(con_data);
	printf("[STT][map] remove connection_id=%s\n", connection_id ? connection_id : "");
}

static int stt_append_pcm_to_ring_buffer(STT_CONNECTION_DATA* con_data, const int16_t* samples, int32_t sample_count)
{
	int32_t i;

	if (!con_data || !samples || sample_count <= 0 || !con_data->ring_buffer || con_data->ring_capacity_samples <= 0) {
		return -1;
	}

	for (i = 0; i < sample_count; i++) {
		con_data->ring_buffer[con_data->write_pos] = samples[i];
		con_data->write_pos = (con_data->write_pos + 1) % con_data->ring_capacity_samples;
		con_data->total_samples_received++;
		if (con_data->samples_count < con_data->ring_capacity_samples) {
			con_data->samples_count++;
		} else {
			con_data->read_pos = (con_data->read_pos + 1) % con_data->ring_capacity_samples;
		}
	}
	return 0;
}

static void stt_process_connections(void)
{
	int i;
	for (i = 0; i < STT_CONNECTION_MAX; i++) {
		STT_CONNECTION_DATA* con_data = &g_stt_connections[i];
		uint64_t new_samples;
		float* analysis_f32;
		struct whisper_vad_params vad_params;
		struct whisper_vad_segments* vad_segments;
		int vad_segment_count;
		int32_t sample_index;
		int32_t analysis_samples;
		int16_t analysis_window[STT_VAD_ANALYSIS_SAMPLES];

		if (con_data->connection_id[0] == '\0' || !con_data->ring_buffer) {
			continue;
		}

		if (con_data->is_recording && con_data->total_samples_received >= STT_PARTIAL_WINDOW_SAMPLES &&
			con_data->total_samples_received - con_data->partial_last_inference_samples >= STT_PARTIAL_INTERVAL_SAMPLES) {
			stt_run_partial_inference(con_data);
		}

		if (!g_whisper_vad_ctx || con_data->total_samples_received < STT_VAD_ANALYSIS_SAMPLES) {
			continue;
		}
		if (con_data->processed_samples + STT_VAD_ANALYSIS_SAMPLES > con_data->total_samples_received) {
			continue;
		}

		/* Analyze each new audio step with a rolling window to allow speech that
		 * began before the window to be recognized without sharing VAD state. */
		new_samples = con_data->total_samples_received - con_data->last_probe_total_samples;
		if (new_samples < (uint64_t)STT_STEP_SAMPLES) {
			continue;
		}
		con_data->last_probe_total_samples = con_data->total_samples_received;
		con_data->window_count++;
		analysis_samples = STT_VAD_ANALYSIS_SAMPLES;
		stt_debug_log(con_data, "VAD_WINDOW index=%u start_sample=%llu end_sample=%llu processed=%llu\n",
			con_data->window_count,
			(unsigned long long)(con_data->total_samples_received - analysis_samples),
			(unsigned long long)con_data->total_samples_received,
			(unsigned long long)con_data->processed_samples);
		if (analysis_samples <= 0 || analysis_samples > con_data->ring_capacity_samples) {
			continue;
		}
		stt_copy_recent_samples(con_data, analysis_window, analysis_samples);
		analysis_f32 = (float*)malloc(sizeof(float) * (size_t)analysis_samples);
		if (!analysis_f32) {
			printf("[STT][vad] analysis alloc failed connection_id=%s\\n", con_data->connection_id);
			continue;
		}
		for (sample_index = 0; sample_index < analysis_samples; sample_index++) {
			analysis_f32[sample_index] = (float)analysis_window[sample_index] / 32768.0f;
		}
		vad_params = whisper_vad_default_params();
		vad_params.threshold = STT_VAD_THRESHOLD;
		vad_params.min_speech_duration_ms = STT_VAD_MIN_SPEECH_MS;
		vad_params.min_silence_duration_ms = STT_VAD_MIN_SILENCE_MS;
		vad_params.max_speech_duration_s = STT_VAD_ANALYSIS_LENGTH_MS / 1000.0f;
		vad_params.speech_pad_ms = STT_VAD_SPEECH_PAD_MS;
		vad_segments = whisper_vad_segments_from_samples(g_whisper_vad_ctx, vad_params, analysis_f32, analysis_samples);
		free(analysis_f32);
		if (!vad_segments) continue;
		vad_segment_count = whisper_vad_segments_n_segments(vad_segments);
		stt_debug_log(con_data, "VAD_RESULT index=%u segments=%d\n", con_data->window_count, vad_segment_count);
		if (con_data->is_recording && vad_segment_count == 0 && !con_data->partial_is_clear) {
			stt_send_partial_message(con_data, "", ++con_data->partial_revision);
			con_data->partial_is_clear = 1;
			stt_debug_log(con_data, "PARTIAL_CLEAR reason=vad_silence revision=%u sample=%llu\n",
				con_data->partial_revision, (unsigned long long)con_data->total_samples_received);
		}
		for (sample_index = 0; sample_index < vad_segment_count; sample_index++) {
			int64_t seg_start = (int64_t)con_data->total_samples_received - analysis_samples +
				(int64_t)(whisper_vad_segments_get_segment_t0(vad_segments, sample_index) * STT_TARGET_SAMPLE_RATE / 100);
			int64_t seg_end = (int64_t)con_data->total_samples_received - analysis_samples +
				(int64_t)(whisper_vad_segments_get_segment_t1(vad_segments, sample_index) * STT_TARGET_SAMPLE_RATE / 100);
			int64_t analysis_start = (int64_t)con_data->total_samples_received - analysis_samples;
			int64_t ring_start = (int64_t)con_data->total_samples_received - con_data->samples_count;
			int64_t inference_start;
			int32_t segment_samples;
			int16_t* segment_pcm;
			if (seg_end >= analysis_start + analysis_samples) {
				stt_debug_log(con_data, "VAD_SEGMENT index=%d start=%lld end=%lld decision=skip reason=window_edge\n",
					sample_index, (long long)seg_start, (long long)seg_end);
				continue;
			}
			if (seg_end <= (int64_t)con_data->processed_samples || seg_end > (int64_t)con_data->total_samples_received) {
				stt_debug_log(con_data, "VAD_SEGMENT index=%d start=%lld end=%lld decision=skip reason=processed_or_future\n",
					sample_index, (long long)seg_start, (long long)seg_end);
				continue;
			}
			if (seg_start < (int64_t)con_data->processed_samples) {
				stt_debug_log(con_data, "VAD_SEGMENT index=%d start=%lld end=%lld decision=skip reason=overlaps_committed_audio\n",
					sample_index, (long long)seg_start, (long long)seg_end);
				continue;
			}
			if (seg_start < analysis_start) seg_start = analysis_start;
			if (seg_end <= seg_start) {
				stt_debug_log(con_data, "VAD_SEGMENT index=%d start=%lld end=%lld decision=skip reason=empty\n",
					sample_index, (long long)seg_start, (long long)seg_end);
				continue;
			}
			inference_start = (int64_t)con_data->processed_samples;
			if (inference_start > seg_start) inference_start = seg_start;
			if (inference_start > ring_start + STT_WHISPER_CONTEXT_SAMPLES) {
				inference_start -= STT_WHISPER_CONTEXT_SAMPLES;
			} else {
				inference_start = ring_start;
			}
			if (seg_end - inference_start > STT_WHISPER_MAX_WINDOW_SAMPLES) {
				stt_debug_log(con_data, "VAD_SEGMENT index=%d start=%lld end=%lld inference_start=%lld decision=skip reason=context_exceeds_window\n",
					sample_index, (long long)seg_start, (long long)seg_end, (long long)inference_start);
				continue;
			}
			if (inference_start < ring_start || inference_start >= seg_end) {
				stt_debug_log(con_data, "VAD_SEGMENT index=%d start=%lld end=%lld inference_start=%lld decision=skip reason=invalid_window\n",
					sample_index, (long long)seg_start, (long long)seg_end, (long long)inference_start);
				continue;
			}
			segment_samples = (int32_t)(seg_end - inference_start);
			stt_debug_log(con_data, "VAD_SEGMENT index=%d speech_start=%lld speech_end=%lld inference_start=%lld inference_end=%lld samples=%d duration_ms=%lld decision=infer\n",
				sample_index, (long long)seg_start, (long long)seg_end, (long long)inference_start,
				(long long)seg_end, segment_samples,
				(long long)((int64_t)segment_samples * 1000 / STT_TARGET_SAMPLE_RATE));
			segment_pcm = (int16_t*)malloc(sizeof(int16_t) * (size_t)segment_samples);
			if (!segment_pcm) continue;
			stt_copy_absolute_samples(con_data, inference_start, segment_pcm, segment_samples);
			{
				struct timespec infer_start_time;
				struct timespec infer_end_time;
				int64_t infer_elapsed_ms = -1;
				uint64_t received_before = con_data->total_samples_received;
				clock_gettime(CLOCK_MONOTONIC, &infer_start_time);
			stt_run_inference_window(con_data, segment_pcm, segment_samples, inference_start);
				clock_gettime(CLOCK_MONOTONIC, &infer_end_time);
				infer_elapsed_ms = (int64_t)(infer_end_time.tv_sec - infer_start_time.tv_sec) * 1000 +
					(infer_end_time.tv_nsec - infer_start_time.tv_nsec) / 1000000;
				stt_debug_log(con_data, "INFER_TIMING duration_ms=%lld input_ms=%lld received_during=%llu total_after=%llu processed_after=%llu\n",
					(long long)infer_elapsed_ms,
					(long long)((int64_t)segment_samples * 1000 / STT_TARGET_SAMPLE_RATE),
					(unsigned long long)(con_data->total_samples_received - received_before),
					(unsigned long long)con_data->total_samples_received,
					(unsigned long long)con_data->processed_samples);
			}
			if ((uint64_t)seg_end > con_data->processed_samples) con_data->processed_samples = (uint64_t)seg_end;
			free(segment_pcm);
		}
		whisper_vad_free_segments(vad_segments);
	}
}

static int stt_write_wav_header(FILE* fp, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample, uint32_t pcm_data_bytes)
{
	uint32_t chunk_size;
	uint32_t byte_rate;
	uint16_t block_align;
	uint32_t fmt_size;
	uint16_t audio_format;

	if (!fp || channels == 0 || bits_per_sample == 0) {
		return -1;
	}

	chunk_size = 36 + pcm_data_bytes;
	byte_rate = sample_rate * channels * (bits_per_sample / 8);
	block_align = (uint16_t)(channels * (bits_per_sample / 8));
	fmt_size = 16;
	audio_format = 1;

	if (0 != fseek(fp, 0, SEEK_SET)) {
		return -1;
	}
	if (4 != fwrite("RIFF", 1, 4, fp)) {
		return -1;
	}
	if (1 != fwrite(&chunk_size, sizeof(chunk_size), 1, fp)) {
		return -1;
	}
	if (4 != fwrite("WAVE", 1, 4, fp)) {
		return -1;
	}
	if (4 != fwrite("fmt ", 1, 4, fp)) {
		return -1;
	}
	if (1 != fwrite(&fmt_size, sizeof(fmt_size), 1, fp)) {
		return -1;
	}
	if (1 != fwrite(&audio_format, sizeof(audio_format), 1, fp)) {
		return -1;
	}
	if (1 != fwrite(&channels, sizeof(channels), 1, fp)) {
		return -1;
	}
	if (1 != fwrite(&sample_rate, sizeof(sample_rate), 1, fp)) {
		return -1;
	}
	if (1 != fwrite(&byte_rate, sizeof(byte_rate), 1, fp)) {
		return -1;
	}
	if (1 != fwrite(&block_align, sizeof(block_align), 1, fp)) {
		return -1;
	}
	if (1 != fwrite(&bits_per_sample, sizeof(bits_per_sample), 1, fp)) {
		return -1;
	}
	if (4 != fwrite("data", 1, 4, fp)) {
		return -1;
	}
	if (1 != fwrite(&pcm_data_bytes, sizeof(pcm_data_bytes), 1, fp)) {
		return -1;
	}
	return 0;
}

static int stt_begin_recording(STT_CONNECTION_DATA* con_data, const char* connection_id, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample)
{
	if (!con_data || !connection_id) {
		printf("[STT][begin] invalid args con_data=%p connection_id=%p\n", (void*)con_data, (void*)connection_id);
		return -1;
	}

	if (con_data->is_recording) {
		printf("[STT][begin] previous session still open, finalizing first: connection_id=%s path=%s bytes=%u chunks=%u\n",
			connection_id,
			con_data->wav_path,
			con_data->pcm_data_bytes,
			con_data->chunk_count);
		stt_finalize_recording(con_data, connection_id, 1);
	}

	stt_reset_connection_data(con_data);
	strncpy(con_data->connection_id, connection_id, sizeof(con_data->connection_id) - 1);
	if (-1 == stt_ensure_connection_buffers(con_data)) {
		return -1;
	}
	con_data->sample_rate = sample_rate > 0 ? sample_rate : 16000;
	con_data->channels = channels > 0 ? channels : 1;
	con_data->bits_per_sample = bits_per_sample > 0 ? bits_per_sample : 16;
	con_data->session_id = ++g_stt_session_counter;
	con_data->is_recording = 1;

	snprintf(con_data->wav_path, sizeof(con_data->wav_path), "./recv_%s_%u.wav", connection_id, con_data->session_id);
	con_data->wav_file = fopen(con_data->wav_path, "wb+");
	if (!con_data->wav_file) {
		printf("[STT] failed to open wav file: %s\n", con_data->wav_path);
		stt_reset_connection_data(con_data);
		return -1;
	}
	snprintf(con_data->txt_path, sizeof(con_data->txt_path), "./recv_%s_%u.txt", connection_id, con_data->session_id);
	con_data->txt_file = fopen(con_data->txt_path, "w");
	if (!con_data->txt_file) {
		printf("[STT] failed to open txt file: %s\n", con_data->txt_path);
	}
	{
		char debug_path[256];
		snprintf(debug_path, sizeof(debug_path), "./recv_%s_%u_debug.txt", connection_id, con_data->session_id);
		con_data->debug_file = fopen(debug_path, "w");
		if (!con_data->debug_file) {
			printf("[STT] failed to open debug log: %s\n", debug_path);
		}
		else {
			stt_debug_log(con_data, "SESSION connection_id=%s session_id=%u wav=%s sample_rate=%u channels=%u bits=%u\n",
				connection_id, con_data->session_id, con_data->wav_path, con_data->sample_rate,
				(unsigned int)con_data->channels, (unsigned int)con_data->bits_per_sample);
		}
	}
	if (-1 == stt_write_wav_header(con_data->wav_file, con_data->sample_rate, con_data->channels, con_data->bits_per_sample, 0)) {
		if (con_data->txt_file) { fclose(con_data->txt_file); con_data->txt_file = NULL; }
		if (con_data->debug_file) { fclose(con_data->debug_file); con_data->debug_file = NULL; }
		fclose(con_data->wav_file);
		remove(con_data->wav_path);
		stt_reset_connection_data(con_data);
		return -1;
	}
	if (0 != fseek(con_data->wav_file, 0, SEEK_END)) {
		if (con_data->txt_file) { fclose(con_data->txt_file); con_data->txt_file = NULL; }
		if (con_data->debug_file) { fclose(con_data->debug_file); con_data->debug_file = NULL; }
		fclose(con_data->wav_file);
		remove(con_data->wav_path);
		stt_reset_connection_data(con_data);
		return -1;
	}
	printf("[STT] begin recording: connection_id=%s path=%s sample_rate=%u channels=%u bits=%u\n",
		connection_id,
		con_data->wav_path,
		con_data->sample_rate,
		(unsigned int)con_data->channels,
		(unsigned int)con_data->bits_per_sample);
	return 0;
}

static int stt_append_pcm_chunk(STT_CONNECTION_DATA* con_data, const void* data, size_t size)
{
	int32_t sample_count;
	if (!con_data || !con_data->is_recording || !con_data->wav_file || !data || size == 0) {
		printf("[STT][append] invalid state con_data=%p recording=%d file=%p data=%p size=%zu\n",
			(void*)con_data,
			con_data ? con_data->is_recording : -1,
			con_data ? (void*)con_data->wav_file : NULL,
			(void*)data,
			size);
		return -1;
	}
	if ((size % sizeof(int16_t)) != 0) {
		printf("[STT][append] invalid pcm byte count=%zu\n", size);
		return -1;
	}
	sample_count = (int32_t)(size / sizeof(int16_t));
	if (-1 == stt_append_pcm_to_ring_buffer(con_data, (const int16_t*)data, sample_count)) {
		printf("[STT][append] ring buffer append failed samples=%d connection_id=%s\n", sample_count, con_data->connection_id);
		return -1;
	}
	if (size != fwrite(data, 1, size, con_data->wav_file)) {
		printf("[STT][append] fwrite failed size=%zu path=%s\n", size, con_data->wav_path);
		return -1;
	}
	con_data->pcm_data_bytes += (uint32_t)size;
	con_data->chunk_count += 1;
	stt_debug_log(con_data, "AUDIO chunk=%u chunk_samples=%d total_samples=%llu pcm_bytes=%u\n",
		con_data->chunk_count, sample_count, (unsigned long long)con_data->total_samples_received,
		con_data->pcm_data_bytes);
	fflush(con_data->wav_file);
	fflush(con_data->wav_file);
	return 0;
}

static int stt_finalize_recording(STT_CONNECTION_DATA* con_data, const char* connection_id, int discard_empty)
{
	char saved_path[256];
	uint32_t saved_bytes;
	uint32_t saved_chunks;

	if (!con_data || !con_data->wav_file) {
		printf("[STT][finalize] no open file con_data=%p file=%p connection_id=%s\n",
			(void*)con_data,
			con_data ? (void*)con_data->wav_file : NULL,
			connection_id ? connection_id : "");
		return -1;
	}

	printf("[STT][finalize] start connection_id=%s path=%s bytes=%u chunks=%u discard_empty=%d\n",
		connection_id ? connection_id : "",
		con_data->wav_path,
		con_data->pcm_data_bytes,
		con_data->chunk_count,
		discard_empty);
	stt_debug_log(con_data, "FINALIZE total_samples=%llu processed=%llu emitted=%llu bytes=%u chunks=%u\n",
		(unsigned long long)con_data->total_samples_received, (unsigned long long)con_data->processed_samples,
		(unsigned long long)con_data->last_emitted_sample, con_data->pcm_data_bytes, con_data->chunk_count);

	memset(saved_path, 0, sizeof(saved_path));
	strncpy(saved_path, con_data->wav_path, sizeof(saved_path) - 1);
	saved_bytes = con_data->pcm_data_bytes;
	saved_chunks = con_data->chunk_count;

	if (-1 == stt_write_wav_header(con_data->wav_file, con_data->sample_rate, con_data->channels, con_data->bits_per_sample, con_data->pcm_data_bytes)) {
		printf("[STT][finalize] failed to rewrite wav header path=%s\n", saved_path);
		fclose(con_data->wav_file);
		con_data->wav_file = NULL;
		remove(saved_path);
		stt_reset_connection_data(con_data);
		return -1;
	}
	fflush(con_data->wav_file);
	fclose(con_data->wav_file);
	con_data->wav_file = NULL;
	if (con_data->txt_file) {
		fflush(con_data->txt_file);
		fclose(con_data->txt_file);
		con_data->txt_file = NULL;
	}
	if (con_data->debug_file) {
		fflush(con_data->debug_file);
		fclose(con_data->debug_file);
		con_data->debug_file = NULL;
	}

	if (discard_empty && saved_bytes == 0) {
		remove(saved_path);
		printf("[STT] empty recording discarded: connection_id=%s path=%s\n", connection_id ? connection_id : "", saved_path);
		stt_reset_connection_data(con_data);
		return 0;
	}

	printf("[STT] wav saved: connection_id=%s path=%s pcm_bytes=%u chunks=%u\n",
		connection_id ? connection_id : "",
		saved_path,
		saved_bytes,
		saved_chunks);
	stt_reset_connection_data(con_data);
	return 0;
}

static void stt_send_json_message(QS_EVENT_PARAMETER params, const char* type, const char* path, uint32_t bytes, uint32_t chunks)
{
	QS_JSON_ELEMENT_OBJECT object;
	char* json;

	api_qs_memory_clean(&g_temporary_memory);
	api_qs_object_create(&g_temporary_memory, &object);
	api_qs_object_push_string(&object, "type", type);
	if (path && path[0] != '\0') {
		api_qs_object_push_string(&object, "path", path);
	}
	api_qs_object_push_unsigned_big_integer(&object, "bytes", bytes);
	api_qs_object_push_unsigned_big_integer(&object, "chunks", chunks);
	json = api_qs_json_encode_object(&object, 1024);
	api_qs_send_ws_message(params, json);
	api_qs_memory_clean(&g_temporary_memory);
}

static void stt_send_partial_message(STT_CONNECTION_DATA* con_data, const char* text, uint32_t revision)
{
	QS_JSON_ELEMENT_OBJECT object;
	char* json;
	if (!con_data || !con_data->server_context || !text) return;

	api_qs_memory_clean(&g_temporary_memory);
	api_qs_object_create(&g_temporary_memory, &object);
	api_qs_object_push_string(&object, "type", "stt_partial");
	api_qs_object_push_unsigned_big_integer(&object, "session_id", con_data->session_id);
	api_qs_object_push_unsigned_big_integer(&object, "revision", revision);
	api_qs_object_push_string(&object, "text", text);
	json = api_qs_json_encode_object(&object, 4096);
	if (json) {
		api_qs_send_ws_binary_by_connection_offset(con_data->server_context, con_data->connection_offset, json, strlen(json));
	}
	api_qs_memory_clean(&g_temporary_memory);
}

static void stt_send_partial_start(STT_CONNECTION_DATA* con_data)
{
	if (!con_data) return;
	con_data->partial_revision = 0;
	stt_send_partial_message(con_data, "", con_data->partial_revision);
}

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif
	if(-1==api_qs_memory_alloc(&g_temporary_memory,1024*1024*4))
	{
		printf("api_qs_memory_alloc failed\n");
		return -1;
	}
	if(-1==api_qs_memory_alloc(&g_kvs_memory, (size_t)(1024 * 1024) * (size_t)(256 + 16)))
	{
		printf("api_qs_memory_alloc failed\n");
		return -1;
	}
	if(-1==api_qs_kvs_create_b256mb(&g_kvs_memory, &g_kvs)){return -1;}
	if(-1==stt_init_whisper()){return -1;}
	int server_port = 8080;
	int scheduler_mode = QS_SCHEDULER_MODE_LOW;
	int32_t max_connection = 10;
	{
		QS_SERVER_SCRIPT_CONTEXT script;
		if(-1==api_qs_script_read_file(&g_temporary_memory, &script, "./server.conf")){return -1;}
		if(-1==api_qs_script_run(&script)){return -1;}
		if(0!=api_qs_script_get_parameter(&script,"server_port")){
			server_port = atoi(api_qs_script_get_parameter(&script,"server_port"));
		}
		if(0!=api_qs_script_get_parameter(&script,"scheduler_mode")){
			const char* sm = api_qs_script_get_parameter(&script,"scheduler_mode");
			if(!strcmp(sm,"high"))       scheduler_mode = QS_SCHEDULER_MODE_HIGH;
			else if(!strcmp(sm,"middle")) scheduler_mode = QS_SCHEDULER_MODE_MIDDLE;
			else                          scheduler_mode = QS_SCHEDULER_MODE_LOW;
		}
		if(0!=api_qs_script_get_parameter(&script,"max_connection")){
			int v = atoi(api_qs_script_get_parameter(&script,"max_connection"));
			if(v < 10) v = 10;
			if(v > 1000) v = 1000;
			max_connection = (int32_t)v;
		}
		api_qs_memory_clean(&g_temporary_memory);
	}
	QS_SERVER_CONTEXT* context = 0;
	if(0 > api_qs_server_init(&context,server_port,max_connection,QS_SERVER_TYPE_HTTP)){return -1;}
	if(-1==api_qs_set_scheduler(context,scheduler_mode)){return -1;}
	if(-1==api_qs_server_create_router(context)){return -1;}
	if(-1==api_qs_server_create_kvs(context,QS_KVS_MEMORY_TYPE_B1MB)){return -1;}
	//if(-1==api_qs_server_create_logger_access(context,"./access_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_debug(context,"./debug_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_error(context,"./error_log.txt")){return -1;}
	api_qs_set_on_connect_event(context, on_connect );
	api_qs_set_on_http_event(context, on_http_event );
	api_qs_set_on_websocket_event(context, on_ws_event );
	api_qs_set_on_close_event(context, on_close );

	for(;;){
		api_qs_update(context);
		stt_process_connections();
		api_qs_sleep(context);
	}
	api_qs_free(context);
	stt_shutdown_whisper();
	api_qs_memory_free(&g_temporary_memory);
	api_qs_memory_free(&g_kvs_memory);
	return 0;
}

int on_connect(QS_EVENT_PARAMETER params)
{
	char* connection_id = api_qs_get_connection_id(params);
	STT_CONNECTION_DATA* con_data = stt_get_or_create_connection_data(connection_id);
	if (con_data) {
		printf("[STT][connect] connection_id=%s map_ready=1\n", connection_id ? connection_id : "");
		return 0;
	}
	printf("[STT][connect] connection_id=%s map_ready=0\n", connection_id ? connection_id : "");
	return 0;
}

int on_http_event(QS_EVENT_PARAMETER params)
{
	int http_status_code = 404;
	return http_status_code;
}

int on_ws_event(QS_EVENT_PARAMETER params)
{
	static int file_counter = 0;
	char* connection_id = api_qs_get_connection_id(params);
	STT_CONNECTION_DATA* con_data = stt_get_or_create_connection_data(connection_id);
	char* message = api_qs_get_ws_message(params);
	ssize_t size = api_qs_get_ws_message_size(params);
	uint8_t opcode = api_qs_get_ws_opcode(params);

	if (!con_data || !connection_id) {
		printf("[STT][ws] missing connection data con_data=%p connection_id=%p opcode=%u size=%zd\n",
			(void*)con_data,
			(void*)connection_id,
			opcode,
			size);
		return 0;
	}

	// printf("[STT][ws] connection_id=%s opcode=%u size=%zd recording=%d chunks=%u bytes=%u\n",
	// 	connection_id,
	// 	opcode,
	// 	size,
	// 	con_data->is_recording,
	// 	con_data->chunk_count,
	// 	con_data->pcm_data_bytes);

	if (opcode == 1 && message != NULL && size > 0) {
		QS_JSON_ELEMENT_OBJECT object;
		char* msg_type;
		int32_t sample_rate;
		int32_t channels;
		int32_t bits_per_sample;

		api_qs_memory_clean(&g_temporary_memory);
		if (-1 == api_qs_json_decode_object(&g_temporary_memory, &object, message)) {
			api_qs_memory_clean(&g_temporary_memory);
			return 0;
		}

		msg_type = api_qs_object_get_string(&object, "type");
		if (!msg_type) {
			printf("[STT][ws] text frame without type: connection_id=%s payload=%s\n", connection_id, message);
			api_qs_memory_clean(&g_temporary_memory);
			return 0;
		}
		//printf("[STT][ws] text type=%s connection_id=%s\n", msg_type, connection_id);

		if (!strcmp(msg_type, "stt_init")) {
			sample_rate = api_qs_object_get_integer_val(&object, "sample_rate");
			channels = api_qs_object_get_integer_val(&object, "channels");
			bits_per_sample = api_qs_object_get_integer_val(&object, "bits_per_sample");
			if (-1 == stt_begin_recording(con_data, connection_id, (uint32_t)sample_rate, (uint16_t)channels, (uint16_t)bits_per_sample)) {
				printf("[STT][ws] stt_init failed connection_id=%s sample_rate=%d channels=%d bits=%d\n",
					connection_id,
					sample_rate,
					channels,
					bits_per_sample);
				api_qs_memory_clean(&g_temporary_memory);
				return 0;
			}
			con_data->server_context = api_qs_get_server_context(params);
			con_data->connection_offset = api_qs_get_connection_offset(params);
			stt_send_partial_start(con_data);
			stt_send_json_message(params, "stt_ready", con_data->wav_path, 0, 0);
		}
		else if (!strcmp(msg_type, "stt_stop")) {
			char saved_path[256];
			uint32_t saved_bytes = con_data->pcm_data_bytes;
			uint32_t saved_chunks = con_data->chunk_count;
			memset(saved_path, 0, sizeof(saved_path));
			strncpy(saved_path, con_data->wav_path, sizeof(saved_path) - 1);
			if (-1 == stt_finalize_recording(con_data, connection_id, 0)) {
				printf("[STT][ws] stt_stop finalize failed connection_id=%s\n", connection_id);
				api_qs_memory_clean(&g_temporary_memory);
				return 0;
			}
			stt_send_json_message(params, "stt_saved", saved_path, saved_bytes, saved_chunks);
		}

		api_qs_memory_clean(&g_temporary_memory);
		return 0;
	}

	if (opcode == 2) {
		if (con_data->is_recording && message != NULL && size > 0) {
			con_data->server_context = api_qs_get_server_context(params);
			con_data->connection_offset = api_qs_get_connection_offset(params);
			if (-1 == stt_append_pcm_chunk(con_data, message, (size_t)size)) {
				printf("[STT] failed to append pcm chunk: connection_id=%s size=%zd\n", connection_id, size);
			}
			else {
				// printf("[STT] pcm chunk: connection_id=%s chunk=%u size=%zd total=%u\n",
				// 	connection_id,
				// 	con_data->chunk_count,
				// 	size,
				// 	con_data->pcm_data_bytes);
			}
			return 0;
		}

		if (message != NULL && size > 0) {
			// Legacy fallback: save one binary message as one WAV file.
			char filepath[256];
			snprintf(filepath, sizeof(filepath), "./recv_%s_%d.wav", connection_id, file_counter++);
			//qs_fwrite_bin(filepath, (char*)message, (size_t)size);
			printf("[on_ws_event] binary recv: connection_id=%s, size=%zd, saved to %s\n",
				connection_id, size, filepath);
		}
		return 0;
	}
	return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
	char* connection_id = api_qs_get_connection_id(params);
	STT_CONNECTION_DATA* con_data = stt_find_connection_data(connection_id);
	printf("[STT][close] connection_id=%s con_data=%p recording=%d bytes=%u chunks=%u path=%s\n",
		connection_id ? connection_id : "",
		(void*)con_data,
		con_data ? con_data->is_recording : 0,
		con_data ? con_data->pcm_data_bytes : 0,
		con_data ? con_data->chunk_count : 0,
		(con_data && con_data->wav_path[0] != '\0') ? con_data->wav_path : "");
	if (con_data && con_data->is_recording) {
		stt_finalize_recording(con_data, connection_id, 0);
	}
	stt_remove_connection_data(connection_id);
	return 0;
}
