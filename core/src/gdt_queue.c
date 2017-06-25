/*
 * Copyright (c) 2014-2017 Katsuya Owari
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * * Neither the name of the <organization> nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gdt_queue.h"

/*
 * メッセージキューユニットの生成
 * @param _ppool queueを格納するメモリプール
 * @param q_munit queueを配置するメモリユニット
 * @param qlen キュー構造体の数
 * @param size キューメッセージの文字サイズ
 */
void gdt_create_message_queue( GDT_MEMORY_POOL* _ppool, int32_t *q_munit, size_t qlen, size_t size )
{
	int i;
#ifdef __WINDOWS__
	HANDLE	*pmutex;
#else
	pthread_mutex_t *pmutex;
#endif
	GDT_MSGQUEUE *pmq;
	int32_t* mqlist;
	GDT_MSG_INFO *pminf;
	char* pmqbuf;
	do{
		if( qlen <= 0 ){
			break;
		}
		(*q_munit) = gdt_create_munit( _ppool, sizeof( GDT_MSGQUEUE ), MEMORY_TYPE_DEFAULT );
		if( (*q_munit) >= 0 )
		{
			pmq = (GDT_MSGQUEUE *)gdt_upointer( _ppool, (*q_munit) );
			pmq->top		= 0;
			pmq->tail		= 0;
			pmq->queuelen	= qlen;
#ifdef __WINDOWS__
			if( 0 >= ( pmq->mqlock_munit = gdt_create_munit( _ppool, sizeof( HANDLE ), MEMORY_TYPE_DEFAULT ) ) ){
				printf("alloc error\n");
				break;
			}
			pmutex = (HANDLE *)gdt_upointer( _ppool, pmq->mqlock_munit );
			*pmutex = CreateMutex( NULL, false , NULL );
#else
			if( 0 >= ( pmq->mqlock_munit = gdt_create_munit( _ppool, sizeof( pthread_mutex_t ), MEMORY_TYPE_DEFAULT ) ) ){
				printf("alloc error\n");
				break;
			}
			pmutex = (pthread_mutex_t *)gdt_upointer( _ppool, pmq->mqlock_munit );
			if( pthread_mutex_init(pmutex, NULL) != 0 ){
				printf( "gdt_create_message_queue:pthread_mutex_init : error \n" );
				break;
			}
#endif
			if( 0 >= ( pmq->queuemunit = gdt_create_munit( _ppool, sizeof( int32_t ) * pmq->queuelen, MEMORY_TYPE_DEFAULT ) ) ){
				printf("create mu array error\n");
				break;
			}
			mqlist = (int32_t *)gdt_upointer( _ppool, pmq->queuemunit );
			//memset( mqlist, -1, sizeof( int32_t ) * pmq->queuelen );
			for( i = 0; i < pmq->queuelen; i++ )
			{
				if( 0 >= ( mqlist[i] = gdt_create_munit( _ppool, sizeof( GDT_MSG_INFO ), MEMORY_TYPE_DEFAULT ) ) ){
					printf("create msg info error\n");
					continue;
				}
				pminf = (GDT_MSG_INFO *)gdt_upointer( _ppool, mqlist[i] );
				if( 0 >= ( pminf->msgmunit = gdt_create_munit( _ppool, sizeof( char ) * size, MEMORY_TYPE_DEFAULT ) ) ){
					printf("create msg queue buf error\n");
					continue;
				}
				pmqbuf = (char *)gdt_upointer( _ppool, pminf->msgmunit );
				pmqbuf[0] = '\0';
				//memset( pmqbuf, 0, sizeof( char ) * size );
				pminf->len = 0;
				pminf->id  = 0;
			}
		}
	}while( false );
}

/*
 * メッセージキューの追加
 * @param _ppool メッセージキューが格納されているメモリプール
 * @param q_munit メッセージキューが格納されているメモリユニットID
 * @param pbuf queueに格納するメッセージ
 * @param size メッセージサイズ
 */
