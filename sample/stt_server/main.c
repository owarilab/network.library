/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
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
	char last_logged_text[1024];
	FILE* wav_file;
	FILE* txt_file;
	uint32_t sample_rate;
	uint16_t channels;
	uint16_t bits_per_sample;
	uint32_t pcm_data_bytes;
	uint32_t chunk_count;
	uint32_t session_id;
	int is_recording;
	char wav_path[256];
	char txt_path[256];
} STT_CONNECTION_DATA;

#define STT_CONNECTION_MAX 128
#define STT_TARGET_SAMPLE_RATE 16000
#define STT_RING_BUFFER_SECONDS 30
#define STT_RING_BUFFER_SAMPLES (STT_TARGET_SAMPLE_RATE * STT_RING_BUFFER_SECONDS)
#define STT_STEP_MS 500
#define STT_KEEP_MS 200
#define STT_STEP_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_STEP_MS) / 1000)
#define STT_KEEP_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_KEEP_MS) / 1000)
#define STT_VAD_COOLDOWN_MS 3000
#define STT_VAD_PROBE_LENGTH_MS 2000
#define STT_VAD_INFERENCE_LENGTH_MS 5000
#define STT_VAD_PROBE_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_VAD_PROBE_LENGTH_MS) / 1000)
#define STT_VAD_INFERENCE_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_VAD_INFERENCE_LENGTH_MS) / 1000)
#define STT_VAD_THRESHOLD 0.15f
#define STT_VAD_N_FRAMES 20
#define STT_VAD_FRAME_MS 100
#define STT_VAD_MIN_ENERGY 0.02f
#define STT_VAD_MEDIAN_THRESHOLD 1e-6f
#define STT_WINDOW_MIN_RMS 0.010f
#define STT_WINDOW_MIN_PEAK 0.050f
#define STT_WHISPER_NO_SPEECH_THOLD 0.60f
#define STT_WHISPER_LOGPROB_THOLD -0.80f
#define STT_TEXT_CONTEXT_EXPIRE_MS 6000

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
static int64_t stt_now_ms(void);
static int stt_vad_simple(const float* pcm, int n_samples, float thold_prob, float* prob_out);
static void stt_measure_signal_stats(const float* pcm, int n_samples, float* rms_out, float* peak_out);
static int stt_is_non_speech_text(const char* text);
static const char* stt_extract_incremental_text(STT_CONNECTION_DATA* con_data, const char* full_text);
static void stt_run_inference_window(STT_CONNECTION_DATA* con_data, const int16_t* samples, int32_t sample_count, int64_t window_start_samples);
static void stt_copy_recent_samples(const STT_CONNECTION_DATA* con_data, int16_t* dst, int32_t sample_count);
static void stt_send_json_message(QS_EVENT_PARAMETER params, const char* type, const char* path, uint32_t bytes, uint32_t chunks);

QS_MEMORY_CONTEXT g_temporary_memory;
QS_MEMORY_CONTEXT g_kvs_memory;
QS_KVS_CONTEXT g_kvs;

static uint32_t g_stt_session_counter = 0;
static STT_CONNECTION_DATA g_stt_connections[STT_CONNECTION_MAX];
static struct whisper_context* g_whisper_ctx = NULL;

static int64_t stt_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000LL + (int64_t)(ts.tv_nsec / 1000000LL);
}

