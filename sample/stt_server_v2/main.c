#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "qs_api.h"
#include "whisper.h"

#define STT_V2_PORT 8080
#define STT_V2_MAX_CONNECTIONS 16
#define STT_V2_SESSION_COUNT 16
#define STT_V2_SAMPLE_RATE 16000
#define STT_V2_MAX_BUFFER_SECONDS 30
#define STT_V2_MAX_BUFFER_SAMPLES (STT_V2_SAMPLE_RATE * STT_V2_MAX_BUFFER_SECONDS)
#define STT_V2_WINDOW_SAMPLES (STT_V2_SAMPLE_RATE * 5)
#define STT_V2_HOP_SAMPLES (STT_V2_SAMPLE_RATE / 2)
#define STT_V2_OVERLAP_SAMPLES STT_V2_HOP_SAMPLES
#define STT_V2_MAX_SEGMENT_SAMPLES (STT_V2_SAMPLE_RATE * 20)
#define STT_V2_MAX_RESULTS 128
#define STT_V2_RESULT_TEXT_CAPACITY 768
#define STT_V2_MODEL_PATH "../../stt/models/ggml-large-v3-turbo.bin"
#define STT_V2_VAD_MODEL_PATH "../../stt/models/ggml-silero-v5.1.2.bin"
#define STT_V2_MESSAGE_CAPACITY 2048
#define STT_V2_TRANSCRIPT_PATH_CAPACITY 256
#define STT_V2_PCM_PATH_CAPACITY 256

typedef struct {
	uint32_t connection_offset;
	uint32_t session_id;
	int active;
	int stopping;
	int done_pending;
	int done_sent;
	int cancelled;
	int worker_inflight;
	int window_inflight;
	uint64_t total_samples_received;
	uint64_t next_vad_end_sample;
	uint64_t last_vad_window_end_sample;
	uint64_t committed_end_sample;
	uint64_t submitted_end_sample;
	uint64_t flush_end_sample;
	uint64_t retain_start_sample;
	uint64_t pcm_base_sample;
	int16_t* pcm;
	size_t pcm_sample_count;
	size_t pcm_capacity;
	char worker_error[64];
	char transcript_path[STT_V2_TRANSCRIPT_PATH_CAPACITY];
	char pcm_file_path[STT_V2_PCM_PATH_CAPACITY];
	FILE* pcm_file;
	uint32_t wav_data_bytes;
} STT_V2_SESSION;

typedef struct {
	uint32_t connection_offset;
	uint32_t session_id;
	uint32_t result_id;
	uint64_t start_sample;
	uint64_t end_sample;
	char text[STT_V2_RESULT_TEXT_CAPACITY];
} STT_V2_RESULT;

static STT_V2_SESSION g_sessions[STT_V2_SESSION_COUNT];
static STT_V2_RESULT g_results[STT_V2_MAX_RESULTS];
static size_t g_result_head;
static size_t g_result_count;
static pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_worker_condition = PTHREAD_COND_INITIALIZER;
static pthread_t g_worker_thread;
static int g_worker_running;
static int g_worker_shutdown;
static int g_worker_busy;
static struct whisper_context* g_whisper_context;
static struct whisper_vad_context* g_vad_context;
static uint32_t g_next_session_id = 1;
static uint32_t g_next_result_id = 1;

