#include "qs_protocol.h"

#include <sys/socket.h>

typedef struct TEST_MESSAGE
{
	QS_WEBSOCKET_CLIENT* client;
	int count;
	uint8_t opcode;
	size_t length;
	uint8_t payload[QS_WEBSOCKET_MAX_MESSAGE_SIZE];
} TEST_MESSAGE;

static int on_message(void* user_data)
{
	TEST_MESSAGE* message = (TEST_MESSAGE*)user_data;
	QS_WEBSOCKET_CLIENT* client = message->client;
	message->count++;
	message->opcode = qs_websocket_client_get_opcode(client);
	message->length = qs_websocket_client_get_payload_length(client);
	memcpy(message->payload, qs_websocket_client_get_payload(client), message->length);
	return 0;
}

static QS_WEBSOCKET_CLIENT* create_open_client(void)
{
	QS_WEBSOCKET_CLIENT* client = NULL;
	if(qs_websocket_client_create(&client, "example.com", 80, "/") != 0) return NULL;
	client->websocket_handshake_complete = 1;
	client->websocket_state = QS_WEBSOCKET_STATE_OPEN;
	return client;
}

static QS_WEBSOCKET_CLIENT* create_handshaking_client(void)
{
	QS_WEBSOCKET_CLIENT* client = NULL;
	if(qs_websocket_client_create(&client, "example.com", 80, "/") != 0) return NULL;
	snprintf(client->websocket_accept, sizeof(client->websocket_accept), "%s", "test-accept");
	return client;
}

