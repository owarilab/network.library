/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "qs_api.h"
#include "qs_io.h"

typedef struct STT_CONNECTION_DATA_STRUCT
{
	char connection_id[256];
	int16_t* ring_buffer;
	int32_t ring_capacity_samples;
	int32_t write_pos;
	int32_t read_pos;
	int32_t samples_count;
	int16_t overlap_buffer[(16000 * 200) / 1000];
	int32_t overlap_samples;
	uint32_t window_count;
	FILE* wav_file;
	uint32_t sample_rate;
	uint16_t channels;
	uint16_t bits_per_sample;
	uint32_t pcm_data_bytes;
	uint32_t chunk_count;
	uint32_t session_id;
	int is_recording;
	char wav_path[256];
} STT_CONNECTION_DATA;

#define STT_CONNECTION_MAX 128
#define STT_TARGET_SAMPLE_RATE 16000
#define STT_RING_BUFFER_SECONDS 30
#define STT_RING_BUFFER_SAMPLES (STT_TARGET_SAMPLE_RATE * STT_RING_BUFFER_SECONDS)
#define STT_STEP_MS 500
#define STT_KEEP_MS 200
#define STT_STEP_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_STEP_MS) / 1000)
#define STT_KEEP_SAMPLES ((STT_TARGET_SAMPLE_RATE * STT_KEEP_MS) / 1000)

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
static void stt_send_json_message(QS_EVENT_PARAMETER params, const char* type, const char* path, uint32_t bytes, uint32_t chunks);

QS_MEMORY_CONTEXT g_temporary_memory;
QS_MEMORY_CONTEXT g_kvs_memory;
QS_KVS_CONTEXT g_kvs;