static int create_session_file_paths(QS_MEMORY_CONTEXT* memory, char* transcript_path,
	size_t transcript_path_capacity, char* wav_path, size_t wav_path_capacity)
{
	time_t now = time(NULL);
	struct tm local_time;
	char timestamp[16];
	char* unique_id;
	FILE* file;
	if (now == (time_t)-1 || localtime_r(&now, &local_time) == NULL) return -1;
	if (strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", &local_time) == 0) return -1;
	unique_id = api_qs_uniqid(memory, 16);
	if (unique_id == NULL) return -1;
	if (snprintf(transcript_path, transcript_path_capacity,
		"transcripts/%s_%s.txt", timestamp, unique_id) >= (int)transcript_path_capacity ||
		snprintf(wav_path, wav_path_capacity,
			"transcripts/%s_%s.wav", timestamp, unique_id) >= (int)wav_path_capacity) return -1;
	file = fopen(transcript_path, "wx");
	if (file == NULL) return -1;
	if (fclose(file) != 0) {
		remove(transcript_path);
		return -1;
	}
	file = fopen(wav_path, "wx");
	if (file == NULL) {
		remove(transcript_path);
		return -1;
	}
	if (fclose(file) != 0) {
		remove(transcript_path);
		remove(wav_path);
		return -1;
	}
	return 0;
}

static void write_wav_u16(FILE* file, uint16_t value)
{
	uint8_t bytes[2] = { (uint8_t)value, (uint8_t)(value >> 8) };
	fwrite(bytes, 1, sizeof(bytes), file);
}

static void write_wav_u32(FILE* file, uint32_t value)
{
	uint8_t bytes[4] = {
		(uint8_t)value, (uint8_t)(value >> 8), (uint8_t)(value >> 16), (uint8_t)(value >> 24)
	};
	fwrite(bytes, 1, sizeof(bytes), file);
}

static int write_wav_header(FILE* file, uint32_t data_bytes)
{
	if (fseek(file, 0, SEEK_SET) != 0) return -1;
	if (fwrite("RIFF", 1, 4, file) != 4) return -1;
	write_wav_u32(file, 36 + data_bytes);
	if (ferror(file)) return -1;
	if (fwrite("WAVEfmt ", 1, 8, file) != 8) return -1;
	write_wav_u32(file, 16);
	write_wav_u16(file, 1);
	write_wav_u16(file, 1);
	write_wav_u32(file, STT_V2_SAMPLE_RATE);
	write_wav_u32(file, STT_V2_SAMPLE_RATE * 2);
	write_wav_u16(file, 2);
	write_wav_u16(file, 16);
	if (ferror(file)) return -1;
	if (fwrite("data", 1, 4, file) != 4) return -1;
	write_wav_u32(file, data_bytes);
	return ferror(file) ? -1 : 0;
}

static int open_session_wav(STT_V2_SESSION* session)
{
	FILE* file = fopen(session->pcm_file_path, "wb+");
	if (file == NULL) return -1;
	session->wav_data_bytes = 0;
	if (write_wav_header(file, 0) != 0 || fflush(file) != 0) {
		fclose(file);
		return -1;
	}
	session->pcm_file = file;
	return 0;
}

static int finalize_session_wav(STT_V2_SESSION* session)
{
	int status = 0;
	if (!session || !session->pcm_file) return 0;
	if (write_wav_header(session->pcm_file, session->wav_data_bytes) != 0 ||
		fflush(session->pcm_file) != 0) status = -1;
	if (fclose(session->pcm_file) != 0) status = -1;
	session->pcm_file = NULL;
	if (status != 0) fprintf(stderr, "Failed to finalize WAV file: %s\n", session->pcm_file_path);
	return status;
}

static int append_session_pcm(STT_V2_SESSION* session, const uint8_t* bytes, size_t byte_count)
{
	if (!session || !session->pcm_file || byte_count > UINT32_MAX - session->wav_data_bytes) return -1;
	if (fseek(session->pcm_file, 0, SEEK_END) != 0 ||
		fwrite(bytes, 1, byte_count, session->pcm_file) != byte_count) return -1;
	session->wav_data_bytes += (uint32_t)byte_count;
	return 0;
}

static void append_transcript(const STT_V2_RESULT* result)
{
	char path[STT_V2_TRANSCRIPT_PATH_CAPACITY];
	int found = 0;
	size_t index;
	FILE* file;
	pthread_mutex_lock(&g_state_mutex);
	for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
		if (g_sessions[index].active && g_sessions[index].session_id == result->session_id) {
			snprintf(path, sizeof(path), "%s", g_sessions[index].transcript_path);
			found = path[0] != '\0';
			break;
		}
	}
	pthread_mutex_unlock(&g_state_mutex);
	if (!found) return;
	file = fopen(path, "a");
	if (file == NULL) {
		fprintf(stderr, "Failed to append transcript: %s\n", path);
		return;
	}
	{
		int write_failed = fprintf(file, "%s\n", result->text) < 0;
		if (fclose(file) != 0) write_failed = 1;
		if (write_failed)
		fprintf(stderr, "Failed to write transcript: %s\n", path);
	}
}

static int create_transcript_directory(void)
{
	struct stat directory_info;
	if (stat("transcripts", &directory_info) == 0) return S_ISDIR(directory_info.st_mode) ? 0 : -1;
	if (errno != ENOENT || mkdir("transcripts", 0755) != 0) return -1;
	return 0;
}

static uint16_t read_pcm_sample_le(const uint8_t* bytes)
{
	return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static void clear_session(STT_V2_SESSION* session)
{
	if (!session) return;
	finalize_session_wav(session);
	free(session->pcm);
	memset(session, 0, sizeof(*session));
}

static void discard_pcm_before_locked(STT_V2_SESSION* session, uint64_t end_sample)
{
	size_t discard;
	if (end_sample <= session->pcm_base_sample) return;
	discard = end_sample - session->pcm_base_sample > (uint64_t)SIZE_MAX ? session->pcm_sample_count :
		(size_t)(end_sample - session->pcm_base_sample);
	if (discard > session->pcm_sample_count) discard = session->pcm_sample_count;
	memmove(session->pcm, session->pcm + discard,
		(session->pcm_sample_count - discard) * sizeof(*session->pcm));
	session->pcm_sample_count -= discard;
	session->pcm_base_sample += discard;
}

static void fail_session_locked(STT_V2_SESSION* session, const char* error_code)
{
	if (!session || !session->active) return;
	if (session->worker_error[0] == '\0')
		snprintf(session->worker_error, sizeof(session->worker_error), "%s", error_code);
	session->stopping = 1;
	session->flush_end_sample = session->total_samples_received;
	session->retain_start_sample = session->flush_end_sample;
	pthread_cond_signal(&g_worker_condition);
}

static STT_V2_SESSION* find_session_locked(uint32_t connection_offset)
{
	size_t index;
	for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
		if (g_sessions[index].active && g_sessions[index].connection_offset == connection_offset) {
			return &g_sessions[index];
		}
	}
	return NULL;
}

