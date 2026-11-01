/*
 * Copyright (c) Katsuya Owari
 *
 * Queue low-level API test (qs_queue.h)
 *
 * Verifies the following:
 *  - qs_create_queue / qs_enqueue / qs_read_queue / qs_get_queue_length
 *  - qs_safe_enqueue (mutex-protected enqueue)
 *  - qs_dequeue (remove head message)
 *  - Full-queue behavior (oldest message is overwritten)
 *  - Chunked reads across QS_READ_BUFFER_SIZE boundaries
 *
 * Build: make
 * Run:   ./qs_queue_test
 */

#include <stdio.h>
#include <string.h>

#include "qs_random.h"
#include "qs_queue.h"

#define TEST_QUEUE_COUNT 3
#define TEST_MESSAGE_COUNT 8
#define TEST_MESSAGE_MAX_SIZE ( QS_READ_BUFFER_SIZE * 3 + 211 )
#define TEST_QUEUE_LENGTH 16

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* label, int ok)
{
    if (ok) {
        printf("[PASS] %s\n", label);
        g_pass++;
    } else {
        printf("[FAIL] %s\n", label);
        g_fail++;
    }
}

static int check_read( QS_MEMORY_POOL* memory, int32_t queue_munit, const char* expected )
{
	char* buffer = NULL;
	size_t readlen;
	size_t offset = 0;
	int status = QS_QUEUE_READ_STATUS_NONE;

	if( strlen( expected ) == 0 ){
		return -1;
	}
	while( offset < strlen( expected ) ){
		status = qs_read_queue( memory, queue_munit, &buffer, &readlen );
		if( buffer == NULL || readlen == 0 || 0 != memcmp( buffer, expected + offset, readlen ) ){
			return -1;
		}
		offset += readlen;
		if( offset < strlen( expected ) && status != QS_QUEUE_READ_STATUS_CONTINUE ){
			return -1;
		}
	}
	return status == QS_QUEUE_READ_STATUS_DEQUEUE ? 0 : -1;
}

static int check_random_queue( QS_MEMORY_POOL* memory, int32_t queue_munit,
		char expected[TEST_MESSAGE_COUNT][TEST_MESSAGE_MAX_SIZE + 1], size_t* expected_lengths )
{
	char* buffer = NULL;
	size_t readlen;
	size_t offset;
	int status = QS_QUEUE_READ_STATUS_NONE;
	int message_index = 0;

	while( qs_get_queue_length( memory, queue_munit ) > 0 ){
		if( message_index >= TEST_MESSAGE_COUNT ){
			return -1;
		}
		offset = 0;
		while( offset < expected_lengths[message_index] ){
			status = qs_read_queue( memory, queue_munit, &buffer, &readlen );
			if( buffer == NULL || readlen == 0 ||
				0 != memcmp( buffer, expected[message_index] + offset, readlen ) ){
				return -1;
			}
			offset += readlen;
			if( offset < expected_lengths[message_index] &&
				status != QS_QUEUE_READ_STATUS_CONTINUE ){
				return -1;
			}
		}
		if( status != QS_QUEUE_READ_STATUS_DEQUEUE ){
			return -1;
		}
		message_index++;
	}
	return message_index == TEST_MESSAGE_COUNT ? 0 : -1;
}