static int test_handshake_response(const char* response, size_t split_at, int expected_result, QS_WEBSOCKET_ERROR expected_error)
{
	int sockets[2];
	QS_WEBSOCKET_CLIENT* client = create_handshaking_client();
	if(client == NULL || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	int result = qs_websocket_client_on_recv(client, sockets[0], (const uint8_t*)response, split_at, NULL, NULL);
	if(result == 0 && split_at < strlen(response)) result = qs_websocket_client_on_recv(client, sockets[0], (const uint8_t*)response + split_at, strlen(response) - split_at, NULL, NULL);
	int passed = result == expected_result && qs_websocket_client_get_error(client) == expected_error;
	if(expected_result == 0) passed = passed && qs_websocket_client_is_handshake_complete(client) && qs_websocket_client_get_state(client) == QS_WEBSOCKET_STATE_OPEN;
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	return passed ? 0 : -1;
}

static int test_handshake_validation(void)
{
	const char* valid_response = "HTTP/1.1 101 Switching Protocols\r\nupgrade: WebSocket\r\nconnection: keep-alive, uPgRaDe\r\nsec-websocket-accept: test-accept\r\n\r\n";
	const char* missing_upgrade = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: test-accept\r\n\r\n";
	const char* missing_accept = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n";
	int failed = 0;
	failed |= test_handshake_response(valid_response, 37, 0, QS_WEBSOCKET_ERROR_NONE);
	failed |= test_handshake_response(missing_upgrade, strlen(missing_upgrade), -1, QS_WEBSOCKET_ERROR_HANDSHAKE_RESPONSE);
	failed |= test_handshake_response(missing_accept, strlen(missing_accept), -1, QS_WEBSOCKET_ERROR_HANDSHAKE_ACCEPT);
	return failed ? -1 : 0;
}

static int test_handshake_request_host(const char* host, int port, const char* expected_host)
{
	int sockets[2];
	char request[2048];
	QS_WEBSOCKET_CLIENT* client = NULL;
	if(qs_websocket_client_create(&client, host, port, "/socket") != 0 || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	int result = qs_websocket_client_on_connect(client, sockets[0]);
	ssize_t request_len = recv(sockets[1], request, sizeof(request) - 1, 0);
	if(request_len >= 0) request[request_len] = '\0';
	char expected_header[1100];
	snprintf(expected_header, sizeof(expected_header), "Host: %s\r\n", expected_host);
	int passed = result == 0 && request_len > 0 && strstr(request, "GET /socket HTTP/1.1\r\n") != NULL && strstr(request, expected_header) != NULL;
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	return passed ? 0 : -1;
}

static int test_handshake_request_host_header(void)
{
	return test_handshake_request_host("example.com", 80, "example.com") ||
		test_handshake_request_host("example.com", 8080, "example.com:8080") ? -1 : 0;
}

static int build_frame(uint8_t* frame, int fin, uint8_t opcode, const uint8_t* payload, size_t payload_len, int use_64_bit_length)
{
	size_t offset = 2;
	frame[0] = (uint8_t)((fin ? 0x80 : 0) | opcode);
	if(use_64_bit_length){
		frame[1] = 127;
		for(size_t index = 0; index < 8; index++) frame[2 + index] = (uint8_t)(payload_len >> (56 - index * 8));
		offset = 10;
	}else if(payload_len > 125){
		frame[1] = 126;
		frame[2] = (uint8_t)(payload_len >> 8);
		frame[3] = (uint8_t)payload_len;
		offset = 4;
	}else{
		frame[1] = (uint8_t)payload_len;
	}
	memcpy(frame + offset, payload, payload_len);
	return (int)(offset + payload_len);
}

static int test_frame_length(size_t payload_len, int use_64_bit_length)
{
	int sockets[2];
	uint8_t* payload = (uint8_t*)malloc(payload_len);
	uint8_t* frame = (uint8_t*)malloc(payload_len + 14);
	TEST_MESSAGE message = {0};
	QS_WEBSOCKET_CLIENT* client = create_open_client();
	if(client == NULL || payload == NULL || frame == NULL || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	for(size_t index = 0; index < payload_len; index++) payload[index] = (uint8_t)(index & 0xff);
	message.client = client;
	int frame_len = build_frame(frame, 1, 2, payload, payload_len, use_64_bit_length);
	int result = qs_websocket_client_on_recv(client, sockets[0], frame, (size_t)frame_len, on_message, &message);
	int passed = result == 0 && message.count == 1 && message.opcode == 2 && message.length == payload_len && memcmp(message.payload, payload, payload_len) == 0;
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	free(frame);
	free(payload);
	return passed ? 0 : -1;
}

static int test_fragmented_message(void)
{
	int sockets[2];
	uint8_t first[] = { 0x01, 0x02, 'h', 'e' };
	uint8_t second[] = { 0x80, 0x03, 'l', 'l', 'o' };
	TEST_MESSAGE message = {0};
	QS_WEBSOCKET_CLIENT* client = create_open_client();
	if(client == NULL || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	message.client = client;
	int result = qs_websocket_client_on_recv(client, sockets[0], first, sizeof(first), on_message, &message);
	result |= qs_websocket_client_on_recv(client, sockets[0], second, sizeof(second), on_message, &message);
	int passed = result == 0 && message.count == 1 && message.opcode == 1 && message.length == 5 && memcmp(message.payload, "hello", 5) == 0;
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	return passed ? 0 : -1;
}

static int test_ping_pong(void)
{
	int sockets[2];
	uint8_t ping[] = { 0x89, 0x02, 'o', 'k' };
	uint8_t pong[8];
	QS_WEBSOCKET_CLIENT* client = create_open_client();
	if(client == NULL || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	int result = qs_websocket_client_on_recv(client, sockets[0], ping, sizeof(ping), NULL, NULL);
	ssize_t pong_len = recv(sockets[1], pong, sizeof(pong), 0);
	int passed = result == 0 && pong_len == 8 && pong[0] == 0x8a && pong[1] == 0x82 && (pong[6] ^ pong[2]) == 'o' && (pong[7] ^ pong[3]) == 'k';
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	return passed ? 0 : -1;
}

static int test_invalid_frame(const uint8_t* frame, size_t frame_len)
{
	int sockets[2];
	QS_WEBSOCKET_CLIENT* client = create_open_client();
	if(client == NULL || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	int result = qs_websocket_client_on_recv(client, sockets[0], frame, frame_len, NULL, NULL);
	int passed = result == -1 && qs_websocket_client_get_error(client) == QS_WEBSOCKET_ERROR_FRAME;
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	return passed ? 0 : -1;
}

static int test_invalid_control_and_close_frames(void)
{
	uint8_t fragmented_ping[] = { 0x09, 0x00 };
	uint8_t oversized_pong[130];
	uint8_t one_byte_close[] = { 0x88, 0x01, 0x00 };
	uint8_t reserved_close_code[] = { 0x88, 0x02, 0x03, 0xed };
	uint8_t invalid_utf8_close[] = { 0x88, 0x04, 0x03, 0xe8, 0xc3, 0x28 };
	memset(oversized_pong, 0, sizeof(oversized_pong));
	oversized_pong[0] = 0x8a;
	oversized_pong[1] = 126;
	oversized_pong[2] = 0;
	oversized_pong[3] = 126;
	return test_invalid_frame(fragmented_ping, sizeof(fragmented_ping)) ||
		test_invalid_frame(oversized_pong, sizeof(oversized_pong)) ||
		test_invalid_frame(one_byte_close, sizeof(one_byte_close)) ||
		test_invalid_frame(reserved_close_code, sizeof(reserved_close_code)) ||
		test_invalid_frame(invalid_utf8_close, sizeof(invalid_utf8_close)) ? -1 : 0;
}

static int test_close_and_invalid_continuation(void)
{
	int sockets[2];
	uint8_t close_frame[] = { 0x88, 0x05, 0x03, 0xe8, 'b', 'y', 'e' };
	uint8_t continuation[] = { 0x80, 0x00 };
	QS_WEBSOCKET_CLIENT* client = create_open_client();
	if(client == NULL || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return -1;
	int result = qs_websocket_client_on_recv(client, sockets[0], close_frame, sizeof(close_frame), NULL, NULL);
	int passed = result == 0 && qs_websocket_client_get_state(client) == QS_WEBSOCKET_STATE_CLOSING && qs_websocket_client_get_close_code(client) == 1000 && strcmp(qs_websocket_client_get_close_reason(client), "bye") == 0;
	qs_websocket_client_destroy(client);
	client = create_open_client();
	result = qs_websocket_client_on_recv(client, sockets[0], continuation, sizeof(continuation), NULL, NULL);
	passed = passed && result == -1 && qs_websocket_client_get_error(client) == QS_WEBSOCKET_ERROR_FRAME;
	close(sockets[0]);
	close(sockets[1]);
	qs_websocket_client_destroy(client);
	return passed ? 0 : -1;
}

int main(void)
{
	int failed = 0;
	failed |= test_frame_length(125, 0);
	failed |= test_frame_length(126, 0);
	failed |= test_frame_length(65535, 0);
	failed |= test_frame_length(1, 1);
	failed |= test_handshake_validation();
	failed |= test_handshake_request_host_header();
	failed |= test_fragmented_message();
	failed |= test_ping_pong();
	failed |= test_invalid_control_and_close_frames();
	failed |= test_close_and_invalid_continuation();
	if(failed != 0){
		fprintf(stderr, "WEBSOCKET PROTOCOL TEST FAILED\n");
		return 1;
	}
	printf("WEBSOCKET PROTOCOL TEST PASSED\n");
	return 0;
}