static STT_V2_SESSION* find_session_by_id_locked(uint32_t session_id)
{
	size_t index;
	for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
		if (g_sessions[index].active && g_sessions[index].session_id == session_id) return &g_sessions[index];
	}
	return NULL;
}

static STT_V2_SESSION* create_session(uint32_t connection_offset)
{
	size_t index;
	STT_V2_SESSION* session;
	session = find_session_locked(connection_offset);
	if (session) return session;
	for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
		if (!g_sessions[index].active) {
			session = &g_sessions[index];
			memset(session, 0, sizeof(*session));
			session->active = 1;
			session->connection_offset = connection_offset;
			session->session_id = g_next_session_id++;
			if (g_next_session_id == 0) g_next_session_id = 1;
			pthread_cond_signal(&g_worker_condition);
			return session;
		}
	}
	return NULL;
}

static int on_connect(QS_EVENT_PARAMETER params)
{
	(void)params;
	return 0;
}

static int on_ws_event(QS_EVENT_PARAMETER params)
{
	uint8_t opcode = api_qs_get_ws_opcode(params);
	ssize_t message_size = api_qs_get_ws_message_size(params);
	uint32_t connection_offset = api_qs_get_connection_offset(params);
	STT_V2_SESSION* session;
	char* message = api_qs_get_ws_message(params);
	if (opcode == 1 && message && message_size > 0) {
		QS_MEMORY_CONTEXT json_memory = { 0 };
		QS_JSON_ELEMENT_OBJECT object;
		char* type;
		if (api_qs_memory_alloc(&json_memory, 65536) != 0) return 0;
		if (api_qs_json_decode_object(&json_memory, &object, message) != 0) {
			api_qs_memory_free(&json_memory);
			return 0;
		}
		type = api_qs_object_get_string(&object, "type");
		if (type && strcmp(type, "stt_init") == 0) {
			int sample_rate = api_qs_object_get_integer_val(&object, "sample_rate");
			int channels = api_qs_object_get_integer_val(&object, "channels");
			int bits_per_sample = api_qs_object_get_integer_val(&object, "bits_per_sample");
			char transcript_path[STT_V2_TRANSCRIPT_PATH_CAPACITY] = "";
			char wav_path[STT_V2_PCM_PATH_CAPACITY] = "";
			int created_session = 0;
			pthread_mutex_lock(&g_state_mutex);
			session = sample_rate == STT_V2_SAMPLE_RATE && channels == 1 && bits_per_sample == 16 ?
				find_session_locked(connection_offset) : NULL;
			if (!session && sample_rate == STT_V2_SAMPLE_RATE && channels == 1 && bits_per_sample == 16) {
				session = create_session(connection_offset);
				created_session = session != NULL;
			}
			if (session) {
				if (session->stopping || session->done_sent) {
					api_qs_send_ws_message_plane(params,
						"{\"type\":\"stt_error\",\"code\":\"session_already_stopping\"}");
					pthread_mutex_unlock(&g_state_mutex);
					api_qs_memory_free(&json_memory);
					return 0;
				}
				if (session->transcript_path[0] == '\0') {
					if (create_session_file_paths(&json_memory, transcript_path, sizeof(transcript_path),
						wav_path, sizeof(wav_path)) != 0) {
						if (created_session) clear_session(session);
						api_qs_send_ws_message_plane(params,
							"{\"type\":\"stt_error\",\"code\":\"transcript_file_create_failed\"}");
						pthread_mutex_unlock(&g_state_mutex);
						api_qs_memory_free(&json_memory);
						return 0;
					}
					snprintf(session->pcm_file_path, sizeof(session->pcm_file_path), "%s", wav_path);
					if (open_session_wav(session) != 0) {
						remove(transcript_path);
						remove(wav_path);
						if (created_session) clear_session(session);
						api_qs_send_ws_message_plane(params,
							"{\"type\":\"stt_error\",\"code\":\"wav_file_create_failed\"}");
						pthread_mutex_unlock(&g_state_mutex);
						api_qs_memory_free(&json_memory);
						return 0;
					}
					snprintf(session->transcript_path, sizeof(session->transcript_path), "%s", transcript_path);
					printf("Transcript file: %s\nWAV file: %s\n",
						session->transcript_path, session->pcm_file_path);
				}
				char response[STT_V2_MESSAGE_CAPACITY];
				snprintf(response, sizeof(response), "{\"type\":\"stt_ready\",\"session_id\":%u}",
					session->session_id);
				api_qs_send_ws_message_plane(params, response);
			} else {
				api_qs_send_ws_message_plane(params,
					"{\"type\":\"stt_error\",\"code\":\"invalid_audio_format_or_session_limit\"}");
			}
			pthread_mutex_unlock(&g_state_mutex);
		} else if (type && strcmp(type, "stt_stop") == 0) {
			pthread_mutex_lock(&g_state_mutex);
			session = find_session_locked(connection_offset);
			if (session) {
				session->stopping = 1;
				if (session->flush_end_sample == 0) session->flush_end_sample = session->total_samples_received;
				pthread_cond_signal(&g_worker_condition);
			}
			pthread_mutex_unlock(&g_state_mutex);
		}
		api_qs_memory_free(&json_memory);
		return 0;
	}

	pthread_mutex_lock(&g_state_mutex);
	session = find_session_locked(connection_offset);

	if (opcode == 2 && session && message && message_size > 0) {
		size_t sample_index;
		int16_t* pcm;
		if ((message_size & 1) != 0) {
			api_qs_send_ws_message_plane(params,
				"{\"type\":\"stt_error\",\"code\":\"invalid_pcm_frame_size\"}");
			pthread_mutex_unlock(&g_state_mutex);
			return 0;
		}
		if (session->stopping) {
			api_qs_send_ws_message_plane(params,
				"{\"type\":\"stt_error\",\"code\":\"session_stopping\"}");
			pthread_mutex_unlock(&g_state_mutex);
			return 0;
		}
		if ((size_t)message_size / 2 > STT_V2_MAX_BUFFER_SAMPLES - session->pcm_sample_count) {
			fail_session_locked(session, "audio_buffer_limit");
			api_qs_send_ws_message_plane(params,
				"{\"type\":\"stt_error\",\"code\":\"audio_buffer_limit\"}");
			pthread_mutex_unlock(&g_state_mutex);
			return 0;
		}
		if (session->pcm_base_sample + session->pcm_sample_count != session->total_samples_received) {
			api_qs_send_ws_message_plane(params,
				"{\"type\":\"stt_error\",\"code\":\"audio_buffer_state_invalid\"}");
			pthread_mutex_unlock(&g_state_mutex);
			return 0;
		}
		if (session->pcm_sample_count + (size_t)message_size / 2 > session->pcm_capacity) {
			size_t capacity = session->pcm_capacity ? session->pcm_capacity : STT_V2_WINDOW_SAMPLES * 2;
			while (capacity < session->pcm_sample_count + (size_t)message_size / 2) capacity *= 2;
			if (capacity > STT_V2_MAX_BUFFER_SAMPLES) capacity = STT_V2_MAX_BUFFER_SAMPLES;
			pcm = (int16_t*)realloc(session->pcm, capacity * sizeof(*pcm));
			if (!pcm) {
				api_qs_send_ws_message_plane(params,
					"{\"type\":\"stt_error\",\"code\":\"audio_buffer_allocation_failed\"}");
				pthread_mutex_unlock(&g_state_mutex);
				return 0;
			}
			session->pcm = pcm;
			session->pcm_capacity = capacity;
		}
		for (sample_index = 0; sample_index < (size_t)message_size / 2; sample_index++) {
			uint16_t bits = read_pcm_sample_le((const uint8_t*)message + sample_index * 2);
			session->pcm[session->pcm_sample_count + sample_index] = (int16_t)bits;
		}
		if (append_session_pcm(session, (const uint8_t*)message, (size_t)message_size) != 0) {
			fail_session_locked(session, "wav_write_failed");
			api_qs_send_ws_message_plane(params,
				"{\"type\":\"stt_error\",\"code\":\"wav_write_failed\"}");
			pthread_mutex_unlock(&g_state_mutex);
			return 0;
		}
		session->pcm_sample_count += (size_t)message_size / 2;
		session->total_samples_received += (uint64_t)message_size / 2;
		pthread_cond_signal(&g_worker_condition);
		pthread_mutex_unlock(&g_state_mutex);
		return 0;
	}
	pthread_mutex_unlock(&g_state_mutex);
	return 0;
}