static int stt_vad_simple(const float* pcm, int n_samples, float thold_prob, float* prob_out)
{
	const int frame_len = STT_TARGET_SAMPLE_RATE * STT_VAD_FRAME_MS / 1000;
	const int n_frames_total = n_samples / frame_len;
	float* energies;
	float* sorted;
	float energy_median;
	float prob;
	int n_check;
	int speech_frames = 0;
	int i;

	if (n_frames_total < 1) {
		if (prob_out) {
			*prob_out = 0.0f;
		}
		return 0;
	}

	energies = (float*)malloc((size_t)n_frames_total * sizeof(float));
	if (!energies) {
		if (prob_out) {
			*prob_out = 0.0f;
		}
		return 0;
	}

	for (i = 0; i < n_frames_total; i++) {
		double energy = 0.0;
		int j;
		for (j = 0; j < frame_len; j++) {
			const float sample = pcm[i * frame_len + j];
			energy += (double)sample * (double)sample;
		}
		energies[i] = (float)sqrt(energy / frame_len);
	}

	sorted = (float*)malloc((size_t)n_frames_total * sizeof(float));
	if (!sorted) {
		free(energies);
		if (prob_out) {
			*prob_out = 0.0f;
		}
		return 0;
	}
	memcpy(sorted, energies, (size_t)n_frames_total * sizeof(float));
	for (i = 1; i < n_frames_total; i++) {
		float key = sorted[i];
		int j = i - 1;
		while (j >= 0 && sorted[j] > key) {
			sorted[j + 1] = sorted[j];
			j--;
		}
		sorted[j + 1] = key;
	}
	if ((n_frames_total % 2) == 0) {
		int mid = n_frames_total / 2;
		energy_median = (sorted[mid - 1] + sorted[mid]) * 0.5f;
	} else {
		energy_median = sorted[n_frames_total / 2];
	}
	free(sorted);

	n_check = STT_VAD_N_FRAMES < n_frames_total ? STT_VAD_N_FRAMES : n_frames_total;
	for (i = n_frames_total - n_check; i < n_frames_total; i++) {
		if (energy_median > STT_VAD_MEDIAN_THRESHOLD && energies[i] > energy_median * thold_prob) {
			speech_frames++;
		} else if (energies[i] > STT_VAD_MIN_ENERGY) {
			speech_frames++;
		}
	}
	free(energies);

	prob = (float)speech_frames / (float)n_check;
	if (prob_out) {
		*prob_out = prob;
	}
	return prob >= thold_prob ? 1 : 0;
}

static int stt_init_whisper(void)
{
	struct whisper_context_params cparams;
	if (g_whisper_ctx) {
		return 0;
	}
	cparams = whisper_context_default_params();
	g_whisper_ctx = whisper_init_from_file_with_params("../../stt/models/ggml-large-v3-turbo.bin", cparams);
	if (!g_whisper_ctx) {
		printf("[STT][whisper] init failed: ../../stt/models/ggml-large-v3-turbo.bin\n");
		return -1;
	}
	printf("[STT][whisper] initialized: ggml-large-v3-turbo.bin\n");
	return 0;
}

