/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>

#include "qs_random.h"
#include "qs_queue.h"

#define TEST_QUEUE_COUNT 3
#define TEST_MESSAGE_COUNT 8
#define TEST_MESSAGE_MAX_SIZE ( QS_READ_BUFFER_SIZE * 3 + 211 )
#define TEST_QUEUE_LENGTH 16

static int check_read( QS_MEMORY_POOL* memory, int32_t queue_munit, const char* expected )
{
	char* buffer = NULL;
	size_t readlen;
	size_t offset = 0;
	int status;

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
	int status;
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
	qs_create_queue( memory, &queue_munit, 4, sizeof( message ) );
	if( queue_munit < 0 ){
		qs_free( memory );
		return -1;
	}
	if( qs_read_queue( memory, queue_munit, &buffer, &readlen ) != QS_QUEUE_READ_STATUS_NONE || buffer != NULL || readlen != 0 ){
		printf( "empty queue test failed\n" );
		qs_free( memory );
		return -1;
	}

	for( i = 0; i < (int)sizeof( message ) - 1; i++ ){
		message[i] = (char)( 'A' + ( i % 26 ) );
	}
	message[sizeof( message ) - 1] = '\0';
	if( qs_enqueue( memory, queue_munit, message, strlen( message ) ) < 0 ){
		qs_free( memory );
		return -1;
	}
	if( check_read( memory, queue_munit, message ) < 0 || qs_get_queue_length( memory, queue_munit ) != 0 ){
		printf( "queue read test failed\n" );
		qs_free( memory );
		return -1;
	}
	for( i = 0; i < QS_READ_BUFFER_SIZE; i++ ){
		exact_message[i] = (char)( 'a' + ( i % 26 ) );
	}
	exact_message[QS_READ_BUFFER_SIZE] = '\0';
	if( qs_enqueue( memory, queue_munit, exact_message, QS_READ_BUFFER_SIZE ) < 0 ||
		check_read( memory, queue_munit, exact_message ) < 0 ){
		printf( "exact buffer queue test failed\n" );
		qs_free( memory );
		return -1;
	}

	qs_srand_32();
	for( queue_index = 0; queue_index < TEST_QUEUE_COUNT; queue_index++ ){
		qs_create_queue( memory, &random_queues[queue_index], TEST_QUEUE_LENGTH,
			TEST_MESSAGE_MAX_SIZE + 1 );
		if( random_queues[queue_index] < 0 ){
			qs_free( memory );
			return -1;
		}
		for( message_index = 0; message_index < TEST_MESSAGE_COUNT; message_index++ ){
			message_length = 1 + (int)( qs_rand_32() % TEST_MESSAGE_MAX_SIZE );
			random_lengths[queue_index][message_index] = (size_t)message_length;
			for( i = 0; i < message_length; i++ ){
				random_messages[queue_index][message_index][i] =
					(char)( 'A' + ( qs_rand_32() % 26 ) );
			}
			random_messages[queue_index][message_index][message_length] = '\0';
			if( qs_enqueue( memory, random_queues[queue_index],
				random_messages[queue_index][message_index], (size_t)message_length ) < 0 ){
				qs_free( memory );
				return -1;
			}
		}
	}
	for( queue_index = 0; queue_index < TEST_QUEUE_COUNT; queue_index++ ){
		if( check_random_queue( memory, random_queues[queue_index],
			random_messages[queue_index], random_lengths[queue_index] ) < 0 ||
			qs_get_queue_length( memory, random_queues[queue_index] ) != 0 ){
			printf( "random queue test failed: queue=%d\n", queue_index );
			qs_free( memory );
			return -1;
		}
	}

	printf( "queue read test passed: %d queues, %d messages each, random chunks of %d bytes\n",
		TEST_QUEUE_COUNT, TEST_MESSAGE_COUNT, QS_READ_BUFFER_SIZE );
	qs_free( memory );
	return 0;
}