static int on_close(QS_EVENT_PARAMETER params)
{
	uint32_t connection_offset = api_qs_get_connection_offset(params);
	STT_V2_SESSION* session;
	pthread_mutex_lock(&g_state_mutex);
	session = find_session_locked(connection_offset);
	if (session) {
		session->stopping = 1;
		session->cancelled = 1;
		session->connection_offset = UINT32_MAX;
		pthread_cond_signal(&g_worker_condition);
	}
	pthread_mutex_unlock(&g_state_mutex);
	return 0;
}

static int enqueue_result_locked(const STT_V2_SESSION* session, uint64_t start_sample,
	uint64_t end_sample, const char* text)
{
	STT_V2_RESULT* result;
	size_t tail;
	if (g_result_count >= STT_V2_MAX_RESULTS) return -1;
	tail = (g_result_head + g_result_count) % STT_V2_MAX_RESULTS;
	result = &g_results[tail];
	memset(result, 0, sizeof(*result));
	result->connection_offset = session->connection_offset;
	result->session_id = session->session_id;
	result->result_id = g_next_result_id++;
	if (g_next_result_id == 0) g_next_result_id = 1;
	result->start_sample = start_sample;
	result->end_sample = end_sample;
	snprintf(result->text, sizeof(result->text), "%s", text ? text : "");
	g_result_count++;
	return 0;
}

