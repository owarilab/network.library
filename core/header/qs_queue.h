/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_QUEUE_H_
#define _QS_QUEUE_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <pthread.h>
#endif
#ifdef __WINDOWS__
#endif

typedef struct QS_MSGQUEUE
{
	int32_t top;
	int32_t tail;
	int32_t status;
	size_t queuelen;
	int32_t queuemunit;
	int32_t mqlock_munit;
} QS_MSGQUEUE;

typedef struct QS_MSG_INFO
{
	int32_t id;
	int32_t msgmunit;
	size_t len;
	uint8_t status;
} QS_MSG_INFO;

void qs_create_message_queue( QS_MEMORY_POOL* _ppool, int32_t *q_munit, size_t qlen, size_t size );
int qs_enqueue( QS_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size );
int qs_safe_enqueue( QS_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size );
int32_t qs_dequeue( QS_MEMORY_POOL* _ppool, int32_t q_munit );
int32_t qs_get_queue_length( QS_MEMORY_POOL* _ppool, int32_t q_munit );

#endif /*_QS_QUEUE_H_*/

#ifdef __cplusplus
}
#endif