static uint32_t g_stt_session_counter = 0;
static STT_CONNECTION_DATA g_stt_connections[STT_CONNECTION_MAX];

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
	con_data->overlap_samples = 0;
	con_data->window_count = 0;
	memset(con_data->wav_path, 0, sizeof(con_data->wav_path));
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
		while (con_data->connection_id[0] != '\0' && con_data->ring_buffer && con_data->samples_count >= STT_STEP_SAMPLES) {
			int32_t sample_index;
			int32_t window_samples = con_data->overlap_samples + STT_STEP_SAMPLES;
			int16_t infer_window[STT_KEEP_SAMPLES + STT_STEP_SAMPLES];

			if (con_data->overlap_samples > 0) {
				memcpy(infer_window, con_data->overlap_buffer, sizeof(int16_t) * con_data->overlap_samples);
			}
			for (sample_index = 0; sample_index < STT_STEP_SAMPLES; sample_index++) {
				infer_window[con_data->overlap_samples + sample_index] = con_data->ring_buffer[con_data->read_pos];
				con_data->read_pos = (con_data->read_pos + 1) % con_data->ring_capacity_samples;
			}
			con_data->samples_count -= STT_STEP_SAMPLES;
			con_data->window_count += 1;

			con_data->overlap_samples = window_samples < STT_KEEP_SAMPLES ? window_samples : STT_KEEP_SAMPLES;
			memcpy(
				con_data->overlap_buffer,
				infer_window + (window_samples - con_data->overlap_samples),
				sizeof(int16_t) * con_data->overlap_samples
			);

			printf(
				"[STT][window] connection_id=%s window=%u step_samples=%d total_samples=%d remain=%d overlap=%d\n",
				con_data->connection_id,
				con_data->window_count,
				STT_STEP_SAMPLES,
				window_samples,
				con_data->samples_count,
				con_data->overlap_samples
			);
		}
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
	if (-1 == stt_write_wav_header(con_data->wav_file, con_data->sample_rate, con_data->channels, con_data->bits_per_sample, 0)) {
		fclose(con_data->wav_file);
		remove(con_data->wav_path);
		stt_reset_connection_data(con_data);
		return -1;
	}
	if (0 != fseek(con_data->wav_file, 0, SEEK_END)) {
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

	// router test
	if(0)
	{
		for(int i=0;i<1000;i++){
			char* room_name = api_qs_uniqid(&g_temporary_memory,32);
			QS_JSON_ELEMENT_OBJECT object;
			api_qs_room_create(context,room_name,&g_temporary_memory,&object);
			char* json = api_qs_json_encode_object(&object,1024);
			printf("room_info : %s\n",json);
			api_qs_memory_clean(&g_temporary_memory);
		}
		api_qs_router_memory_info(context);
	}

	// kvs test
	if(0)
	{
		QS_KVS_CONTEXT kvs;
		if(-1!=api_qs_server_get_kvs(context,&kvs)){
			for(int i=0;i<10;i++){
				char* key = api_qs_uniqid(&g_temporary_memory,32);
				char value[128];
				memset(value,0,sizeof(value));
				int is_create_buffer = 1;
				if(is_create_buffer){
					// create buffer (size sizeof(value) bytes)
					memset(value,' ',sizeof(value)-1);
					if(-1 != api_qs_kvs_set(&kvs,key,value,0)){
						char* cache_value = api_qs_kvs_get(&kvs,key);
						size_t buffer_size = api_qs_kvs_get_buffer_size(&kvs,key);
						char* random_value = api_qs_uniqid(&g_temporary_memory,128);
						snprintf(cache_value,buffer_size,"value_%d_%s",i, random_value);
						char* after_cache_value = api_qs_kvs_get(&kvs,key);
						size_t after_buffer_size = api_qs_kvs_get_buffer_size(&kvs,key);
						printf("key : %s , value : %s , buffer_size : %d, strlen(%d)\n",key,after_cache_value,(int)after_buffer_size,(int)strlen(after_cache_value));
					}
				}else{
					char* random_value = api_qs_uniqid(&g_temporary_memory,16);
					snprintf(value,sizeof(value),"value_%d_%s",i, random_value);
					api_qs_kvs_set(&kvs,key,value,0);
				}
				api_qs_memory_clean(&g_temporary_memory);
			}

			QS_JSON_ELEMENT_OBJECT object;
			QS_JSON_ELEMENT_ARRAY array;
			api_qs_object_create(&g_temporary_memory,&object);
			api_qs_array_create(&g_temporary_memory,&array);
			int32_t key_length = api_qs_kvs_keys(&array,&kvs);
			api_qs_object_push_integer(&object,"len",key_length);
			api_qs_object_push_array(&object,"keys",&array);
			char* json = api_qs_json_encode_object(&object,1024 * 512);
			printf("kvs_info : %s\n",json);
			for(int i=0;i<api_qs_array_get_length(&array);i++){
				char* key = api_qs_array_get_string(&array,i);
				char* value = api_qs_kvs_get(&kvs,key);
				printf("key : %s , value : %s\n",key,value);
			}
			printf("key_length : %d\n",key_length);
			api_qs_memory_clean(&g_temporary_memory);
		}
	}

	//api_qs_memory_info(&g_temporary_memory);
	//api_qs_memory_info(&g_kvs_memory);
	//api_qs_router_memory_info(context);
	//api_qs_kvs_memory_info(context);
	//api_qs_server_memory_info(context);

	for(;;){
		api_qs_update(context);
		stt_process_connections();
		api_qs_sleep(context);
	}
	api_qs_free(context);
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
	//QS_SERVER_CONTEXT* context = api_qs_get_server_context(params);
	api_qs_memory_clean(&g_temporary_memory);
	// curl -X POST  -H "Content-Type: application/json" -d '{"arr1":[{"id":1,"value":"arr1_v1"},{"id":2,"value":"arr1_v2"}],"arr2":[{"id":1,"value":"arr2_v1"},{"id":2,"value":"arr2_v2"}]}' "http://localhost:4444/api/json_test"
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/json_test")){
			printf("body : %s\n",api_qs_get_http_post_body(params));
			QS_JSON_ELEMENT_OBJECT object;
			api_qs_json_decode_object(&g_temporary_memory,&object,api_qs_get_http_post_body(params));
			QS_JSON_ELEMENT_ARRAY arr1;
			QS_JSON_ELEMENT_ARRAY arr2;
			api_qs_object_get_array(&object,"arr1",&arr1);
			api_qs_object_get_array(&object,"arr2",&arr2);
			printf("arr1 length : %d\n",api_qs_array_get_length(&arr1));
			printf("arr2 length : %d\n",api_qs_array_get_length(&arr2));
			int i;
			printf("<<<show arr1\n");
			for(i=0;i<api_qs_array_get_length(&arr1);i++){
				QS_JSON_ELEMENT_OBJECT tmpobj;
				api_qs_array_get_object(&arr1,i,&tmpobj);
				int32_t* id = api_qs_object_get_integer(&tmpobj,"id");
				printf("id : %d\n",*id);
				printf("value : %s\n",api_qs_object_get_string(&tmpobj,"value"));
			}
			printf("<<<show arr2\n");
			for(i=0;i<api_qs_array_get_length(&arr2);i++){
				QS_JSON_ELEMENT_OBJECT tmpobj;
				api_qs_array_get_object(&arr2,i,&tmpobj);
				int32_t* id = api_qs_object_get_integer(&tmpobj,"id");
				printf("id : %d\n",*id);
				printf("value : %s\n",api_qs_object_get_string(&tmpobj,"value"));
			}
		}
	}

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

	printf("[STT][ws] connection_id=%s opcode=%u size=%zd recording=%d chunks=%u bytes=%u\n",
		connection_id,
		opcode,
		size,
		con_data->is_recording,
		con_data->chunk_count,
		con_data->pcm_data_bytes);

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
		printf("[STT][ws] text type=%s connection_id=%s\n", msg_type, connection_id);

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
				printf("[STT] pcm chunk: connection_id=%s chunk=%u size=%zd total=%u\n",
					connection_id,
					con_data->chunk_count,
					size,
					con_data->pcm_data_bytes);
			}
			return 0;
		}

		if (message != NULL && size > 0) {
			// Legacy fallback: save one binary message as one WAV file.
			char filepath[256];
			snprintf(filepath, sizeof(filepath), "./recv_%s_%d.wav", connection_id, file_counter++);
			qs_fwrite_bin(filepath, (char*)message, (size_t)size);
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