static int has_queued_results_locked(uint32_t connection_offset, uint32_t session_id)
{
	size_t index;
	for (index = 0; index < g_result_count; index++) {
		const STT_V2_RESULT* result = &g_results[(g_result_head + index) % STT_V2_MAX_RESULTS];
		if (result->connection_offset == connection_offset && result->session_id == session_id) return 1;
	}
	return 0;
}

static int get_vad_window_locked(STT_V2_SESSION* session, int16_t* pcm,
	uint64_t* window_start, int* sample_count, int* flush_tail)
{
	uint64_t available_end = session->total_samples_received;
	uint64_t available_samples = available_end - session->retain_start_sample;
	size_t source_offset;
		if (session->stopping || !session->active) {
		if (session->retain_start_sample >= session->flush_end_sample) return 0;
		*flush_tail = 1;
		available_samples = session->flush_end_sample - session->retain_start_sample;
		*sample_count = (int)(available_samples > STT_V2_WINDOW_SAMPLES ? STT_V2_WINDOW_SAMPLES : available_samples);
		*window_start = session->retain_start_sample;
	} else {
		*flush_tail = 0;
		if (available_end < STT_V2_WINDOW_SAMPLES) {
			if (available_end < STT_V2_HOP_SAMPLES ||
				available_end - session->next_vad_end_sample < STT_V2_HOP_SAMPLES) return 0;
			*sample_count = (int)available_end;
			*window_start = 0;
		} else {
			if (available_end - session->next_vad_end_sample < STT_V2_HOP_SAMPLES) return 0;
			*sample_count = STT_V2_WINDOW_SAMPLES;
			*window_start = available_end - STT_V2_WINDOW_SAMPLES;
		}
	}
	if (*window_start < session->pcm_base_sample) return 0;
	source_offset = (size_t)(*window_start - session->pcm_base_sample);
	if (source_offset + (size_t)*sample_count > session->pcm_sample_count) return 0;
	memcpy(pcm, session->pcm + source_offset, (size_t)*sample_count * sizeof(*pcm));
	if (!*flush_tail) {
		session->next_vad_end_sample = available_end;
		session->last_vad_window_end_sample = *window_start + (uint64_t)*sample_count;
	}
	return 1;
}

