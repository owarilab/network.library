#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "whisper.h"

#define SAMPLE_RATE 16000
#define CONTEXT_SAMPLES (8 * SAMPLE_RATE)
#define MAX_WINDOW_SAMPLES (30 * SAMPLE_RATE)
#define STREAM_STEP_SAMPLES (2 * SAMPLE_RATE)
#define STABILITY_LAG_SAMPLES (2 * SAMPLE_RATE)
#define VAD_ANALYSIS_SAMPLES (5 * SAMPLE_RATE)
#define VAD_STEP_SAMPLES (SAMPLE_RATE / 2)
#define VAD_THRESHOLD 0.5f
#define VAD_MIN_SPEECH_MS 250
#define VAD_MIN_SILENCE_MS 500
#define VAD_SPEECH_PAD_MS 150
#define TEXT_CAPACITY 32768

typedef struct transcript_segment {
	int64_t start_sample;
	int64_t end_sample;
	char text[8192];
} TRANSCRIPT_SEGMENT;

typedef struct transcript_result {
	TRANSCRIPT_SEGMENT* segments;
	int count;
	int capacity;
} TRANSCRIPT_RESULT;

static uint16_t read_u16_le(const unsigned char* data)
{
	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const unsigned char* data)
{
	return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static int is_ignorable_text_segment(const char* text, float no_speech_prob)
{
	const char* cursor = text;
	if (!text || no_speech_prob >= 0.60f) return 1;
	while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') cursor++;
	return *cursor == '\0' || strstr(cursor, "ありがとうございました") != NULL ||
		strstr(cursor, "(音楽)") != NULL || strstr(cursor, "[音楽]") != NULL ||
		strstr(cursor, "(拍手)") != NULL || strstr(cursor, "[拍手]") != NULL ||
		strstr(cursor, "♪") != NULL;
}

static int decode_audio_window(struct whisper_context* context, const float* audio,
	int64_t start_sample, int sample_count, TRANSCRIPT_RESULT* result)
{
	struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	int segment_index;
	int count;

	memset(result, 0, sizeof(*result));
	params.language = "ja";
	params.translate = false;
	params.print_progress = false;
	params.print_realtime = false;
	params.print_timestamps = false;
	params.no_context = true;
	params.single_segment = false;
	params.suppress_blank = true;
	params.suppress_nst = true;
	params.temperature = 0.0f;
	params.temperature_inc = 0.0f;
	params.logprob_thold = -0.80f;
	params.no_speech_thold = 0.60f;
	if (whisper_full(context, params, audio, sample_count) != 0) return -1;

	count = whisper_full_n_segments(context);
	result->segments = (TRANSCRIPT_SEGMENT*)calloc((size_t)count, sizeof(*result->segments));
	if (!result->segments && count > 0) return -1;
	result->capacity = count;
	for (segment_index = 0; segment_index < count; segment_index++) {
		const char* text = whisper_full_get_segment_text(context, segment_index);
		float no_speech_prob = whisper_full_get_segment_no_speech_prob(context, segment_index);
		TRANSCRIPT_SEGMENT* segment;
		if (is_ignorable_text_segment(text, no_speech_prob)) continue;
		segment = &result->segments[result->count++];
		segment->start_sample = start_sample + whisper_full_get_segment_t0(context, segment_index) * 160;
		segment->end_sample = start_sample + whisper_full_get_segment_t1(context, segment_index) * 160;
		snprintf(segment->text, sizeof(segment->text), "%s", text);
	}
	return 0;
}

static void free_transcript_result(TRANSCRIPT_RESULT* result)
{
	if (result) {
		free(result->segments);
		memset(result, 0, sizeof(*result));
	}
}

static void flatten_transcript(const TRANSCRIPT_RESULT* result, char* text, size_t capacity)
{
	size_t used = 0;
	int index;
	if (capacity == 0) return;
	text[0] = '\0';
	for (index = 0; index < result->count; index++) {
		size_t length = strlen(result->segments[index].text);
		if (length >= capacity - used) length = capacity - used - 1;
		memcpy(text + used, result->segments[index].text, length);
		used += length;
		text[used] = '\0';
		if (used + 1 >= capacity) break;
	}
}

static size_t common_utf8_prefix(const char* left, const char* right)
{
	size_t index = 0;
	while (left[index] != '\0' && right[index] != '\0' && left[index] == right[index]) index++;
	while (index > 0 && (((unsigned char)left[index] & 0xc0u) == 0x80u)) index--;
	return index;
}

static int run_watermark_simulation(struct whisper_context* context, const float* audio, int total_samples)
{
	char committed[TEXT_CAPACITY] = "";
	char previous_hypothesis[TEXT_CAPACITY] = "";
	char latest_hypothesis[TEXT_CAPACITY] = "";
	int64_t previous_end = 0;
	int64_t committed_sample = 0;
	int observation_count = 0;
	int revision_count = 0;
	int late_revision_count = 0;
	int stable_observations = 0;
	int step;

	printf("WATERMARK_SIM step_ms=2000 stability_lag_ms=2000 context_ms=8000\n");
	for (step = STREAM_STEP_SAMPLES; step <= total_samples; step += STREAM_STEP_SAMPLES) {
		int64_t available_end = step;
		int64_t window_start;
		int window_samples;
		TRANSCRIPT_RESULT current_result;
		char current_hypothesis[TEXT_CAPACITY];
		size_t common_length;
		size_t committed_length;
		int current_changed;

		if (step + STREAM_STEP_SAMPLES > total_samples) available_end = total_samples;
		window_start = available_end > CONTEXT_SAMPLES ? available_end - CONTEXT_SAMPLES : 0;
		window_samples = (int)(available_end - window_start);
		if (decode_audio_window(context, audio + window_start, window_start, window_samples, &current_result) != 0) {
			fprintf(stderr, "Whisper stream inference failed at sample %lld\n", (long long)available_end);
			return -1;
		}
		flatten_transcript(&current_result, current_hypothesis, sizeof(current_hypothesis));
		current_changed = strcmp(current_hypothesis, latest_hypothesis) != 0;
		if (current_changed) revision_count++;
		committed_length = strlen(committed);
		if (committed_length > 0 && strncmp(current_hypothesis, committed, committed_length) != 0) {
			late_revision_count++;
			printf("WATERMARK_LATE_REVISION at_ms=%lld committed=%s hypothesis=%s\n",
				(long long)available_end * 1000 / SAMPLE_RATE, committed, current_hypothesis);
		}

		common_length = common_utf8_prefix(previous_hypothesis, current_hypothesis);
		if (common_length > committed_length) {
			memcpy(committed, current_hypothesis, common_length);
			committed[common_length] = '\0';
			committed_sample = available_end > STABILITY_LAG_SAMPLES ? available_end - STABILITY_LAG_SAMPLES : 0;
			stable_observations = 1;
		} else if (common_length == committed_length && common_length > 0) {
			stable_observations++;
		}
		if (committed_length > 0 && strncmp(current_hypothesis, committed, strlen(committed)) == 0) {
			printf("WATERMARK_UPDATE available_ms=%lld committed_ms=%lld stable_observations=%d committed=%s partial=%s\n",
				(long long)available_end * 1000 / SAMPLE_RATE,
				(long long)committed_sample * 1000 / SAMPLE_RATE,
				stable_observations, committed, current_hypothesis + strlen(committed));
		} else {
			printf("WATERMARK_UPDATE available_ms=%lld committed_ms=%lld stable_observations=%d committed=%s partial=%s\n",
				(long long)available_end * 1000 / SAMPLE_RATE,
				(long long)committed_sample * 1000 / SAMPLE_RATE,
				stable_observations, committed, current_hypothesis);
		}

		if (current_changed) {
			printf("WATERMARK_REVISION at_ms=%lld hypothesis=%s\n",
				(long long)available_end * 1000 / SAMPLE_RATE, current_hypothesis);
		}
		strncpy(previous_hypothesis, current_hypothesis, sizeof(previous_hypothesis) - 1);
		previous_hypothesis[sizeof(previous_hypothesis) - 1] = '\0';
		strncpy(latest_hypothesis, current_hypothesis, sizeof(latest_hypothesis) - 1);
		latest_hypothesis[sizeof(latest_hypothesis) - 1] = '\0';
		previous_end = available_end;
		observation_count++;
		free_transcript_result(&current_result);
	}
	if (previous_end < total_samples) {
		TRANSCRIPT_RESULT final_result;
		char final_hypothesis[TEXT_CAPACITY];
		int64_t window_start = total_samples > CONTEXT_SAMPLES ? total_samples - CONTEXT_SAMPLES : 0;
		if (decode_audio_window(context, audio + window_start, window_start,
			(int)(total_samples - window_start), &final_result) == 0) {
			flatten_transcript(&final_result, final_hypothesis, sizeof(final_hypothesis));
			if (strcmp(final_hypothesis, latest_hypothesis) != 0) revision_count++;
			if (strlen(committed) > 0 && strncmp(final_hypothesis, committed, strlen(committed)) != 0) {
				late_revision_count++;
			}
			strncpy(latest_hypothesis, final_hypothesis, sizeof(latest_hypothesis) - 1);
			latest_hypothesis[sizeof(latest_hypothesis) - 1] = '\0';
			free_transcript_result(&final_result);
		}
	}

	printf("WATERMARK_RESULT observations=%d revisions=%d late_revision_events=%d committed_chars=%zu final_hypothesis=%s\n",
		observation_count, revision_count, late_revision_count, strlen(committed), latest_hypothesis);
	return 0;
}

static int run_vad_watermark_baseline(struct whisper_context* context,
	struct whisper_vad_context* vad_context, const float* audio, int total_samples)
{
	struct whisper_vad_params vad_params = whisper_vad_default_params();
	int segment_index;
	int committed_segments = 0;
	int skipped_overlap_segments = 0;
	int64_t committed_sample = 0;
	char final_transcript[TEXT_CAPACITY] = "";
	size_t transcript_length = 0;
	int64_t analysis_end;

	vad_params.threshold = VAD_THRESHOLD;
	vad_params.min_speech_duration_ms = VAD_MIN_SPEECH_MS;
	vad_params.min_silence_duration_ms = VAD_MIN_SILENCE_MS;
	vad_params.max_speech_duration_s = (float)VAD_ANALYSIS_SAMPLES / SAMPLE_RATE;
	vad_params.speech_pad_ms = VAD_SPEECH_PAD_MS;
	printf("VAD_WATERMARK_BASELINE mode=rolling step_ms=500 analysis_ms=5000 commit_lag_ms=1000\n");
	for (analysis_end = VAD_ANALYSIS_SAMPLES; analysis_end <= total_samples; analysis_end += VAD_STEP_SAMPLES) {
		float* window = (float*)malloc(sizeof(float) * VAD_ANALYSIS_SAMPLES);
		struct whisper_vad_segments* segments;
		int segment_count;
		if (!window) return -1;
		memcpy(window, audio + analysis_end - VAD_ANALYSIS_SAMPLES, sizeof(float) * VAD_ANALYSIS_SAMPLES);
		segments = whisper_vad_segments_from_samples(vad_context, vad_params, window, VAD_ANALYSIS_SAMPLES);
		free(window);
		if (!segments) return -1;
		segment_count = whisper_vad_segments_n_segments(segments);
		for (segment_index = 0; segment_index < segment_count; segment_index++) {
			int64_t window_start = analysis_end - VAD_ANALYSIS_SAMPLES;
			int64_t speech_start = window_start + (int64_t)(whisper_vad_segments_get_segment_t0(segments, segment_index) * SAMPLE_RATE / 100.0f);
			int64_t speech_end = window_start + (int64_t)(whisper_vad_segments_get_segment_t1(segments, segment_index) * SAMPLE_RATE / 100.0f);
			int64_t safe_end = analysis_end - SAMPLE_RATE;
			int64_t inference_start;
			int inference_count;
			float* pcm_float;
			TRANSCRIPT_RESULT result;
			char hypothesis[TEXT_CAPACITY] = "";
			int sample_index;
			if (speech_end >= analysis_end) continue;
			if (speech_end > safe_end) continue;
			if (speech_end <= committed_sample) {
				skipped_overlap_segments++;
				continue;
			}
			if (speech_start < committed_sample) speech_start = committed_sample;
			if (speech_start < window_start) speech_start = window_start;
			inference_start = committed_sample > CONTEXT_SAMPLES ? committed_sample - CONTEXT_SAMPLES : 0;
			if (inference_start < window_start) inference_start = window_start;
			if (speech_end - inference_start > MAX_WINDOW_SAMPLES) inference_start = speech_end - MAX_WINDOW_SAMPLES;
			inference_count = (int)(speech_end - inference_start);
			pcm_float = (float*)malloc(sizeof(float) * (size_t)inference_count);
			if (!pcm_float) {
				free(pcm_float);
				whisper_vad_free_segments(segments);
				return -1;
			}
			for (sample_index = 0; sample_index < inference_count; sample_index++) {
				pcm_float[sample_index] = audio[inference_start + sample_index];
			}
			if (decode_audio_window(context, pcm_float, inference_start, inference_count, &result) != 0) {
				free(pcm_float);
				whisper_vad_free_segments(segments);
				return -1;
			}
			free(pcm_float);
			flatten_transcript(&result, hypothesis, sizeof(hypothesis));
			free_transcript_result(&result);
			if (safe_end > committed_sample) committed_sample = safe_end;
			committed_segments++;
			if (transcript_length > 0 && transcript_length < sizeof(final_transcript) - 1) {
				final_transcript[transcript_length++] = '\n';
				final_transcript[transcript_length] = '\0';
			}
			if (strlen(hypothesis) >= sizeof(final_transcript) - transcript_length) {
				whisper_vad_free_segments(segments);
				fprintf(stderr, "VAD baseline transcript exceeded output capacity\n");
				return -1;
			}
			memcpy(final_transcript + transcript_length, hypothesis, strlen(hypothesis) + 1);
			transcript_length += strlen(hypothesis);
			printf("VAD_WATERMARK_FINAL index=%d window_end_ms=%lld safe_end_ms=%lld text=%s\n",
				committed_segments - 1, (long long)analysis_end * 1000 / SAMPLE_RATE,
				(long long)safe_end * 1000 / SAMPLE_RATE, hypothesis);
		}
		whisper_vad_free_segments(segments);
	}
	printf("VAD_WATERMARK_RESULT committed_segments=%d skipped_overlap_segments=%d committed_ms=%lld transcript=%s\n",
		committed_segments, skipped_overlap_segments, (long long)committed_sample * 1000 / SAMPLE_RATE, final_transcript);
	return 0;
}

static int read_wav_pcm16_mono(const char* path, int16_t** samples_out, int* sample_count_out)
{
	FILE* file = NULL;
	unsigned char header[12];
	unsigned char chunk_header[8];
	unsigned char format_data[40];
	uint16_t audio_format = 0;
	uint16_t channels = 0;
	uint16_t bits_per_sample = 0;
	uint32_t sample_rate = 0;
	unsigned char* pcm_bytes = NULL;
	uint32_t pcm_size = 0;
	int have_format = 0;
	int have_data = 0;
	int16_t* samples = NULL;
	uint32_t i;
	int result = -1;

	file = fopen(path, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open WAV '%s': %s\n", path, strerror(errno));
		return -1;
	}
	if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
		memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
		fprintf(stderr, "Invalid RIFF/WAVE file: %s\n", path);
		goto cleanup;
	}

	while (fread(chunk_header, 1, sizeof(chunk_header), file) == sizeof(chunk_header)) {
		uint32_t chunk_size = read_u32_le(chunk_header + 4);
		long chunk_start = ftell(file);
		if (chunk_start < 0) goto cleanup;

		if (memcmp(chunk_header, "fmt ", 4) == 0) {
			size_t read_size = chunk_size < sizeof(format_data) ? chunk_size : sizeof(format_data);
			if (read_size < 16 || fread(format_data, 1, read_size, file) != read_size) {
				fprintf(stderr, "Invalid WAV format chunk: %s\n", path);
				goto cleanup;
			}
			audio_format = read_u16_le(format_data);
			channels = read_u16_le(format_data + 2);
			sample_rate = read_u32_le(format_data + 4);
			bits_per_sample = read_u16_le(format_data + 14);
			have_format = 1;
		} else if (memcmp(chunk_header, "data", 4) == 0) {
			pcm_bytes = (unsigned char*)malloc(chunk_size);
			if (!pcm_bytes || fread(pcm_bytes, 1, chunk_size, file) != chunk_size) {
				fprintf(stderr, "Failed to read WAV data chunk: %s\n", path);
				goto cleanup;
			}
			pcm_size = chunk_size;
			have_data = 1;
			break;
		}

		if (fseek(file, chunk_start + chunk_size + (chunk_size & 1u), SEEK_SET) != 0) {
			fprintf(stderr, "Invalid WAV chunk length: %s\n", path);
			goto cleanup;
		}
	}

	if (!have_format || !have_data || audio_format != 1 || channels != 1 ||
		sample_rate != SAMPLE_RATE || bits_per_sample != 16 || (pcm_size & 1u) != 0) {
		fprintf(stderr, "WAV must be PCM16, mono, %d Hz: %s\n", SAMPLE_RATE, path);
		goto cleanup;
	}
	if (pcm_size / 2u > 0x7fffffffu) {
		fprintf(stderr, "WAV is too large: %s\n", path);
		goto cleanup;
	}
	samples = (int16_t*)malloc((size_t)pcm_size);
	if (!samples) {
		fprintf(stderr, "Failed to allocate WAV samples\n");
		goto cleanup;
	}
	for (i = 0; i < pcm_size / 2u; i++) {
		samples[i] = (int16_t)read_u16_le(pcm_bytes + i * 2u);
	}
	*samples_out = samples;
	*sample_count_out = (int)(pcm_size / 2u);
	samples = NULL;
	result = 0;

cleanup:
	free(samples);
	free(pcm_bytes);
	if (file) fclose(file);
	return result;
}