static void stt_measure_signal_stats(const float* pcm, int n_samples, float* rms_out, float* peak_out)
{
	double sum = 0.0;
	float peak = 0.0f;
	int i;

	if (rms_out) {
		*rms_out = 0.0f;
	}
	if (peak_out) {
		*peak_out = 0.0f;
	}
	if (!pcm || n_samples <= 0) {
		return;
	}

	for (i = 0; i < n_samples; i++) {
		float abs_sample = pcm[i] < 0.0f ? -pcm[i] : pcm[i];
		sum += (double)pcm[i] * (double)pcm[i];
		if (abs_sample > peak) {
			peak = abs_sample;
		}
	}

	if (rms_out) {
		*rms_out = (float)sqrt(sum / n_samples);
	}
	if (peak_out) {
		*peak_out = peak;
	}
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

static const char* stt_extract_incremental_text(STT_CONNECTION_DATA* con_data, const char* full_text)
{
	const char* delta_text;

	if (!con_data || !full_text) {
		return NULL;
	}
	if (full_text[0] == '\0') {
		return NULL;
	}
	if (con_data->last_logged_text[0] == '\0') {
		strncpy(con_data->last_logged_text, full_text, sizeof(con_data->last_logged_text) - 1);
		return full_text;
	}
	if (!strcmp(con_data->last_logged_text, full_text)) {
		return NULL;
	}
	/* Reverse containment: new text is a subset of what was already logged.
	 * Handles hallucinations where Whisper produces fewer repetitions than the
	 * previous window (e.g. last="はい。×6", full="はい。×4" -> skip). */
	if (strstr(con_data->last_logged_text, full_text) != NULL) {
		return NULL;
	}
	/* Prefix-based delta extraction: new text starts with last text */
	if (strstr(full_text, con_data->last_logged_text) == full_text) {
		delta_text = full_text + strlen(con_data->last_logged_text);
		while (*delta_text != '\0') {
			if (*delta_text == ' ' || *delta_text == '\t' || *delta_text == '\n' || *delta_text == '\r' || *delta_text == ',' || *delta_text == '.' || *delta_text == '!' || *delta_text == '?') {
				delta_text++;
				continue;
			}
			if (strncmp(delta_text, "、", strlen("、")) == 0) {
				delta_text += strlen("、");
				continue;
			}
			if (strncmp(delta_text, "。", strlen("。")) == 0) {
				delta_text += strlen("。");
				continue;
			}
			break;
		}
		strncpy(con_data->last_logged_text, full_text, sizeof(con_data->last_logged_text) - 1);
		if (*delta_text == '\0') {
			return NULL;
		}
		return delta_text;
	}
	strncpy(con_data->last_logged_text, full_text, sizeof(con_data->last_logged_text) - 1);
	return full_text;
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

static void stt_shutdown_whisper(void)
{
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
	char full_text[1024];
	const char* emit_text;
	int64_t last_seg_t1_ms = -1;

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
	wparams.single_segment = true;
	wparams.suppress_blank = true;
	wparams.suppress_nst = true;
	wparams.temperature = 0.4f; // default 0.0f
	wparams.temperature_inc = 0.0f;
	wparams.logprob_thold = STT_WHISPER_LOGPROB_THOLD;
	wparams.no_speech_thold = STT_WHISPER_NO_SPEECH_THOLD;

	ret = whisper_full(g_whisper_ctx, wparams, pcmf32, sample_count);
	if (ret != 0) {
		printf("[STT][infer] whisper_full failed connection_id=%s ret=%d samples=%d\n", con_data->connection_id, ret, sample_count);
		free(pcmf32);
		return;
	}

	full_text[0] = '\0';
	n_segments = whisper_full_n_segments(g_whisper_ctx);
	for (i = 0; i < n_segments; i++) {
		const char* seg_text = whisper_full_get_segment_text(g_whisper_ctx, i);
		float no_speech_prob = whisper_full_get_segment_no_speech_prob(g_whisper_ctx, i);
		if (no_speech_prob > max_no_speech_prob) {
			max_no_speech_prob = no_speech_prob;
		}
		int64_t seg_t1_ms = whisper_full_get_segment_t1(g_whisper_ctx, i);
		if (seg_t1_ms > last_seg_t1_ms) {
			last_seg_t1_ms = seg_t1_ms;
		}
		if (seg_text) {
			strncat(full_text, seg_text, sizeof(full_text) - strlen(full_text) - 1);
		}
	}
	if (max_no_speech_prob >= STT_WHISPER_NO_SPEECH_THOLD) {
		printf("[STT][infer] connection_id=%s window=%u skipped by no_speech_prob=%.2f text=%s\n",
			con_data->connection_id,
			con_data->window_count,
			max_no_speech_prob,
			full_text[0] != '\0' ? full_text : "[empty]");
		free(pcmf32);
		return;
	}
	if (stt_is_non_speech_text(full_text)) {
		printf("[STT][infer] connection_id=%s window=%u filtered non-speech text=%s\n",
			con_data->connection_id,
			con_data->window_count,
			full_text[0] != '\0' ? full_text : "[empty]");
		free(pcmf32);
		return;
	}
	emit_text = stt_extract_incremental_text(con_data, full_text);
	if (!emit_text) {
		printf("[STT][infer] connection_id=%s window=%u skipped duplicate text=%s\n",
			con_data->connection_id,
			con_data->window_count,
			full_text[0] != '\0' ? full_text : "[empty]");
		free(pcmf32);
		return;
	}
	printf("[STT][infer] connection_id=%s window=%u text=%s\n",
		con_data->connection_id,
		con_data->window_count,
		emit_text);
	if (con_data->txt_file) {
		fprintf(con_data->txt_file, "%s\n", emit_text);
		fflush(con_data->txt_file);
	}
	/* Update processed_samples using Whisper's segment timestamp.
	 * Always advance by at least INFERENCE_SAMPLES to prevent re-inference loops
	 * when Whisper hallucinates (produces long text with small timestamps). */
	int64_t abs_end = window_start_samples + STT_VAD_INFERENCE_SAMPLES;
	if (last_seg_t1_ms > 0) {
		int64_t ts_end = window_start_samples + last_seg_t1_ms * STT_TARGET_SAMPLE_RATE / 1000;
		if (ts_end > abs_end) {
			abs_end = ts_end;
		}
	}
	int64_t old_processed = con_data->processed_samples;
	con_data->processed_samples = abs_end;
	if (con_data->processed_samples > con_data->total_samples_received) {
		con_data->processed_samples = con_data->total_samples_received;
	}
	printf("[STT][debug] connection_id=%s window=%u total=%llu old_processed=%lu new_processed=%lu advance=%ld last_seg_t1_ms=%lld\n",
		con_data->connection_id, con_data->window_count,
		(unsigned long long)con_data->total_samples_received,
		(unsigned long)old_processed,
		(unsigned long)con_data->processed_samples,
		(long)(con_data->processed_samples - old_processed),
		(long long)last_seg_t1_ms);
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
	memset(con_data->last_logged_text, 0, sizeof(con_data->last_logged_text));
	memset(con_data->wav_path, 0, sizeof(con_data->wav_path));
	memset(con_data->txt_path, 0, sizeof(con_data->txt_path));
	con_data->txt_file = NULL;
	memset(con_data->overlap_buffer, 0, sizeof(con_data->overlap_buffer));
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
		int64_t now_ms;
		int64_t elapsed_ms;
		float vad_prob = 0.0f;
		float* probe_f32;
		int has_speech;
		int32_t sample_index;
		int32_t inference_samples;
		int16_t probe_window[STT_VAD_PROBE_SAMPLES];
		int16_t infer_window[STT_VAD_INFERENCE_SAMPLES];

		if (con_data->connection_id[0] == '\0' || !con_data->ring_buffer) {
			continue;
		}

		/* Need minimum audio for VAD probe */
		if (con_data->total_samples_received < STT_VAD_PROBE_SAMPLES) {
			continue;
		}

		/* Advance one step at a time: only process when STEP_SAMPLES of new audio
		 * have arrived since the last probe. This matches stt/main.cpp VAD mode
		 * where the probe window advances with real incoming audio, preventing
		 * the same 5-second window from being inferred repeatedly. */
		new_samples = con_data->total_samples_received - con_data->last_probe_total_samples;
		if (new_samples < (uint64_t)STT_STEP_SAMPLES) {
			continue;
		}
		con_data->last_probe_total_samples += STT_STEP_SAMPLES;
		con_data->window_count++;

		// printf(
		// 	"[STT][window] connection_id=%s window=%u total_received=%llu new_since_probe=%llu\n",
		// 	con_data->connection_id,
		// 	con_data->window_count,
		// 	(unsigned long long)con_data->total_samples_received,
		// 	(unsigned long long)new_samples
		// );

		/* VAD probe: check last STT_VAD_PROBE_LENGTH_MS of audio */
		stt_copy_recent_samples(con_data, probe_window, STT_VAD_PROBE_SAMPLES);
		probe_f32 = (float*)malloc(sizeof(float) * STT_VAD_PROBE_SAMPLES);
		if (!probe_f32) {
			printf("[STT][vad] probe alloc failed connection_id=%s\n", con_data->connection_id);
			continue;
		}
		for (sample_index = 0; sample_index < STT_VAD_PROBE_SAMPLES; sample_index++) {
			probe_f32[sample_index] = (float)probe_window[sample_index] / 32768.0f;
		}
		has_speech = stt_vad_simple(probe_f32, STT_VAD_PROBE_SAMPLES, STT_VAD_THRESHOLD, &vad_prob);
		free(probe_f32);

		if (!has_speech) {
			// printf("[STT][probe] connection_id=%s window=%u speech_ratio=%.2f [SILENT]\n",
			// 	con_data->connection_id,
			// 	con_data->window_count,
			// 	vad_prob);
			continue;
		}

		/* Cooldown: prevent inference more often than STT_VAD_COOLDOWN_MS
		 * (matches stt/main.cpp: t_last_detection cooldown) */
		now_ms = stt_now_ms();
		elapsed_ms = now_ms - con_data->last_inference_time_ms;
		if (con_data->last_inference_time_ms > 0 && elapsed_ms < STT_VAD_COOLDOWN_MS) {
			// printf("[STT][cooldown] connection_id=%s window=%u elapsed_ms=%lld\n",
			// 	con_data->connection_id,
			// 	con_data->window_count,
			// 	(long long)elapsed_ms);
			continue;
		}

		/* Reset stale text context so a new speech segment starts fresh */
		if (con_data->last_inference_time_ms > 0 &&
			(now_ms - con_data->last_inference_time_ms) > STT_TEXT_CONTEXT_EXPIRE_MS) {
			memset(con_data->last_logged_text, 0, sizeof(con_data->last_logged_text));
		}

		/* Grab inference window: from max(processed, total - INFERENCE) to total.
	 * This ensures we only infer on audio that hasn't been processed yet. */
		int64_t window_start = (int64_t)con_data->total_samples_received - STT_VAD_INFERENCE_SAMPLES;
		if ((uint64_t)window_start < con_data->processed_samples) {
			window_start = (int64_t)con_data->processed_samples;
		}
		inference_samples = (int32_t)(con_data->total_samples_received - window_start);

		/* DEBUG: log if we're stuck */
		if (inference_samples <= 0) {
			printf("[STT][debug] connection_id=%s window=%u total=%llu processed=%lu probe_total=%lu SKIP_ZERO\n",
				con_data->connection_id, con_data->window_count,
				(unsigned long long)con_data->total_samples_received,
				(unsigned long)con_data->processed_samples,
				(unsigned long)con_data->last_probe_total_samples);
			continue;
		}

		/* DEBUG: log window params */
		printf("[STT][debug] connection_id=%s window=%u total=%llu processed=%lu window_start=%ld inference_samples=%d\n",
			con_data->connection_id, con_data->window_count,
			(unsigned long long)con_data->total_samples_received,
			(unsigned long)con_data->processed_samples,
			(long)window_start, inference_samples);

		stt_copy_recent_samples(con_data, infer_window, inference_samples);
		stt_run_inference_window(con_data, infer_window, inference_samples, window_start);
		con_data->last_inference_time_ms = stt_now_ms();
		/* Advance probe baseline to current position so the NEXT probe is driven
		 * by fresh incoming audio, not residual queued steps over the same window. */
		con_data->last_probe_total_samples = con_data->total_samples_received;
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
	con_data->txt_file = fopen(con_data->txt_path, "a");
	if (!con_data->txt_file) {
		printf("[STT] failed to open txt file: %s\n", con_data->txt_path);
	}
	if (-1 == stt_write_wav_header(con_data->wav_file, con_data->sample_rate, con_data->channels, con_data->bits_per_sample, 0)) {
		if (con_data->txt_file) { fclose(con_data->txt_file); con_data->txt_file = NULL; }
		fclose(con_data->wav_file);
		remove(con_data->wav_path);
		stt_reset_connection_data(con_data);
		return -1;
	}
	if (0 != fseek(con_data->wav_file, 0, SEEK_END)) {
		if (con_data->txt_file) { fclose(con_data->txt_file); con_data->txt_file = NULL; }
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