static void process_vad_window(uint32_t session_id,
	uint64_t window_start, int sample_count, int flush_tail, const int16_t* pcm)
{
	float* audio = (float*)malloc((size_t)sample_count * sizeof(*audio));
	struct whisper_vad_params params = whisper_vad_default_params();
	struct whisper_vad_segments* segments;
	int segment_count = 0;
	int segment_index;
	if (!audio) {
		pthread_mutex_lock(&g_state_mutex);
		{
			STT_V2_SESSION* session = find_session_by_id_locked(session_id);
			if (session && session->session_id == session_id) {
				snprintf(session->worker_error, sizeof(session->worker_error), "%s", "vad_allocation_failed");
				session->stopping = 1;
				session->flush_end_sample = session->total_samples_received;
				session->retain_start_sample = session->flush_end_sample;
			}
		}
		pthread_mutex_unlock(&g_state_mutex);
		return;
	}
	for (segment_index = 0; segment_index < sample_count; segment_index++) {
		audio[segment_index] = (float)pcm[segment_index] / 32768.0f;
	}
	params.threshold = 0.5f;
	params.min_speech_duration_ms = 250;
	params.min_silence_duration_ms = 500;
	params.max_speech_duration_s = 20.0f;
	params.speech_pad_ms = 150;
	segments = whisper_vad_segments_from_samples(g_vad_context, params, audio, sample_count);
	free(audio);
	if (!segments) {
		pthread_mutex_lock(&g_state_mutex);
		{
			STT_V2_SESSION* session = find_session_by_id_locked(session_id);
			if (session && session->session_id == session_id) {
				snprintf(session->worker_error, sizeof(session->worker_error), "%s", "vad_failed");
				session->stopping = 1;
				session->flush_end_sample = session->total_samples_received;
				session->retain_start_sample = session->flush_end_sample;
			}
		}
		pthread_mutex_unlock(&g_state_mutex);
		return;
	}
	segment_count = whisper_vad_segments_n_segments(segments);
	for (segment_index = 0; segment_index < segment_count; segment_index++) {
		float raw_start = whisper_vad_segments_get_segment_t0(segments, segment_index);
		float raw_end = whisper_vad_segments_get_segment_t1(segments, segment_index);
		uint64_t start_sample = window_start + (uint64_t)(raw_start * STT_V2_SAMPLE_RATE / 100.0f);
		uint64_t end_sample = window_start + (uint64_t)(raw_end * STT_V2_SAMPLE_RATE / 100.0f);
		uint64_t window_end = window_start + (uint64_t)sample_count;
		int is_last = segment_index == segment_count - 1;
		if (!flush_tail && is_last && raw_end >=
			(float)sample_count / (STT_V2_SAMPLE_RATE / 100.0f)) {
			pthread_mutex_lock(&g_state_mutex);
			{
				STT_V2_SESSION* session = find_session_by_id_locked(session_id);
					if (session && session->session_id == session_id && !session->cancelled) {
						uint64_t open_start = window_start + (uint64_t)(raw_start * STT_V2_SAMPLE_RATE / 100.0f);
						uint64_t discard_before = open_start > STT_V2_OVERLAP_SAMPLES ?
							open_start - STT_V2_OVERLAP_SAMPLES : 0;
						if (discard_before > session->retain_start_sample)
							session->retain_start_sample = discard_before;
				}
			}
			pthread_mutex_unlock(&g_state_mutex);
			continue;
		}
		if (end_sample > window_end) end_sample = window_end;
		if (start_sample < window_start) start_sample = window_start;
		pthread_mutex_lock(&g_state_mutex);
		{
			STT_V2_SESSION* session = find_session_by_id_locked(session_id);
			if (session && session->session_id == session_id && !session->cancelled &&
				end_sample > session->submitted_end_sample) {
				if (start_sample < session->submitted_end_sample) start_sample = session->submitted_end_sample;
				if (end_sample > start_sample) {
					size_t pcm_offset = (size_t)(start_sample - session->pcm_base_sample);
					size_t segment_samples = (size_t)(end_sample - start_sample);
					float* segment_audio = segment_samples <= STT_V2_MAX_SEGMENT_SAMPLES ?
						(float*)malloc(segment_samples * sizeof(*segment_audio)) : NULL;
					if (segment_audio && start_sample >= session->pcm_base_sample &&
						pcm_offset <= session->pcm_sample_count &&
						segment_samples <= session->pcm_sample_count - pcm_offset) {
						size_t sample;
						char text[STT_V2_MESSAGE_CAPACITY] = "";
						for (sample = 0; sample < segment_samples; sample++)
							segment_audio[sample] = (float)session->pcm[pcm_offset + sample] / 32768.0f;
						pthread_mutex_unlock(&g_state_mutex);
						{
							struct whisper_full_params full_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
							int status;
							full_params.language = "ja";
							full_params.translate = false;
							full_params.no_context = true;
							full_params.print_progress = false;
							full_params.print_realtime = false;
							full_params.print_timestamps = false;
							status = whisper_full(g_whisper_context, full_params, segment_audio, (int)segment_samples);
							if (status == 0) {
								int text_index;
								size_t used = 0;
								for (text_index = 0; text_index < whisper_full_n_segments(g_whisper_context); text_index++) {
									const char* part = whisper_full_get_segment_text(g_whisper_context, text_index);
									size_t length = strlen(part);
									if (length > sizeof(text) - used - 1) length = sizeof(text) - used - 1;
									memcpy(text + used, part, length);
									used += length;
									text[used] = '\0';
								}
							}
						}
						pthread_mutex_lock(&g_state_mutex);
						session = find_session_by_id_locked(session_id);
						if (session && session->session_id == session_id && !session->cancelled) {
							if (text[0] && enqueue_result_locked(session, start_sample, end_sample, text) == 0) {
								session->submitted_end_sample = end_sample;
								session->committed_end_sample = end_sample;
							} else if (text[0]) {
								fail_session_locked(session, "result_queue_full");
							}
						}
							free(segment_audio);
						} else {
							free(segment_audio);
							fail_session_locked(session, "transcription_buffer_failed");
						}
				} else {
						fail_session_locked(session, "transcription_buffer_failed");
				}
			}
			if (session && session->session_id == session_id && !session->cancelled && flush_tail) {
				session->retain_start_sample = session->flush_end_sample;
			}
		}
		pthread_mutex_unlock(&g_state_mutex);
	}
	whisper_vad_free_segments(segments);
	pthread_mutex_lock(&g_state_mutex);
	{
		STT_V2_SESSION* session = find_session_by_id_locked(session_id);
		if (session && session->session_id == session_id && !session->cancelled) {
			uint64_t safe_end = session->submitted_end_sample;
			uint64_t overlap_start = session->total_samples_received > STT_V2_OVERLAP_SAMPLES ?
				session->total_samples_received - STT_V2_OVERLAP_SAMPLES : 0;
			if (!session->stopping && overlap_start < safe_end) safe_end = overlap_start;
			if (session->stopping && session->retain_start_sample >= session->flush_end_sample)
				safe_end = session->flush_end_sample;
			discard_pcm_before_locked(session, safe_end);
		}
	}
	pthread_mutex_unlock(&g_state_mutex);
}