static void print_token_timestamps(struct whisper_context* context, int segment_index)
{
	int token_count = whisper_full_n_tokens(context, segment_index);
	int token_index;
	int valid_dtw = 0;
	printf("TOKEN_TIMESTAMPS segment=%d tokens=%d\n", segment_index, token_count);
	for (token_index = 0; token_index < token_count; token_index++) {
		whisper_token_data token = whisper_full_get_token_data(context, segment_index, token_index);
		const char* text = whisper_full_get_token_text(context, segment_index, token_index);
		if (token.t_dtw >= 0) valid_dtw++;
		printf("  token=%d t0=%lld t1=%lld t_dtw=%lld text=%s\n",
			token_index, (long long)token.t0, (long long)token.t1,
			(long long)token.t_dtw, text ? text : "");
	}
	printf("TOKEN_TIMESTAMPS_SUMMARY segment=%d valid_dtw=%d/%d\n",
		segment_index, valid_dtw, token_count);
}

int main(int argc, char** argv)
{
	const char* wav_path;
	const char* model_path = "../../stt/models/ggml-large-v3.bin";
	const char* vad_model_path = "../../stt/models/ggml-silero-v5.1.2.bin";
	int16_t* samples = NULL;
	int sample_count = 0;
	float* audio = NULL;
	struct whisper_context_params context_params;
	struct whisper_vad_context_params vad_context_params;
	struct whisper_vad_params vad_params;
	struct whisper_context* context = NULL;
	struct whisper_vad_context* vad_context = NULL;
	struct whisper_vad_segments* vad_segments = NULL;
	int vad_segment_count;
	int segment_index;
	int exit_code = 1;

	int watermark_mode = 0;
	int argument_index = 1;
	if (argument_index < argc && strcmp(argv[argument_index], "--watermark") == 0) {
		watermark_mode = 1;
		argument_index++;
	}
	if (argc - argument_index < 1 || argc - argument_index > 3) {
		fprintf(stderr, "Usage: %s [--watermark] <recording.wav> [whisper-model] [vad-model]\n", argv[0]);
		return 2;
	}
	wav_path = argv[argument_index++];
	if (argument_index < argc) model_path = argv[argument_index++];
	if (argument_index < argc) vad_model_path = argv[argument_index];

	if (read_wav_pcm16_mono(wav_path, &samples, &sample_count) != 0) goto cleanup;
	if (sample_count <= 0) {
		fprintf(stderr, "WAV contains no samples: %s\n", wav_path);
		goto cleanup;
	}
	audio = (float*)malloc(sizeof(float) * (size_t)sample_count);
	if (!audio) {
		fprintf(stderr, "Failed to allocate float audio buffer\n");
		goto cleanup;
	}
	for (segment_index = 0; segment_index < sample_count; segment_index++) {
		audio[segment_index] = (float)samples[segment_index] / 32768.0f;
	}

	context_params = whisper_context_default_params();
	context_params.dtw_token_timestamps = true;
	context_params.dtw_aheads_preset = WHISPER_AHEADS_LARGE_V3;
	context = whisper_init_from_file_with_params(model_path, context_params);
	if (!context) {
		fprintf(stderr, "Failed to load Whisper model: %s\n", model_path);
		goto cleanup;
	}
	if (watermark_mode) {
		if (run_watermark_simulation(context, audio, sample_count) != 0) goto cleanup;
	}
	vad_context_params = whisper_vad_default_context_params();
	vad_context_params.n_threads = 1;
	vad_context = whisper_vad_init_from_file_with_params(vad_model_path, vad_context_params);
	if (!vad_context) {
		fprintf(stderr, "Failed to load VAD model: %s\n", vad_model_path);
		goto cleanup;
	}
	if (watermark_mode) {
		if (run_vad_watermark_baseline(context, vad_context, audio, sample_count) != 0) goto cleanup;
		exit_code = 0;
		goto cleanup;
	}

	vad_params = whisper_vad_default_params();
	vad_params.threshold = 0.5f;
	vad_params.min_speech_duration_ms = 250;
	vad_params.min_silence_duration_ms = 500;
	vad_params.max_speech_duration_s = 5.0f;
	vad_params.speech_pad_ms = 150;
	vad_segments = whisper_vad_segments_from_samples(vad_context, vad_params, audio, sample_count);
	if (!vad_segments) {
		fprintf(stderr, "VAD segmentation failed\n");
		goto cleanup;
	}

	vad_segment_count = whisper_vad_segments_n_segments(vad_segments);
	printf("WAV path=%s samples=%d duration_ms=%lld vad_segments=%d\n", wav_path, sample_count,
		(long long)sample_count * 1000 / SAMPLE_RATE, vad_segment_count);
	for (segment_index = 0; segment_index < vad_segment_count; segment_index++) {
		int64_t speech_start = (int64_t)(whisper_vad_segments_get_segment_t0(vad_segments, segment_index) * SAMPLE_RATE / 100.0f);
		int64_t speech_end = (int64_t)(whisper_vad_segments_get_segment_t1(vad_segments, segment_index) * SAMPLE_RATE / 100.0f);
		int64_t inference_start;
		int64_t inference_end;
		int inference_samples;
		float* inference_audio;
		struct whisper_full_params params;
		int result;
		int decoded_segment;

		if (speech_start < 0) speech_start = 0;
		if (speech_end > sample_count) speech_end = sample_count;
		if (speech_end <= speech_start) continue;
		inference_start = speech_start > CONTEXT_SAMPLES ? speech_start - CONTEXT_SAMPLES : 0;
		inference_end = speech_end;
		if (inference_end - inference_start > MAX_WINDOW_SAMPLES) {
			inference_start = inference_end - MAX_WINDOW_SAMPLES;
		}
		inference_samples = (int)(inference_end - inference_start);
		inference_audio = audio + inference_start;

		printf("VAD_SEGMENT index=%d speech_start_sample=%lld speech_end_sample=%lld inference_start_sample=%lld inference_end_sample=%lld\n",
			segment_index, (long long)speech_start, (long long)speech_end,
			(long long)inference_start, (long long)inference_end);
		params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
		params.language = "ja";
		params.translate = false;
		params.print_progress = false;
		params.print_realtime = false;
		params.print_timestamps = false;
		params.no_context = true;
		params.single_segment = false;
		params.suppress_blank = true;
		params.suppress_nst = true;
		params.temperature = 0.0f;
		params.temperature_inc = 0.0f;
		params.logprob_thold = -0.80f;
		params.no_speech_thold = 0.60f;
		result = whisper_full(context, params, inference_audio, inference_samples);
		if (result != 0) {
			fprintf(stderr, "Whisper inference failed for VAD segment %d: %d\n", segment_index, result);
			continue;
		}
		for (decoded_segment = 0; decoded_segment < whisper_full_n_segments(context); decoded_segment++) {
			int64_t t0 = inference_start + whisper_full_get_segment_t0(context, decoded_segment) * 160;
			int64_t t1 = inference_start + whisper_full_get_segment_t1(context, decoded_segment) * 160;
			printf("SEGMENT index=%d t0_sample=%lld t1_sample=%lld text=%s\n", decoded_segment,
				(long long)t0, (long long)t1, whisper_full_get_segment_text(context, decoded_segment));
			print_token_timestamps(context, decoded_segment);
		}
	}
	exit_code = 0;

cleanup:
	if (vad_segments) whisper_vad_free_segments(vad_segments);
	if (vad_context) whisper_vad_free(vad_context);
	if (context) whisper_free(context);
	free(audio);
	free(samples);
	return exit_code;
}