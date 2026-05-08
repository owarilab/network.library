#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qs_api.h"
#include "qs_llama_module.h"

int qs_test_parse_positive_int64_strict(const char* text, int64_t* out_value);
int qs_test_parse_int_in_range_strict(const char* text, int min_value, int max_value, int* out_value);
void qs_test_strip_markdown_code_fences_in_place(char* text);

static char g_sse_capture[32768];
static size_t g_sse_capture_len = 0;

static void reset_capture(void)
{
	g_sse_capture[0] = '\0';
	g_sse_capture_len = 0;
}

static int append_capture(const char* text)
{
	size_t text_len = strlen(text);
	if (g_sse_capture_len + text_len + 1 > sizeof(g_sse_capture)) {
		return -1;
	}
	memcpy(g_sse_capture + g_sse_capture_len, text, text_len + 1);
	g_sse_capture_len += text_len;
	return 0;
}

static int count_substring(const char* haystack, const char* needle)
{
	int count = 0;
	const char* current = haystack;
	size_t needle_len = strlen(needle);

	while ((current = strstr(current, needle)) != NULL) {
		count++;
		current += needle_len;
	}
	return count;
}

static int count_char(const char* text, char target)
{
	int count = 0;
	for (; *text != '\0'; text++) {
		if (*text == target) {
			count++;
		}
	}
	return count;
}

void api_qs_send_response_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* response)
{
	(void)context;
	(void)connection_offset;
	if (append_capture(response) != 0) {
		fprintf(stderr, "capture buffer overflow\n");
		exit(2);
	}
}

static int test_parse_positive_int64(void)
{
	int64_t value = 0;
	if (qs_test_parse_positive_int64_strict("42", &value) != 0 || value != 42) {
		fprintf(stderr, "parse_positive_int64 valid case failed\n");
		return -1;
	}
	if (qs_test_parse_positive_int64_strict("12abc", &value) == 0) {
		fprintf(stderr, "parse_positive_int64 accepted invalid suffix\n");
		return -1;
	}
	if (qs_test_parse_positive_int64_strict("0", &value) == 0) {
		fprintf(stderr, "parse_positive_int64 accepted zero\n");
		return -1;
	}
	return 0;
}

static int test_parse_int_in_range(void)
{
	int value = 0;
	if (qs_test_parse_int_in_range_strict("3", 1, 100, &value) != 0 || value != 3) {
		fprintf(stderr, "parse_int_in_range valid case failed\n");
		return -1;
	}
	if (qs_test_parse_int_in_range_strict("3xyz", 1, 100, &value) == 0) {
		fprintf(stderr, "parse_int_in_range accepted invalid suffix\n");
		return -1;
	}
	if (qs_test_parse_int_in_range_strict("101", 1, 100, &value) == 0) {
		fprintf(stderr, "parse_int_in_range accepted out-of-range value\n");
		return -1;
	}
	return 0;
}

static int test_strip_markdown_code_fences(void)
{
	char text[128] = "```json\n{\"a\":1}\n```";
	qs_test_strip_markdown_code_fences_in_place(text);
	if (strcmp(text, "{\"a\":1}\n") != 0) {
		fprintf(stderr, "strip_markdown_code_fences produced unexpected result: %s\n", text);
		return -1;
	}
	return 0;
}

static int test_sse_long_payload_is_chunked_without_loss(void)
{
	QS_LLM_HTTP_STREAM_CONTEXT stream;
	char payload[9001];

	memset(&stream, 0, sizeof(stream));
	memset(payload, 'A', sizeof(payload) - 1);
	payload[sizeof(payload) - 1] = '\0';
	stream.server_context = (QS_SERVER_CONTEXT*)1;
	stream.connection_offset = 123;
	stream.is_open = 1;

	reset_capture();
	if (qs_llm_http_stream_send_event(&stream, "sources", payload) != 0) {
		fprintf(stderr, "qs_llm_http_stream_send_event failed\n");
		return -1;
	}
	if (count_substring(g_sse_capture, "event: sources\n") != 1) {
		fprintf(stderr, "unexpected number of event headers\n");
		return -1;
	}
	if (count_substring(g_sse_capture, "data: ") != 3) {
		fprintf(stderr, "unexpected number of data chunks\n");
		return -1;
	}
	if (count_char(g_sse_capture, 'A') != 9000) {
		fprintf(stderr, "payload was truncated during SSE send\n");
		return -1;
	}
	return 0;
}

int main(void)
{
	if (test_parse_positive_int64() != 0) {
		return 1;
	}
	if (test_parse_int_in_range() != 0) {
		return 1;
	}
	if (test_strip_markdown_code_fences() != 0) {
		return 1;
	}
	if (test_sse_long_payload_is_chunked_without_loss() != 0) {
		return 1;
	}

	printf("All tests passed\n");
	return 0;
}