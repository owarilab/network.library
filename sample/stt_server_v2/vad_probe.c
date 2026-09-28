#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "whisper.h"

#define TARGET_SAMPLE_RATE 16000
#define ANALYSIS_SAMPLES (5 * TARGET_SAMPLE_RATE)

static uint16_t read_u16_le(const unsigned char* data)
{
	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const unsigned char* data)
{
	return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

int main(int argc, char** argv)
{
	FILE* file;
	unsigned char header[44];
	unsigned char* pcm_bytes = NULL;
	int16_t* pcm_i16 = NULL;
	float* pcm_f32 = NULL;
	long file_size;
	uint32_t data_size;
	uint16_t channels;
	uint16_t bits_per_sample;
	uint32_t sample_rate;
	size_t sample_count;
	size_t offset;
	struct whisper_vad_context_params context_params;
	struct whisper_vad_context* vad_context = NULL;
	struct whisper_vad_params vad_params;
	struct whisper_vad_segments* segments = NULL;
	int segment_index;
	int result = 1;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <silero-model> <input.wav>\n", argv[0]);
		return 2;
	}

	file = fopen(argv[2], "rb");
	if (!file) {
		perror("fopen wav");
		return 1;
	}
	if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
		memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0 ||
		memcmp(header + 12, "fmt ", 4) != 0 || memcmp(header + 36, "data", 4) != 0) {
		fprintf(stderr, "Expected a canonical PCM WAV with a 44-byte header\n");
		goto cleanup;
	}

	channels = read_u16_le(header + 22);
	sample_rate = read_u32_le(header + 24);
	bits_per_sample = read_u16_le(header + 34);
	data_size = read_u32_le(header + 40);
	if (read_u16_le(header + 20) != 1 || channels != 1 || sample_rate != TARGET_SAMPLE_RATE ||
		bits_per_sample != 16 || data_size % 2 != 0) {
		fprintf(stderr, "Expected mono 16-bit PCM at %d Hz\n", TARGET_SAMPLE_RATE);
		goto cleanup;
	}

	if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) < 44 ||
		(uint64_t)(file_size - 44) < data_size || fseek(file, 44, SEEK_SET) != 0) {
		fprintf(stderr, "Invalid WAV data length\n");
		goto cleanup;
	}
	pcm_bytes = (unsigned char*)malloc(data_size);
	if (!pcm_bytes || fread(pcm_bytes, 1, data_size, file) != data_size) {
		fprintf(stderr, "Unable to read WAV samples\n");
		goto cleanup;
	}
	sample_count = data_size / 2;
	pcm_i16 = (int16_t*)malloc(sample_count * sizeof(*pcm_i16));
	pcm_f32 = (float*)malloc(sample_count * sizeof(*pcm_f32));
	if (!pcm_i16 || !pcm_f32) {
		fprintf(stderr, "Unable to allocate PCM buffers\n");
		goto cleanup;
	}
	for (offset = 0; offset < sample_count; offset++) {
		uint16_t sample_bits = read_u16_le(pcm_bytes + offset * 2);
		pcm_i16[offset] = (int16_t)sample_bits;
		pcm_f32[offset] = (float)pcm_i16[offset] / 32768.0f;
	}

	context_params = whisper_vad_default_context_params();
	context_params.n_threads = 1;
	vad_context = whisper_vad_init_from_file_with_params(argv[1], context_params);
	if (!vad_context) {
		fprintf(stderr, "Unable to initialize Silero model: %s\n", argv[1]);
		goto cleanup;
	}

	vad_params = whisper_vad_default_params();
	vad_params.threshold = 0.5f;
	vad_params.min_speech_duration_ms = 250;
	vad_params.min_silence_duration_ms = 500;
	vad_params.max_speech_duration_s = (float)ANALYSIS_SAMPLES / TARGET_SAMPLE_RATE;
	vad_params.speech_pad_ms = 150;

	printf("input_samples=%zu sample_rate=%u window_samples=%d\n",
		sample_count, sample_rate, ANALYSIS_SAMPLES);
	for (offset = 0; offset < sample_count; offset += ANALYSIS_SAMPLES) {
		int window_samples = (int)(sample_count - offset);
		int segment_count;
		if (window_samples > ANALYSIS_SAMPLES) window_samples = ANALYSIS_SAMPLES;
		segments = whisper_vad_segments_from_samples(vad_context, vad_params,
			pcm_f32 + offset, window_samples);
		if (!segments) {
			fprintf(stderr, "VAD failed at sample %zu\n", offset);
			goto cleanup;
		}
		segment_count = whisper_vad_segments_n_segments(segments);
		for (segment_index = 0; segment_index < segment_count; segment_index++) {
			float t0 = whisper_vad_segments_get_segment_t0(segments, segment_index);
			float t1 = whisper_vad_segments_get_segment_t1(segments, segment_index);
			printf("window_start=%zu segment=%d t0=%0.6f t1=%0.6f "
				"seconds_samples=[%lld,%lld) centiseconds_samples=[%lld,%lld)\n",
				offset, segment_index, t0, t1,
				(long long)(offset + (size_t)(t0 * TARGET_SAMPLE_RATE)),
				(long long)(offset + (size_t)(t1 * TARGET_SAMPLE_RATE)),
				(long long)(offset + (size_t)(t0 * TARGET_SAMPLE_RATE / 100.0f)),
				(long long)(offset + (size_t)(t1 * TARGET_SAMPLE_RATE / 100.0f)));
		}
		whisper_vad_free_segments(segments);
		segments = NULL;
	}
	result = 0;

cleanup:
	if (segments) whisper_vad_free_segments(segments);
	if (vad_context) whisper_vad_free(vad_context);
	free(pcm_f32);
	free(pcm_i16);
	free(pcm_bytes);
	fclose(file);
	return result;
}