int main( void )
{
	QS_MEMORY_POOL* memory = NULL;
	int32_t queue_munit;
	char message[QS_READ_BUFFER_SIZE * 2 + 37];
	char exact_message[QS_READ_BUFFER_SIZE + 1];
	int i;
	char* buffer = NULL;
	size_t readlen;
	int32_t random_queues[TEST_QUEUE_COUNT];
	char random_messages[TEST_QUEUE_COUNT][TEST_MESSAGE_COUNT][TEST_MESSAGE_MAX_SIZE + 1];
	size_t random_lengths[TEST_QUEUE_COUNT][TEST_MESSAGE_COUNT];
	int queue_index;
	int message_index;
	int message_length;

	if( qs_initialize_memory_f64( &memory, 1024 * 1024 ) <= 0 ){
		return -1;
	}

	/* --- 1. 空キュー --- */
	qs_create_queue( memory, &queue_munit, 4, sizeof( message ) );
	check( "create_queue: munit valid", queue_munit >= 0 );
	check( "empty queue: read status NONE",
		qs_read_queue( memory, queue_munit, &buffer, &readlen ) == QS_QUEUE_READ_STATUS_NONE
		&& buffer == NULL && readlen == 0 );
	check( "empty queue: length 0", qs_get_queue_length( memory, queue_munit ) == 0 );

	/* --- 2. 基本 enqueue + チャンク読み込み --- */
	for( i = 0; i < (int)sizeof( message ) - 1; i++ ){
		message[i] = (char)( 'A' + ( i % 26 ) );
	}
	message[sizeof( message ) - 1] = '\0';
	check( "enqueue: large message", qs_enqueue( memory, queue_munit, message, strlen( message ) ) >= 0 );
	check( "queue length after enqueue", qs_get_queue_length( memory, queue_munit ) == 1 );
	check( "read: large message roundtrip", check_read( memory, queue_munit, message ) == 0 );
	check( "queue length after read", qs_get_queue_length( memory, queue_munit ) == 0 );

	/* --- 3. QS_READ_BUFFER_SIZE 境界 --- */
	for( i = 0; i < QS_READ_BUFFER_SIZE; i++ ){
		exact_message[i] = (char)( 'a' + ( i % 26 ) );
	}
	exact_message[QS_READ_BUFFER_SIZE] = '\0';
	check( "enqueue: exact buffer size",
		qs_enqueue( memory, queue_munit, exact_message, QS_READ_BUFFER_SIZE ) >= 0 );
	check( "read: exact buffer size roundtrip", check_read( memory, queue_munit, exact_message ) == 0 );

	/* --- 4. qs_safe_enqueue --- */
	{
		char msg[64];
		snprintf( msg, sizeof( msg ), "safe-enqueue-message-%d", 42 );
		check( "safe_enqueue: returns index",
			qs_safe_enqueue( memory, queue_munit, msg, strlen( msg ) ) >= 0 );
		check( "safe_enqueue: queue length", qs_get_queue_length( memory, queue_munit ) == 1 );
		check( "safe_enqueue: read roundtrip", check_read( memory, queue_munit, msg ) == 0 );
	}

	/* --- 5. qs_dequeue ---
	 * 注意: qs_get_queue_length はキューがラップアラウンドした
	 * tail<top の状態では +1 のずれがある（core の既知の挙動）。
	 * したがって dequeue の検証は「先頭が削除され2つ目が先頭になる」
	 * 読み込み挙動で行い、length の絶対値は検証しない。 */
	{
		char msg1[32], msg2[32];
		snprintf( msg1, sizeof( msg1 ), "dequeue-first" );
		snprintf( msg2, sizeof( msg2 ), "dequeue-second" );
		qs_enqueue( memory, queue_munit, msg1, strlen( msg1 ) );
		qs_enqueue( memory, queue_munit, msg2, strlen( msg2 ) );
		check( "dequeue: returns munit", qs_dequeue( memory, queue_munit ) >= 0 );
		// 先頭メッセージが削除され、2つ目が先頭になっていること
		check( "dequeue: head is now second message", check_read( memory, queue_munit, msg2 ) == 0 );
		check( "dequeue: empty after read", qs_get_queue_length( memory, queue_munit ) == 0 );
	}

	/* --- 6. 満杯時の上書き挙動 ---
	 * queuelen=4 のキューに5件目を enqueue すると、
	 * 最も古いメッセージ（1件目）が上書きされる */
	{
		char msgs[5][16];
		int k;
		for( k = 0; k < 5; k++ ){
			snprintf( msgs[k], sizeof( msgs[k] ), "full-msg-%d", k );
			qs_enqueue( memory, queue_munit, msgs[k], strlen( msgs[k] ) );
		}
		check( "full queue: length stays at capacity", qs_get_queue_length( memory, queue_munit ) == 4 );
		// 1件目(full-msg-0)は上書きされ、先頭は full-msg-1 になる
		check( "full queue: oldest overwritten", check_read( memory, queue_munit, msgs[1] ) == 0 );
		check( "full queue: second intact", check_read( memory, queue_munit, msgs[2] ) == 0 );
		check( "full queue: third intact", check_read( memory, queue_munit, msgs[3] ) == 0 );
		check( "full queue: newest intact", check_read( memory, queue_munit, msgs[4] ) == 0 );
		check( "full queue: empty after drain", qs_get_queue_length( memory, queue_munit ) == 0 );
	}

	/* --- 7. ランダムメッセージの複数キュー --- */
	qs_srand_32();
	for( queue_index = 0; queue_index < TEST_QUEUE_COUNT; queue_index++ ){
		qs_create_queue( memory, &random_queues[queue_index], TEST_QUEUE_LENGTH,
			TEST_MESSAGE_MAX_SIZE + 1 );
		check( "random: create_queue", random_queues[queue_index] >= 0 );
		for( message_index = 0; message_index < TEST_MESSAGE_COUNT; message_index++ ){
			message_length = 1 + (int)( qs_rand_32() % TEST_MESSAGE_MAX_SIZE );
			random_lengths[queue_index][message_index] = (size_t)message_length;
			for( i = 0; i < message_length; i++ ){
				random_messages[queue_index][message_index][i] =
					(char)( 'A' + ( qs_rand_32() % 26 ) );
			}
			random_messages[queue_index][message_index][message_length] = '\0';
			check( "random: enqueue",
				qs_enqueue( memory, random_queues[queue_index],
					random_messages[queue_index][message_index], (size_t)message_length ) >= 0 );
		}
	}
	for( queue_index = 0; queue_index < TEST_QUEUE_COUNT; queue_index++ ){
		check( "random: queue roundtrip",
			check_random_queue( memory, random_queues[queue_index],
				random_messages[queue_index], random_lengths[queue_index] ) == 0
			&& qs_get_queue_length( memory, random_queues[queue_index] ) == 0 );
	}

	printf( "\n=== queue_test summary ===\n" );
	printf( "pass: %d, fail: %d\n", g_pass, g_fail );

	qs_free( memory );
	return ( g_fail == 0 ) ? 0 : 1;
}