static void complete_stopping_sessions_locked(void)
{
	size_t index;
	for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
		STT_V2_SESSION* session = &g_sessions[index];
		if (session->active && session->stopping && !session->worker_inflight &&
			!session->window_inflight && !session->done_sent &&
			(session->cancelled || session->flush_end_sample == 0 ||
			session->retain_start_sample >= session->flush_end_sample) &&
			!has_queued_results_locked(session->connection_offset, session->session_id)) {
			session->done_pending = 1;
			if (session->connection_offset == UINT32_MAX) clear_session(session);
		}
	}
}

static int session_has_work_locked(const STT_V2_SESSION* session)
{
	if (!session->active) return 0;
	if (session->stopping)
		return session->cancelled || session->retain_start_sample < session->flush_end_sample;
	return session->total_samples_received >= STT_V2_HOP_SAMPLES &&
		session->total_samples_received - session->next_vad_end_sample >= STT_V2_HOP_SAMPLES;
}

static void* stt_worker_main(void* unused)
{
	(void)unused;
	for (;;) {
		int did_work = 0;
		size_t index;
		pthread_mutex_lock(&g_state_mutex);
		while (!g_worker_shutdown) {
			int has_input = 0;
			for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
				STT_V2_SESSION* session = &g_sessions[index];
				if (session_has_work_locked(session)) {
					has_input = 1;
					break;
				}
			}
			if (has_input) break;
			pthread_cond_wait(&g_worker_condition, &g_state_mutex);
		}
		if (g_worker_shutdown) {
			pthread_mutex_unlock(&g_state_mutex);
			break;
		}
		g_worker_busy = 1;
		for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
			STT_V2_SESSION* session = &g_sessions[index];
			int16_t window[STT_V2_WINDOW_SAMPLES];
			uint64_t window_start = 0;
			int sample_count = 0;
			int flush_tail = 0;
			uint32_t session_id;
			if (!session->active || !get_vad_window_locked(session, window, &window_start, &sample_count, &flush_tail)) continue;
			session_id = session->session_id;
			session->worker_inflight = 1;
			session->window_inflight = 1;
			pthread_mutex_unlock(&g_state_mutex);
			process_vad_window(session_id, window_start, sample_count, flush_tail, window);
			pthread_mutex_lock(&g_state_mutex);
			{
				STT_V2_SESSION* completed = find_session_by_id_locked(session_id);
				if (completed && completed->session_id == session_id) {
					completed->worker_inflight = 0;
					completed->window_inflight = 0;
					if (completed->cancelled) clear_session(completed);
					else if (completed->stopping &&
						completed->last_vad_window_end_sample == completed->flush_end_sample)
						completed->retain_start_sample = completed->flush_end_sample;
				}
			}
			did_work = 1;
			(void)flush_tail;
		}
		complete_stopping_sessions_locked();
		g_worker_busy = 0;
		pthread_cond_broadcast(&g_worker_condition);
		pthread_mutex_unlock(&g_state_mutex);
		if (!did_work) continue;
	}
	return NULL;
}