int gdt_push_queue( GDT_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size )
{
	int addindex = -1;
	GDT_MSGQUEUE *pmq;
	int32_t* mqlist;
	GDT_MSG_INFO *pminf;
	char* pmqbuf;
	do{
		if( q_munit < 0 ){
			printf( "q_munit is not allocate : %d\n", q_munit );
			break;
		}
		pmq = (GDT_MSGQUEUE *)gdt_upointer( _ppool, q_munit );
		if( pmq->queuemunit < 0 ){
			printf( "queuemunit is not allocate : %d\n", pmq->queuemunit );
			break;
		}
		mqlist = (int32_t *)gdt_upointer( _ppool, pmq->queuemunit );
		if( mqlist[pmq->tail] < 0 ){
			printf( "mqlist[pmq->tail] is not allocate : %d\n", pmq->tail );
			break;
		}
		pminf = (GDT_MSG_INFO*)gdt_upointer( _ppool, mqlist[pmq->tail] );
		if( pminf->msgmunit < 0 ){
			printf( "pminf->msgmunit is not allocate : %d\n", pminf->msgmunit );
			break;
		}
		pmqbuf = (char *)gdt_upointer( _ppool, pminf->msgmunit );
		pminf->len = size;
		if( pminf->len >= gdt_usize( _ppool, pminf->msgmunit ) )
		{
			pminf->len = gdt_usize( _ppool, pminf->msgmunit ) -1;
		}
		memcpy( pmqbuf, pbuf, pminf->len );
		pmqbuf[pminf->len] = '\0';
		addindex = pmq->tail;
		pmq->tail = ( pmq->tail + 1 ) % pmq->queuelen;
		if( pmq->tail == pmq->top ){
			pmq->top = ( pmq->top + 1 ) % pmq->queuelen;
		}
	}while( false );
	return addindex;
}

/*
 * メッセージキューの追加( 排他制御 )
 * @param _ppool メッセージキューが格納されているメモリプール
 * @param q_munit メッセージキューが格納されているメモリユニットID
 * @param pbuf queueに格納するメッセージ
 * @param size メッセージサイズ
 */
int gdt_safe_push_queue( GDT_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size )
{
	int addindex = -1;
	GDT_MSGQUEUE *pmq;
#ifdef __WINDOWS__
	HANDLE	*pmutex;
#else
	pthread_mutex_t *pmutex;
#endif
	do{
		if( q_munit < 0 ){
			printf( "q_munit is not allocate : %d\n", q_munit );
			break;
		}
		pmq = (GDT_MSGQUEUE *)gdt_upointer( _ppool, q_munit );
		if( pmq->queuemunit < 0 )
		{
			printf( "queuemunit is not allocate : %d\n", pmq->queuemunit );
			break;
		}
#ifdef __WINDOWS__
		pmutex = (HANDLE *)gdt_upointer( _ppool, pmq->mqlock_munit );
		WaitForSingleObject( *pmutex, INFINITE );
		addindex = gdt_push_queue( _ppool, q_munit, pbuf, size );
		ReleaseMutex( *pmutex );
#else
		pmutex = (pthread_mutex_t *)gdt_upointer( _ppool, pmq->mqlock_munit );
		(void) pthread_mutex_lock( pmutex );
		addindex = gdt_push_queue( _ppool, q_munit, pbuf, size );
		(void) pthread_mutex_unlock( pmutex );
#endif
	}while( false );
	return addindex;
}

/*
 * メセージ取り出し
 */
int32_t gdt_dequeue( GDT_MEMORY_POOL* _ppool, int32_t q_munit )
{
	int32_t munit = -1;
	GDT_MSGQUEUE *pmq;
	int32_t* mqlist;
	GDT_MSG_INFO *pminf;
	do{
		if( q_munit < 0 ){
			printf( "q_munit is not allocate : %d\n", q_munit );
			break;
		}
		pmq = (GDT_MSGQUEUE *)gdt_upointer( _ppool, q_munit );
		if( pmq->queuemunit < 0 ){
			printf( "queuemunit is not allocate : %d\n", pmq->queuemunit );
			break;
		}
		mqlist = (int32_t *)gdt_upointer( _ppool, pmq->queuemunit );
		if( pmq->top == pmq->tail ){
			break;
		}
		if( mqlist[pmq->top] < 0 ){
			printf( "mqlist[pmq->top] is not allocate : %d\n", pmq->top );
			break;
		}
		pminf = (GDT_MSG_INFO*)gdt_upointer( _ppool, mqlist[pmq->top] );
		if( pminf->msgmunit < 0 ){
			printf( "pminf->msgmunit is not allocate : %d\n", pminf->msgmunit );
			break;
		}
		munit = pminf->msgmunit;
		pmq->top = ( pmq->top + 1 ) % pmq->queuelen;
	}while( false );
	return munit;
}