static void drain_pending_results(QS_SERVER_CONTEXT* server)
{
	for (;;) {
		STT_V2_RESULT result;
		int found = 0;
		pthread_mutex_lock(&g_state_mutex);
		if (g_result_count > 0) {
			result = g_results[g_result_head];
			g_result_head = (g_result_head + 1) % STT_V2_MAX_RESULTS;
			g_result_count--;
			found = 1;
		}
		pthread_mutex_unlock(&g_state_mutex);
		if (!found) break;
		{
			int session_is_live = 0;
			size_t index;
			pthread_mutex_lock(&g_state_mutex);
			for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
				if (g_sessions[index].active && !g_sessions[index].cancelled &&
					g_sessions[index].connection_offset == result.connection_offset &&
					g_sessions[index].session_id == result.session_id) {
					session_is_live = 1;
					break;
				}
			}
			pthread_mutex_unlock(&g_state_mutex);
			if (!session_is_live) continue;
		}
			append_transcript(&result);
		{
			char json[STT_V2_RESULT_TEXT_CAPACITY * 2 + 256];
			char escaped[STT_V2_RESULT_TEXT_CAPACITY * 2];
			size_t in, out = 0;
			for (in = 0; result.text[in] && out + 2 < sizeof(escaped); in++) {
				unsigned char ch = (unsigned char)result.text[in];
				if (ch == '"' || ch == '\\') escaped[out++] = '\\';
				if (ch >= 0x20) escaped[out++] = (char)ch;
			}
			escaped[out] = '\0';
			snprintf(json, sizeof(json),
				"{\"type\":\"stt_final\",\"session_id\":%u,\"result_id\":%u,"
				"\"start_sample\":%llu,\"end_sample\":%llu,\"text\":\"%s\"}",
				result.session_id,
				result.result_id,
				(unsigned long long)result.start_sample,
				(unsigned long long)result.end_sample,
				escaped);
			api_qs_send_ws_text_by_connection_offset(server, result.connection_offset, json);
		}
	}
		for (;;) {
			uint32_t connection_offset = 0;
			uint32_t session_id = 0;
			char error_code[64] = "";
			int found = 0;
			size_t index;
			pthread_mutex_lock(&g_state_mutex);
			for (index = 0; index < STT_V2_SESSION_COUNT; index++) {
				if (g_sessions[index].active && g_sessions[index].done_pending) {
					connection_offset = g_sessions[index].connection_offset;
					session_id = g_sessions[index].session_id;
					snprintf(error_code, sizeof(error_code), "%s", g_sessions[index].worker_error);
					g_sessions[index].done_pending = 0;
					g_sessions[index].done_sent = 1;
					found = 1;
					break;
				}
			}
			pthread_mutex_unlock(&g_state_mutex);
			if (!found) break;
			if (connection_offset != UINT32_MAX) {
				char json[192];
				if (error_code[0]) {
					snprintf(json, sizeof(json), "{\"type\":\"stt_error\",\"session_id\":%u,\"code\":\"%s\"}",
						session_id, error_code);
				} else {
					snprintf(json, sizeof(json), "{\"type\":\"stt_done\",\"session_id\":%u}", session_id);
				}
				api_qs_send_ws_text_by_connection_offset(server, connection_offset, json);
				pthread_mutex_lock(&g_state_mutex);
				{
					STT_V2_SESSION* session = find_session_by_id_locked(session_id);
					if (session && session->connection_offset == connection_offset) clear_session(session);
				}
				pthread_mutex_unlock(&g_state_mutex);
			}
		}
		pthread_mutex_lock(&g_state_mutex);
		complete_stopping_sessions_locked();
		pthread_mutex_unlock(&g_state_mutex);
}

static int initialize_models(void)
{
	struct whisper_context_params whisper_params = whisper_context_default_params();
	struct whisper_vad_context_params vad_params = whisper_vad_default_context_params();
	vad_params.n_threads = 1;
	g_whisper_context = whisper_init_from_file_with_params(STT_V2_MODEL_PATH, whisper_params);
	if (!g_whisper_context) {
		fprintf(stderr, "Failed to load Whisper model: %s\n", STT_V2_MODEL_PATH);
		return -1;
	}
	g_vad_context = whisper_vad_init_from_file_with_params(STT_V2_VAD_MODEL_PATH, vad_params);
	if (!g_vad_context) {
		fprintf(stderr, "Failed to load Silero VAD model: %s\n", STT_V2_VAD_MODEL_PATH);
		whisper_free(g_whisper_context);
		g_whisper_context = NULL;
		return -1;
	}
	return 0;
}

static int start_worker(void)
{
	if (pthread_create(&g_worker_thread, NULL, stt_worker_main, NULL) != 0) return -1;
	g_worker_running = 1;
	return 0;
}

static void stop_worker(void)
{
	if (!g_worker_running) return;
	pthread_mutex_lock(&g_state_mutex);
	g_worker_shutdown = 1;
	pthread_cond_broadcast(&g_worker_condition);
	pthread_mutex_unlock(&g_state_mutex);
	pthread_join(g_worker_thread, NULL);
	g_worker_running = 0;
}

int main(void)
{
	QS_SERVER_CONTEXT* server = NULL;
	if (create_transcript_directory() != 0) {
		fprintf(stderr, "Failed to create transcript directory\n");
		return 1;
	}
	if (initialize_models() != 0) return 1;
	if (start_worker() != 0) {
		whisper_vad_free(g_vad_context);
		whisper_free(g_whisper_context);
		return 1;
	}
	if (api_qs_server_init(&server, STT_V2_PORT, STT_V2_MAX_CONNECTIONS, QS_SERVER_TYPE_HTTP) != 0) {
		stop_worker();
		whisper_vad_free(g_vad_context);
		whisper_free(g_whisper_context);
		return 1;
	}
	if (api_qs_server_create_router(server) != 0) {
		api_qs_free(server);
		stop_worker();
		whisper_vad_free(g_vad_context);
		whisper_free(g_whisper_context);
		return 1;
	}
	api_qs_set_on_connect_event(server, on_connect);
	api_qs_set_on_websocket_event(server, on_ws_event);
	api_qs_set_on_close_event(server, on_close);
	printf("STT Server V2 listening on port %d\n", STT_V2_PORT);
	for (;;) {
		api_qs_update(server);
		drain_pending_results(server);
		api_qs_sleep(server);
	}
	api_qs_free(server);
	whisper_vad_free(g_vad_context);
	whisper_free(g_whisper_context);
	return 0;
}