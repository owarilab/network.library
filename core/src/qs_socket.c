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

#include "qs_socket.h"

QS_SOCKET_OPTION* gdt_create_tcp_server(char* hostname, char* portnum)
{
	QS_MEMORY_POOL* memory_pool = NULL;
	QS_SOCKET_OPTION *option;
	size_t maxconnection = 1000;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 128, SIZE_MBYTE * 128, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		return NULL;
	}
	int32_t option_munit = gdt_create_munit( memory_pool, sizeof( QS_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
	if( option_munit == -1 ){
		return NULL;
	}
	option = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool,option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOCKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

QS_SOCKET_OPTION* gdt_create_tcp_server_plane(char* hostname, char* portnum)
{
	QS_MEMORY_POOL* memory_pool = NULL;
	QS_SOCKET_OPTION *option;
	size_t maxconnection = 1000;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 128, SIZE_MBYTE * 128, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		return NULL;
	}
	int32_t option_munit = gdt_create_munit( memory_pool, sizeof( QS_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
	if( option_munit == -1 ){
		return NULL;
	}
	option = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool,option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOCKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_PLAIN, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

QS_SOCKET_OPTION* gdt_create_udp_server(char* hostname, char* portnum)
{
	QS_MEMORY_POOL* memory_pool = NULL;
	QS_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 1, SIZE_MBYTE * 1, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 1) <= 0) {
		return NULL;
	}
	int32_t option_munit = gdt_create_munit( memory_pool, sizeof( QS_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
	if( option_munit == -1 ){
		return NULL;
	}
	option = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool,option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOCKET_TYPE_SERVER_UDP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)){
		return NULL;
	}
	return option;
}

QS_SOCKET_OPTION* gdt_create_tcp_client(char* hostname, char* portnum)
{
	QS_MEMORY_POOL* memory_pool = NULL;
	QS_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 1, SIZE_MBYTE * 1, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		return NULL;
	}
	int32_t option_munit = gdt_create_munit(memory_pool, sizeof(QS_SOCKET_OPTION), MEMORY_TYPE_DEFAULT);
	if (option_munit == -1) {
		return NULL;
	}
	option = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool, option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOCKET_TYPE_CLIENT_TCP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

QS_SOCKET_OPTION* gdt_create_tcp_client_plane(char* hostname, char* portnum)
{
	QS_MEMORY_POOL* memory_pool = NULL;
	QS_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 1, SIZE_MBYTE * 1, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		return NULL;
	}
	int32_t option_munit = gdt_create_munit(memory_pool, sizeof(QS_SOCKET_OPTION), MEMORY_TYPE_DEFAULT);
	if (option_munit == -1) {
		return NULL;
	}
	option = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool, option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOCKET_TYPE_CLIENT_TCP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_PLAIN, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

QS_SOCKET_OPTION* gdt_create_udp_client(char* hostname, char* portnum)
{
	QS_MEMORY_POOL* memory_pool = NULL;
	QS_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		return NULL;
	}
	int32_t option_munit = gdt_create_munit(memory_pool, sizeof(QS_SOCKET_OPTION), MEMORY_TYPE_DEFAULT);
	if (option_munit == -1) {
		return NULL;
	}
	option = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool, option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOCKET_TYPE_CLIENT_UDP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

ssize_t gdt_send_message(uint32_t payload_type, char* payload, size_t payload_len, QS_RECV_INFO *gdt_recv_info)
{
	ssize_t ret = 0;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option != NULL ){
		if( gdt_recv_info->tinfo->sockparam.acc != -1 ){
			if( -1 == ( ret = gdt_send(option, &gdt_recv_info->tinfo->sockparam, payload, payload_len, payload_type) ) ){
				if( option->close_callback != NULL ){
					option->close_callback( gdt_recv_info->tinfo );
				}
				gdt_free_sockparam( option, &gdt_recv_info->tinfo->sockparam );
			}
		}
	}
	return ret;
}

ssize_t gdt_send_message_broadcast(uint32_t payload_type, char* payload, size_t payload_len, QS_RECV_INFO *gdt_recv_info)
{
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option != NULL ){
		return gdt_send_broadcast( option, payload, payload_len, payload_type );
	}
	return 0;
}

ssize_t gdt_send_message_othercast(uint32_t payload_type, char* payload, size_t payload_len, QS_RECV_INFO *gdt_recv_info)
{
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option == NULL ){
		return 0;
	}
	ssize_t ret = 0;
	QS_SERVER_CONNECTION_INFO *tmptinfo;
	int i;
	if( -1 == option->connection_munit )
	{
		return 0;
	}
	for( i = 0; i < option->maxconnection; i++ )
	{
		tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), i );
		if(tmptinfo->sockparam.acc != -1 && gdt_recv_info->tinfo->sockparam.acc != tmptinfo->sockparam.acc )
		{
			if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, payload, payload_len, payload_type ) ) ){
				if( option->close_callback != NULL ){
					option->close_callback( tmptinfo );
				}
				gdt_free_sockparam( option, &tmptinfo->sockparam );
			}
		}
	}
	return ret;
}

ssize_t gdt_send_message_multicast(uint32_t payload_type, char* payload, size_t payload_len, QS_RECV_INFO *gdt_recv_info, QS_MEMORY_POOL* array_memory, int32_t array_munit)
{
	ssize_t ret = 0;
	if( array_munit == -1 ){
		return ret;
	}
	QS_SOCKET_OPTION *option = (QS_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option->connection_munit == -1 ){
		return ret;
	}
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int i;
	char* pbuf;
	parray = (QS_ARRAY*)QS_GET_POINTER( array_memory, array_munit );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( array_memory, parray->munit );
	for( i = 0; i < parray->len; i++ )
	{
		pbuf = (char*)QS_GET_POINTER(array_memory,(elm+i)->munit);
		if( strcmp(pbuf,"") ){
			int offset = atoi(pbuf);
			QS_SERVER_CONNECTION_INFO *tmptinfo;
			if( offset >= 0 && offset < option->maxconnection )
			{
				tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), offset );
				if( tmptinfo->sockparam.acc != -1 )
				{
					if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, payload, payload_len, payload_type ) ) ){
						if( option->close_callback != NULL ){
							option->close_callback( tmptinfo );
						}
						gdt_free_sockparam( option, &tmptinfo->sockparam );
					}
				}
			}
		}
	}
	return ret;
}

ssize_t gdt_send_message_multiothercast(uint32_t payload_type, char* payload, size_t payload_len, QS_RECV_INFO *gdt_recv_info, QS_MEMORY_POOL* array_memory, int32_t array_munit)
{
	ssize_t ret = 0;
	if( array_munit == -1 ){
		return ret;
	}
	QS_SOCKET_OPTION *option = (QS_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( -1 == option->connection_munit ){
		return ret;
	}
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int i;
	char* pbuf;
	parray = (QS_ARRAY*)QS_GET_POINTER( array_memory, array_munit );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( array_memory, parray->munit );
	for( i = 0; i < parray->len; i++ )
	{
		pbuf = (char*)QS_GET_POINTER(array_memory,(elm+i)->munit);
		if( strcmp(pbuf,"") ){
			int offset = atoi(pbuf);
			QS_SERVER_CONNECTION_INFO *tmptinfo;
			if( offset >= 0 && offset < option->maxconnection && offset != gdt_recv_info->tinfo->index )
			{
				tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), offset );
				if( -1 != tmptinfo->sockparam.acc )
				{
					if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, payload, payload_len, payload_type ) ) ){
						if( option->close_callback != NULL ){
							option->close_callback( tmptinfo );
						}
						gdt_free_sockparam( option, &tmptinfo->sockparam );
					}
				}
			}
		}
	}
	return ret;
}

ssize_t gdt_client_send_message(uint32_t payload_type, char* payload, size_t payload_len, QS_SOCKET_OPTION *option)
{
	ssize_t ret = 0;
	if( option == NULL ){
		return ret;
	}
	if( option->connection_munit != -1 )
	{
		QS_SERVER_CONNECTION_INFO* gdt_connection_info = (QS_SERVER_CONNECTION_INFO*)QS_GET_POINTER(option->memory_pool, option->connection_munit);
		if( gdt_connection_info != NULL && gdt_connection_info->sockparam.acc != -1 ){
			if( -1 == ( ret = gdt_send(option, &gdt_connection_info->sockparam, payload, payload_len, payload_type) ) ){
				if( option->close_callback != NULL ){
					option->close_callback( gdt_connection_info );
				}
				gdt_close_socket(&gdt_connection_info->sockparam.acc,NULL);
				gdt_free_sockparam( option, &gdt_connection_info->sockparam );
			}
		}
	}
	return ret;
}

int32_t gdt_set_client_id(QS_SOCKET_OPTION *option,uint32_t id)
{
	if( option == NULL ){
		return QS_SYSTEM_ERROR;
	}
	if( option->connection_munit != -1 )
	{
		QS_SERVER_CONNECTION_INFO* gdt_connection_info = (QS_SERVER_CONNECTION_INFO*)QS_GET_POINTER(option->memory_pool, option->connection_munit);
		if( gdt_connection_info != NULL && gdt_connection_info->sockparam.acc != -1 ){
			gdt_connection_info->index = id;
			return QS_SYSTEM_OK;
		}
	}
	return QS_SYSTEM_ERROR;
}

int gdt_initialize_socket_option( 
	  QS_SOCKET_OPTION *option
	, char* hostname
	, char* portnum
	, uint8_t socket_type
	, uint8_t mode
	, uint8_t protocol
	, size_t maxconnection
	, QS_MEMORY_POOL* memory_pool
	, QS_MEMORY_POOL* mmap_memory_pool
)
{
	int result = 0;
	option->inetflag			= 0;
	option->sockid				= -1;
	option->sockid6				= -1;
	option->recvtimeoutmode		= 0;//TIMEOUT_MODE_NONE;
	option->maxconnection		= maxconnection;
	option->socket_type			= socket_type;
	option->mode				= mode;
	option->protocol			= protocol;
	option->t_sec				= 5;	// default 5sec
	option->t_usec				= 0;	// default 0usec
	option->s_sec				= 5;	// default 5sec
	option->s_usec				= 0;	// default 0usec
	option->sleep_usec			= 1000;
	option->wait_read = 0;
	option->connection_start_callback	= NULL;
	option->send_finish_callback		= NULL;
	option->plane_recv_callback			= NULL;
	option->payload_recv_callback 		= NULL;
	option->close_callback				= NULL;
	option->timeout_callback			= NULL;
	option->user_recv_function			= NULL;
	option->user_send_function			= NULL;
	option->user_protocol_filter        = NULL;
	option->recvbuffer_size				= SIZE_KBYTE*16;
	option->sendbuffer_size				= SIZE_KBYTE*128;
	option->msgbuffer_size				= SIZE_KBYTE*16;
	option->connection_munit			= -1;
	if( hostname == NULL ){
		option->host_name_munit			= -1;
	}
	else{
		if( ( option->host_name_munit = gdt_create_munit( memory_pool, SIZE_BYTE * ( 128 ), MEMORY_TYPE_DEFAULT ) ) != -1 )
		{
			(void)snprintf( (char *)gdt_upointer( memory_pool, option->host_name_munit ), gdt_usize( memory_pool, option->host_name_munit ), "%s", hostname );
		}
	}
	if( portnum == NULL ){
		option->port_num_munit = -1;
	}else{
		if( ( option->port_num_munit = gdt_create_munit( memory_pool, SIZE_BYTE * ( 128 ), MEMORY_TYPE_DEFAULT ) ) != -1 )
		{
			(void) snprintf( (char *)gdt_upointer( memory_pool, option->port_num_munit ), gdt_usize( memory_pool, option->port_num_munit ), "%s", portnum );
		}
	}
	
	if( -1 == ( option->backend_munit = gdt_create_munit( memory_pool, sizeof(QS_SOCKET_OPTION) * option->maxconnection, MEMORY_TYPE_DEFAULT) ) ){
		return -1;
	}
	QS_SOCKET_OPTION* backend_clients = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory_pool,option->backend_munit);
	int i;
	for(i=0;i<option->maxconnection;i++){
		memset(&backend_clients[i],0,sizeof(QS_SOCKET_OPTION));
		backend_clients[i].sockid = -1;
	}
	
	option->lock_file_fd = -1;
	option->lock_file_munit = -1;
	option->memory_pool = memory_pool;
	option->mmap_memory_pool = mmap_memory_pool;
	option->application_data = NULL;
	option->addr = NULL;
#ifdef __WINDOWS__
	result = WSAStartup( MAKEWORD( 2, 2 ), &option->wsdata );
	if( result != 0 )
	{
		switch( result )
		{
		case WSASYSNOTREADY:
			printf("WSASYSNOTREADY\n");
			break;
		case WSAVERNOTSUPPORTED:
			printf("WSAVERNOTSUPPORTED\n");
			break;
		case WSAEINPROGRESS:
			printf("WSAEINPROGRESS\n"); 
			break;
		case WSAEPROCLIM:
			printf("WSAEPROCLIM\n"); 
			break;
		case WSAEFAULT:
			printf("WSAEFAULT\n");
			break;
		}
	}
#endif
	return result;
}

void set_on_connect_event( QS_SOCKET_OPTION *option, QS_CONNECTION_EVENT_CALLBACK func )
{
	option->connection_start_callback = func;
}
void set_on_sent_event( QS_SOCKET_OPTION *option, QS_CALLBACK func )
{
	option->send_finish_callback = func;
}
void set_on_packet_recv_event( QS_SOCKET_OPTION *option, QS_CALLBACK func )
{
	option->plane_recv_callback = func;
}
void set_on_payload_recv_event( QS_SOCKET_OPTION *option, QS_ON_RECV func )
{
	option->payload_recv_callback = func;
}
void set_on_close_event( QS_SOCKET_OPTION *option, QS_CONNECTION_EVENT_CALLBACK func )
{
	option->close_callback = func;
}
void set_user_recv_event( QS_SOCKET_OPTION *option, QS_USER_RECV func )
{
	option->user_recv_function = func;
}
void set_user_send_event(QS_SOCKET_OPTION *option, QS_USER_SEND func)
{
	option->user_send_function = func;
}
void set_user_protocol_filter(QS_SOCKET_OPTION *option, QS_USER_PROTOCOL_FILTER func)
{
	option->user_protocol_filter = func;
}
void gdt_set_timeout_event( QS_SOCKET_OPTION *option, QS_CALLBACK func )
{
	option->timeout_callback = func;
}
void gdt_set_connection_timeout( QS_SOCKET_OPTION *option, int32_t sec, int32_t usec )
{
	option->t_sec = sec;
	option->t_usec = usec;
}
void gdt_set_select_timeout( QS_SOCKET_OPTION *option, int32_t sec, int32_t usec )
{
	option->s_sec = sec;
	option->s_usec = usec;
}

void gdt_set_recv_buffer( QS_SOCKET_OPTION* option, size_t buffer_size )
{
	option->recvbuffer_size = buffer_size;
}
void gdt_set_send_buffer( QS_SOCKET_OPTION* option, size_t buffer_size )
{
	option->sendbuffer_size = buffer_size;
}
void gdt_set_message_buffer( QS_SOCKET_OPTION* option, size_t buffer_size )
{
	option->msgbuffer_size = buffer_size;
}

QS_SOCKET_OPTION* gdt_get_backend( QS_SOCKET_OPTION* option, int index )
{
	QS_SOCKET_OPTION* backend_clients = (QS_SOCKET_OPTION*)QS_GET_POINTER(option->memory_pool,option->backend_munit);
	if(index<0||index>=option->maxconnection){
		return NULL;
	}
	return &backend_clients[index];
}

void gdt_init_socket_param( QS_SOCKPARAM *psockparam )
{
	psockparam->http_header_munit	= -1;
	psockparam->wsockkey_munit		= -1;
	psockparam->c_status			= PROTOCOL_STATUS_DEFAULT;
	psockparam->acc					= -1;
	psockparam->type				= 0;
	psockparam->phase				= 0;
	psockparam->fin					= 1;
	psockparam->rsv					= 0;
	psockparam->opcode				= 0;
	psockparam->mask				= 0;
	psockparam->ckpayloadlen		= 0;
	psockparam->payload_type		= 0;
	psockparam->payloadlen			= -1;
	psockparam->maskindex			= 0;
	psockparam->payloadmask			= 0;
	psockparam->tmpmsglen			= 0;
	psockparam->tmpbitsift			= -1;
	psockparam->continue_pos		= 0;
	psockparam->appdata32bit = 0x00000000;
	psockparam->buf_munit			= -1;
	psockparam->header_size			= 0;
	memset(psockparam->header,0,sizeof(psockparam->header));
	//psockparam->fromlen = sizeof(psockparam->from);
}

void gdt_free_sockparam( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam )
{
	psockparam->c_status			= PROTOCOL_STATUS_DEFAULT;
	psockparam->acc					= -1;
	psockparam->type				= 0;
	psockparam->phase				= 0;
	psockparam->fin					= 1;
	psockparam->rsv					= 0;
	psockparam->opcode				= 0;
	psockparam->mask				= 0;
	psockparam->ckpayloadlen		= 0;
	psockparam->payload_type		= 0;
	psockparam->payloadlen			= -1;
	psockparam->maskindex			= 0;
	psockparam->payloadmask			= 0;
	psockparam->tmpmsglen			= 0;
	psockparam->tmpbitsift			= -1;
	psockparam->continue_pos		= 0;
	psockparam->appdata32bit = 0x00000000;
//	if( option->memory_pool != NULL && psockparam->buf_munit >= 0 ){
//		gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
//	}
//	psockparam->buf_munit			= -1;
	psockparam->header_size			= 0;
	memset(psockparam->header,0,sizeof(psockparam->header));
}

int gdt_close_socket(QS_SOCKET_ID* sock, char* error )
{
	int errorno = 0;
#ifdef __WINDOWS__
	shutdown(*sock, SD_BOTH);
	closesocket( *sock );
#else
	(void) close( *sock );
#endif
	if( error != NULL ){
		errorno = gdt_error( error );
	}
	*sock = -1;
	return errorno;
}

int32_t gdt_make_connection_info( QS_SOCKET_OPTION *option )
{
	option->connection_munit = gdt_create_munit( option->memory_pool, sizeof( QS_SERVER_CONNECTION_INFO ) * ( option->maxconnection ), MEMORY_TYPE_DEFAULT );
	if( -1 != option->connection_munit )
	{
		int i;
		QS_SERVER_CONNECTION_INFO * child;
		for( i = 0; i < option->maxconnection; i++ )
		{
			child = (QS_SERVER_CONNECTION_INFO *)gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), i );
			gdt_make_connection_info_core(option,child,i);
		}
	}
	return option->connection_munit;
}

int32_t gdt_make_connection_info_core( QS_SOCKET_OPTION *option, QS_SERVER_CONNECTION_INFO* tinfo, int index )
{
	size_t recvbuffer_size = sizeof( char ) * ( option->recvbuffer_size );
	size_t msgbuffer_size = sizeof( char ) * ( option->msgbuffer_size );
	tinfo->id = -1;
	tinfo->index = index;
	tinfo->gdt_socket_option= option;
	tinfo->create_time = time(NULL);
	tinfo->update_time = tinfo->create_time;
	if( -1 == ( tinfo->recvbuf_munit = gdt_create_munit( option->memory_pool, recvbuffer_size, MEMORY_TYPE_DEFAULT ) ) ){
		return QS_SYSTEM_ERROR;
	}
	if( -1 == ( tinfo->recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( struct QS_RECV_INFO ), MEMORY_TYPE_DEFAULT ) ) ){
		return QS_SYSTEM_ERROR;
	}
	if( -1 == ( tinfo->recvmsg_munit = gdt_create_munit( option->memory_pool, msgbuffer_size, MEMORY_TYPE_DEFAULT ) ) ){
		return QS_SYSTEM_ERROR;
	}
	tinfo->user_information = -1;
	gdt_init_socket_param( &tinfo->sockparam );
	return QS_SYSTEM_OK;
}

void gdt_initialize_connection_info( QS_SOCKET_OPTION *option, struct QS_SERVER_CONNECTION_INFO* tinfo )
{
	char *pbuf;
	if( tinfo->recvbuf_munit != -1 )
	{
		pbuf = (char*)gdt_upointer( option->memory_pool, tinfo->recvbuf_munit );
		memset( pbuf, 0, gdt_usize( option->memory_pool, tinfo->recvbuf_munit ) );
	}
	if( tinfo->recvmsg_munit != -1 )
	{
		pbuf = (char*)gdt_upointer( option->memory_pool, tinfo->recvmsg_munit );
		memset( pbuf, 0, gdt_usize( option->memory_pool, tinfo->recvmsg_munit ) );
	}
}


ssize_t gdt_send_one( QS_SOCKET_OPTION *option, uint32_t connection, char *buf, size_t size, uint32_t payload_type )
{
	ssize_t ret = 0;
	QS_SERVER_CONNECTION_INFO *tmptinfo;
	int i;
	if( -1 == option->connection_munit )
	{
		return ret;
	}
	for( i = 0; i < option->maxconnection; i++ )
	{
		tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), i );
		if( tmptinfo->index == connection )
		{
			if( -1 != tmptinfo->sockparam.acc )
			{
				if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, buf, size, payload_type ) ) ){
					if( option->close_callback != NULL ){
						option->close_callback( tmptinfo );
					}
					gdt_free_sockparam( option, &tmptinfo->sockparam );
				}
			}
			break;
		}
	}
	return ret;
}

ssize_t gdt_send_broadcast( QS_SOCKET_OPTION *option, char *buf, size_t size, uint32_t payload_type )
{
	ssize_t ret = 0;
	QS_SERVER_CONNECTION_INFO *tmptinfo;
	int i;
	if( -1 == option->connection_munit )
	{
		return ret;
	}
	for( i = 0; i < option->maxconnection; i++ )
	{
		tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), i );
		if( -1 != tmptinfo->sockparam.acc )
		{
			if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, buf, size, payload_type ) ) ){
				if( option->close_callback != NULL ){
					option->close_callback( tmptinfo );
				}
				gdt_free_sockparam( option, &tmptinfo->sockparam );
			}
		}
	}
	return ret;
}

ssize_t gdt_send( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, char *buf, size_t size, uint32_t payload_type )
{
	ssize_t len = 0;
	int error_code = QS_SYSTEM_OK;
	if( psockparam->type == SOCK_TYPE_NORMAL_UDP )
	{
		struct sockaddr* addr = NULL;
		size_t fromlen = 0;
		if (option->socket_type == SOCKET_TYPE_SERVER_UDP) {
			addr = (struct sockaddr *)&psockparam->from;
			fromlen = psockparam->fromlen;
		}
		else{
			if( ( option->inetflag & INET_FLAG_BIT_CONNECT_UDP ) == 0 ){
				struct sockaddr_storage saddr;
				socklen_t saddr_len = 0;
				if( QS_SYSTEM_OK != gdt_get_sockaddr_info(option,&saddr,&saddr_len) )
				{
					printf("gdt_get_sockaddr_info error\n");
					return len;
				}
				addr = (struct sockaddr*)&saddr;
				fromlen = saddr_len;
			}
		}
		if( option->protocol == PROTOCOL_SIMPLE )
		{
			if( ( len = gdt_send_udpmsg( option, psockparam, buf, size, payload_type, addr, fromlen ) ) == -1 )
			{
				perror( "gdt_send_udpmsg" );
				error_code = QS_SYSTEM_ERROR;
			}
		}
		else{
			if( ( len = gdt_sendto_all( psockparam->acc, buf, size, 0, addr, fromlen ) ) == -1 )
			{
				perror( "gdt_sendto_all" );
				error_code = QS_SYSTEM_ERROR;
			}
		}
	}
	else if( psockparam->type == SOCK_TYPE_NORMAL_TCP )
	{
		if( option->protocol == PROTOCOL_SIMPLE )
		{
			if( ( len = gdt_send_msg( option, psockparam, buf, size, payload_type ) ) == -1 )
			{
				perror( "gdt_send_msg" );
				error_code = QS_SYSTEM_ERROR;
			}
		}
		else{
			if( ( len = gdt_send_all( psockparam->acc, buf, size, 0 ) ) == -1 )
			{
				perror( "gdt_send_all" );
				error_code = QS_SYSTEM_ERROR;
			}
		}
	}
	else{
		perror( "invalid send protocol" );
		len = -1;
		error_code = QS_SYSTEM_ERROR;
	}
	if( error_code == QS_SYSTEM_OK ){
		if( option->send_finish_callback != NULL ){
			option->send_finish_callback( (void*)psockparam );
		}
	}
	return len;
}

ssize_t gdt_send_all(QS_SOCKET_ID soc, char *buf, size_t size, int flag )
{
	int32_t len, lest;
	char *ptr;
	for( ptr = buf, lest = size; lest > 0; ptr += len, lest -= len )
	{
		if( ( len = send( soc, ptr, lest, flag ) ) == -1 )
		{
#ifdef __WINDOWS__
			int err = WSAGetLastError();
			if (err != 0 && WSAGetLastError() != WSAEWOULDBLOCK /*&& WSAGetLastError() != WSA_INVALID_HANDLE*/) {
				return (-1);
			}
#else
			if (errno == 0) {
				//gdt_sleep(1);
			}
			//  && errno != EWOULDBLOCK
			else if( errno != EAGAIN  ){
				return (-1);
			}
#endif
			else{
				//gdt_sleep(1);
			}
			len = 0;
		}
#ifdef __QS_DEBUG__
		else{
			//printf( "send progress:%d/%zu\n", len, size );
			//if( lest-len > 0 ){
			//	gdt_sleep(1);
			//	printf( "send progress:%d/%zu\n", len, size );
			//}
		}
#endif
	}
	return (size);
}

ssize_t gdt_sendto_all(QS_SOCKET_ID soc, char *buf, size_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen )
{
	int32_t len, lest;
	char *ptr;
	for( ptr = buf, lest = size; lest > 0; ptr += len, lest -= len )
	{
		if( ( len = sendto( soc, ptr, lest, flag, pfrom, fromlen ) ) == -1 )
		//if( ( len = sendto( soc, ptr, lest, flag, NULL, 0 ) ) == -1 )
		{
#ifdef __WINDOWS__
			int err = WSAGetLastError();
			if (err != 0 && WSAGetLastError() != WSAEWOULDBLOCK /*&& WSAGetLastError() != WSA_INVALID_HANDLE*/) {
				return (-1);
			}
#else
			if (errno == 0) {
				//gdt_sleep(1);
			}
			//  && errno != EWOULDBLOCK
			else if (errno != EAGAIN) {
				return (-1);
			}
#endif
			else {
				//gdt_sleep(1);
			}
			len = 0;
		}
#ifdef __QS_DEBUG__
		else{
			//printf( "send progress:%d/%zu\n", len, size );
		}
#endif
	}
	return (size);
}

void gdt_set_sock_option( QS_SOCKET_OPTION *option )
{
	socklen_t len;
#ifdef __WINDOWS__
	(void) setsockopt(option->sockid, SOL_SOCKET, SO_RCVBUF, (const char*)&option->recvbuffer_size, sizeof(option->recvbuffer_size));
	(void)setsockopt(option->sockid, SOL_SOCKET, SO_SNDBUF, (const char*)&option->sendbuffer_size, sizeof(option->sendbuffer_size));
	len = sizeof(option->recvbuffer_size);
	if (getsockopt(option->sockid, SOL_SOCKET, SO_RCVBUF, (const char*)&option->recvbuffer_size, &len) == -1) {
		perror("getsockopt");
	}
	len = sizeof(option->sendbuffer_size);
	if (getsockopt(option->sockid, SOL_SOCKET, SO_SNDBUF, (const char*)&option->sendbuffer_size, &len) == -1) {
		perror("getsockopt");
	}
#else
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_RCVBUF, &option->recvbuffer_size, sizeof( option->recvbuffer_size ) );
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_SNDBUF, &option->sendbuffer_size, sizeof( option->sendbuffer_size ) );
	len = sizeof(option->recvbuffer_size);
	if (getsockopt(option->sockid, SOL_SOCKET, SO_RCVBUF, &option->recvbuffer_size, &len) == -1) {
		perror("getsockopt");
	}
	len = sizeof(option->sendbuffer_size);
	if (getsockopt(option->sockid, SOL_SOCKET, SO_SNDBUF, &option->sendbuffer_size, &len) == -1) {
		perror("getsockopt");
	}
#endif
//#ifdef __LINUX__
//	int flag = 0;
//	(void) setsockopt( option->sockid, IPPROTO_TCP, TCP_CORK, (const void*)flag, sizeof(int) );
//	int flag2 = 1;
//	(void) setsockopt( option->sockid, IPPROTO_TCP, TCP_NODELAY, (const void*)flag2, sizeof(int) );
//#endif

//#ifdef __WINDOWS__
//	struct linger l;
//	l.l_onoff=0;
//	l.l_linger=0;
//	(void) setsockopt( option->sockid, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof( l ) );
//#endif;

//#ifdef __QS_DEBUG__
//	printf( "default:SO_RCVBUF=%zd\n", option->recvbuffer_size );
//	printf( "default:SO_SNDBUF=%zd\n", option->sendbuffer_size );
//#endif
}

int gdt_set_block(QS_SOCKET_ID fd, int flag )
{
#ifdef __WINDOWS__
#else
	int flags;
#endif
	int error = 0;
#ifdef __WINDOWS__
	flag = !(flag);
	ioctlsocket(fd, FIONBIO, &flag);
#else
	if( ( flags = fcntl( fd, F_GETFL, 0 ) ) == -1 )
	{
		perror("fcntl");
		return -1;
	}
	// non blocking
	if( flag == 0 ){
		(void) fcntl( fd, F_SETFL, flags | O_NONBLOCK );
	}
	// blocking
	else{
		(void) fcntl( fd, F_SETFL, flags & ( ~O_NONBLOCK ) );
	}
#endif
	return error;
}

int gdt_get_sockaddr_info( QS_SOCKET_OPTION *option, struct sockaddr_storage *saddr, socklen_t *addr_len )
{
	char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct addrinfo hints, *res0;
	int errcode;
//#ifdef __QS_DEBUG__
//	printf( "gdt_get_sockaddr_info: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
//#endif
	(void) memset( &hints, 0, sizeof( hints ) );
	if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) == 0 ){
		hints.ai_family = AF_INET;
	}
	else{
		hints.ai_family = AF_INET6;
	}
	hints.ai_socktype = SOCK_STREAM;
	if( option->socket_type == SOCKET_TYPE_CLIENT_TCP )
	{
		hints.ai_socktype = SOCK_STREAM;
	}
	else if( option->socket_type == SOCKET_TYPE_CLIENT_UDP )
	{
		hints.ai_socktype = SOCK_DGRAM;
	}
	hints.ai_flags	= AI_PASSIVE;
	//hints.ai_flags = AI_NUMERICSERV;
	if( ( errcode = getaddrinfo( (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) , (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), &hints, &res0 ) ) != 0 ){
		printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
		return QS_SYSTEM_ERROR;
	}
	if( ( errcode = getnameinfo( res0->ai_addr, res0->ai_addrlen, 
		nbuf, sizeof( nbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV ) ) != 0 )
	{
		printf( "getnameinfo():%s\n",gai_strerror( errcode ) );
		freeaddrinfo( res0 );
		return QS_SYSTEM_ERROR;
	}
//#ifdef __QS_DEBUG__
//		printf( "gdt_get_sockaddr_info:addr=%s\n", nbuf );
//		printf( "gdt_get_sockaddr_info:port=%s\n", sbuf );
//#endif
	memcpy(saddr,res0->ai_addr,res0->ai_addrlen);
	*addr_len = res0->ai_addrlen;
	freeaddrinfo( res0 );
	return QS_SYSTEM_OK;
}

/*
 * create server socket
 * @param option
 * @return QS_SOCKET_ID( error : -1 )
 *
 * ※if bind error : bind: Address already in use
 * echo "1" > /proc/sys/net/ipv6/bindv6only
 */
QS_SOCKET_ID gdt_server_socket( QS_SOCKET_OPTION *option, int is_ipv6 )
{
	QS_SOCKET_ID sock = -1;
	char nbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	struct addrinfo hints;
	int opt , errcode;
	socklen_t opt_len;
	char* hostname = ( option->host_name_munit >= 0 ) ? (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) : NULL;
	char* port = ( option->port_num_munit >= 0 ) ? (char *)gdt_upointer( option->memory_pool,option->port_num_munit ) : NULL;

	memset( &hints, 0, sizeof( hints ) );
	if( is_ipv6 == 1 ){
		hints.ai_family   = AF_INET6;
	}
	else{
		hints.ai_family   = AF_INET;
	}
	hints.ai_socktype = SOCK_STREAM;
	if( option->socket_type == SOCKET_TYPE_SERVER_TCP )
	{
		hints.ai_socktype = SOCK_STREAM;
	}
	else if( option->socket_type == SOCKET_TYPE_SERVER_UDP )
	{
		hints.ai_socktype = SOCK_DGRAM;	
	}
	hints.ai_flags	= AI_PASSIVE;
	if( ( errcode = getaddrinfo( hostname , port, &hints, &option->addr ) ) != 0 ){
		printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
		return -1;
	}

	do{
		if( ( errcode = getnameinfo( option->addr->ai_addr, option->addr->ai_addrlen, 
			nbuf, sizeof( nbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV ) ) != 0 )
		{
			printf( "getnameinfo():%s\n",gai_strerror( errcode ) );
			break;
		}
		if( ( sock = socket( option->addr->ai_family, option->addr->ai_socktype, option->addr-> ai_protocol ) ) == -1 )
		{
			gdt_error("socket");
			break;
		}
		opt = 1;
		opt_len = sizeof( opt );
		if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len ) == -1 )
		{
			gdt_close_socket( &sock, "setsockopt" );
			break;
		}
		if( bind( sock, option->addr->ai_addr, option->addr->ai_addrlen) == -1 )
		{
			gdt_close_socket( &sock, "bind" );
			break;
		}
		if( option->socket_type == SOCKET_TYPE_SERVER_TCP )
		{
			if( listen( sock, SOMAXCONN ) == -1 )
			{
				gdt_close_socket( &sock, "listen" );
				break;
			}
		}
	}while( false );

	gdt_free_addrinfo(option);

	return ( sock );
}

QS_SOCKET_ID gdt_client_socket( QS_SOCKET_OPTION *option )
{
	QS_SOCKET_ID sock = -1;
	char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct addrinfo hints;
	int errcode;

	char* hostname = (char *)gdt_upointer( option->memory_pool,option->host_name_munit );
	char* portnum = (char *)gdt_upointer( option->memory_pool,option->port_num_munit );
	memset( &hints, 0, sizeof( hints ) );
	if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) == 0 ){
		hints.ai_family = AF_INET;
	}
	else{
		hints.ai_family = AF_INET6;
	}
	hints.ai_socktype = SOCK_STREAM;
	if( option->socket_type == SOCKET_TYPE_CLIENT_TCP )
	{
		hints.ai_socktype = SOCK_STREAM;
	}
	else if( option->socket_type == SOCKET_TYPE_CLIENT_UDP )
	{
		hints.ai_socktype = SOCK_DGRAM;
	}
	hints.ai_flags	= AI_PASSIVE; // AI_NUMERICSERV;
	if( ( errcode = getaddrinfo( hostname , portnum, &hints, &option->addr ) ) != 0 ){
		printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
		return -1;
	}
	if( ( errcode = getnameinfo( option->addr->ai_addr, option->addr->ai_addrlen, 
		nbuf, sizeof( nbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV ) ) != 0 )
	{
		printf( "getnameinfo():%s\n",gai_strerror( errcode ) );
		freeaddrinfo( option->addr );
		return -1;
	}
	if( ( sock = socket( option->addr->ai_family, option->addr->ai_socktype, option->addr->ai_protocol ) ) == -1 )
	{
		freeaddrinfo( option->addr );
		(void) gdt_error("socket");
		return -1;
	}
	if( option->socket_type == SOCKET_TYPE_CLIENT_UDP )
	{
		if( ( option->inetflag & INET_FLAG_BIT_CONNECT_UDP ) == 0 ){
			struct addrinfo *local_info;
			if( ( errcode = getaddrinfo( NULL , "0", &hints, &local_info ) ) != 0 ){
				gdt_close_socket( &sock, "getaddrinfo" );
				printf( "getaddrinfo() error : %s\n",gai_strerror( errcode ) );
			}
			if( bind( sock, (struct sockaddr *)local_info->ai_addr, sizeof(struct sockaddr)) == -1 )
			{
				gdt_close_socket( &sock, "bind" );
			}
			freeaddrinfo( local_info );
			freeaddrinfo( option->addr );
			return sock;
		}
	}

	if (option->t_sec <= 0 && option->t_usec <= 0)
	{
		if (connect(sock, option->addr->ai_addr, option->addr->ai_addrlen) == -1) {
			gdt_close_socket(&sock, "connect");
			freeaddrinfo(option->addr);
			return sock;
		}
		if (-1 == gdt_check_socket_error(sock))
		{
			gdt_close_socket(&sock, "connect");
			freeaddrinfo(option->addr);
			return sock;
		}
		freeaddrinfo(option->addr);
		return sock;
	}


	do {
		int width;
		struct timeval timeout;
		fd_set mask, write_mask, read_mask;
		int connectSuccess = false;
		(void)gdt_set_block(sock, 0);
		if (connect(sock, option->addr->ai_addr, option->addr->ai_addrlen) == -1)
		{
#ifdef __WINDOWS__
			int err = WSAGetLastError();
			if (err != 0 && err != WSAEWOULDBLOCK)
#else
			if (errno != EINPROGRESS && errno != EINTR)
#endif
			{
				gdt_close_socket(&sock, "select");
				freeaddrinfo(option->addr);
				break;
			}
		}
		else {
			(void)gdt_set_block(sock, 1);
			freeaddrinfo(option->addr);
			break;
		}
		width = 0;
		FD_ZERO(&mask);
		FD_SET(sock, &mask);
		width = (int)(sock + 1);
		timeout.tv_sec = option->t_sec;
		timeout.tv_usec = option->t_usec;
		for (;;)
		{
			write_mask = mask;
			read_mask = mask;
			fd_set* pwrite = &write_mask;
			if (option->wait_read == 1) {
				pwrite = NULL;
			}
			switch (select(width, &read_mask, pwrite, NULL, &timeout))
			{
			case -1:
				if (errno != EINTR)
				{
					gdt_close_socket(&sock, "select");
					freeaddrinfo(option->addr);
				}
				break;
			case 0:
				gdt_close_socket(&sock, "select:timeout");
				freeaddrinfo(option->addr);
				break;
			default:
				if ((pwrite != NULL && FD_ISSET(sock, pwrite)) || FD_ISSET(sock, &read_mask))
				{
					if (-1 == gdt_check_socket_error(sock))
					{
						gdt_close_socket(&sock, "connect");
					}
					else {
						(void)gdt_set_block(sock, 1);
						connectSuccess = true;
					}
					freeaddrinfo(option->addr);
				}
				break;
			}
			if (-1 == sock || connectSuccess == true) {
				break;
			}
		}
	} while (false);

	return sock;
}

QS_SOCKET_ID gdt_wait_client_socket(QS_SOCKET_ID sock,QS_SOCKET_OPTION *option)
{
	do {
		int width;
		struct timeval timeout;
		fd_set mask, write_mask, read_mask;
		int connectSuccess = false;
		(void)gdt_set_block(sock, 0);
		if (connect(sock, option->addr->ai_addr, option->addr->ai_addrlen) == -1)
		{
#ifdef __WINDOWS__
			int err = WSAGetLastError();
			if (err != 0 && err != WSAEWOULDBLOCK)
#else
			if (errno != EINPROGRESS && errno != EINTR)
#endif
			{
				gdt_close_socket(&sock, "select");
				freeaddrinfo(option->addr);
				break;
			}
		}
		else {
			(void)gdt_set_block(sock, 1);
			freeaddrinfo(option->addr);
			break;
		}
		width = 0;
		FD_ZERO(&mask);
		FD_SET(sock, &mask);
		width = (int)(sock + 1);
		timeout.tv_sec = option->t_sec;
		timeout.tv_usec = option->t_usec;
		for (;;)
		{
			write_mask = mask;
			read_mask = mask;
			fd_set* pwrite = &write_mask;
			if (option->wait_read == 1) {
				pwrite = NULL;
			}
			switch (select(width, &read_mask, pwrite, NULL, &timeout))
			{
			case -1:
				if (errno != EINTR)
				{
					gdt_close_socket(&sock, "select");
					freeaddrinfo(option->addr);
				}
				break;
			case 0:
				gdt_close_socket(&sock, "select:timeout");
				freeaddrinfo(option->addr);
				break;
			default:
				if ((pwrite != NULL && FD_ISSET(sock, pwrite)) || FD_ISSET(sock, &read_mask))
				{
					if (-1 == gdt_check_socket_error(sock))
					{
						gdt_close_socket(&sock, "connect");
					}
					else {
						(void)gdt_set_block(sock, 1);
						connectSuccess = true;
					}
					freeaddrinfo(option->addr);
				}
				break;
			}
			if (-1 == sock || connectSuccess == true) {
				break;
			}
		}
	} while (false);
	return sock;
}

int gdt_check_socket_error(QS_SOCKET_ID sock)
{
	int getopt_val;
	socklen_t len;
	len = sizeof( len );
	if( getsockopt( sock, SOL_SOCKET, SO_ERROR, &getopt_val, &len ) != -1 )
	{
		if( getopt_val == 0 )
		{
			return 0;
		}
		else{
#ifdef __WINDOWS__
			char errbuf[256];
			strerror_s(errbuf,255,getopt_val);
#else
			(void)fprintf(stderr, "getsockopt:%d:%s\n", getopt_val, strerror(getopt_val));
#endif
			return -1;
		}
	}
	return -1;
}

void gdt_free_addrinfo(QS_SOCKET_OPTION* option)
{
	if(option->addr==NULL){
		return;
	}
	freeaddrinfo(option->addr);
	option->addr = NULL;
}

void gdt_disconnect( QS_SOCKPARAM *psockparam )
{
	psockparam->c_status = PROTOCOL_STATUS_DEFAULT;
}

void* gdt_make_socket( QS_SOCKET_OPTION *option )
{
	if( option == NULL || option->memory_pool == NULL ){
		return NULL;
	}
	if( option->socket_type==SOCKET_TYPE_SERVER_TCP || option->socket_type==SOCKET_TYPE_SERVER_UDP){
		if( ( option->sockid = gdt_server_socket( option, 0 ) ) <= 0 ){
			printf( "gdt_server_socket error: port=%s, host=%s\n",
					(char *)gdt_upointer( option->memory_pool,option->port_num_munit ),
					(char *)gdt_upointer( option->memory_pool,option->host_name_munit )
				  );
			option->sockid=0;
			return NULL;
		}
		if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) != 0 ){
			if( ( option->sockid6 = gdt_server_socket( option, 1 ) ) <= 0 ){
				printf( "gdt_server_socket ipv6 error: port=%s, host=%s\n",
						(char *)gdt_upointer( option->memory_pool,option->port_num_munit ),
						(char *)gdt_upointer( option->memory_pool,option->host_name_munit )
					  );
				option->sockid6 = -1;
				return NULL;
			}
		}
		gdt_set_sock_option( option );
	}
	else if( option->socket_type==SOCKET_TYPE_CLIENT_TCP || option->socket_type==SOCKET_TYPE_CLIENT_UDP){
		if( ( option->sockid = gdt_client_socket( option ) ) <= 0 ){
			printf( "gdt_client_socket error: port=%s, host=%s\n",
				(char *)gdt_upointer( option->memory_pool,option->port_num_munit ),
				(char *)gdt_upointer( option->memory_pool,option->host_name_munit )
			);
			return NULL;
		}
		gdt_set_sock_option( option );
	}
	return NULL;
}

void* gdt_socket( QS_SOCKET_OPTION *option )
{
	if( option == NULL || option->memory_pool == NULL ){
		printf("empty memory error\n");
		return ( (void *) NULL );
	}
	switch( option->socket_type )
	{
		case SOCKET_TYPE_SERVER_TCP:
		case SOCKET_TYPE_SERVER_UDP:
			if( -1 == ( option->sockid = gdt_server_socket( option, 0 ) ) )
			{
				break;
			}
			if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) != 0 )
			{
				if( -1 == ( option->sockid6 = gdt_server_socket( option, 1 ) ) )
				{
					option->sockid6 = -1;
					gdt_close_socket(&option->sockid,NULL);
					break;
				}
			}
			gdt_set_sock_option( option );
			gdt_nonblocking_server(option);
			break;
		case SOCKET_TYPE_CLIENT_TCP:
		case SOCKET_TYPE_CLIENT_UDP:
			if( -1 == ( option->sockid = gdt_client_socket( option ) ) )
			{
				break;
			}
			gdt_set_sock_option( option );
			gdt_nonblocking_client(option);
			break;
		default:
			printf( "socket_type error\n" );
			break;
	}

	if(option->sockid==-1){
		printf( 
			"create socket error: port=%s, host=%s\n"
			, (char *)gdt_upointer( option->memory_pool,option->port_num_munit )
			, (char *)gdt_upointer( option->memory_pool,option->host_name_munit )
		);
	}

	return ( (void *) NULL );
}

/*
 * close socket handle
 */
void gdt_free_socket( QS_SOCKET_OPTION *option )
{
#ifdef __WINDOWS__
	if( option->sockid > -1 ){
		shutdown(option->sockid, SD_BOTH);
		closesocket( option->sockid );
	}
	if( option->mode == SOCKET_MODE_THREAD )
	{
		CloseHandle(option->accept_lock);
	}
	WSACleanup();
#else
	if( option->lock_file_fd >= 0 ){
		(void) close( option->lock_file_fd );
		option->lock_file_fd = -1;
	}
	if( option->sockid > -1 ){
		(void) close( option->sockid );
	}
	if( option->mode == SOCKET_MODE_THREAD )
	{
		(void) pthread_mutex_destroy( &option->accept_lock );
	}
#endif
	option->sockid = -1;
}

void gdt_recv_event(QS_SOCKET_OPTION *option, QS_SERVER_CONNECTION_INFO *child, socklen_t srlen)
{
	QS_RECV_INFO *rinfo;
	if (srlen == -1) {
		return;
	}
	if (srlen == 0) {
#ifdef __WINDOWS__
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			//perror("recv 0byte");
			gdt_close_socket(&child->id, NULL);
		}
#else
		if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK){
			//perror("recv 0byte");
		}
		gdt_close_socket(&child->id, NULL);
#endif
		return;
	}
	if (option->socket_type == SOCKET_TYPE_CLIENT_UDP || option->socket_type == SOCKET_TYPE_SERVER_UDP){
		if( 0 != getnameinfo((struct sockaddr *) &child->sockparam.from, child->sockparam.fromlen,child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV) )
		{
			printf("getnameinfo error.\n");
			return;
		}
	}
	rinfo = (struct QS_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
	child->sockparam.acc = child->id;
	
	if(option->socket_type == SOCKET_TYPE_SERVER_UDP){
		child->sockparam.acc = option->sockid;
	}
	
	rinfo->tinfo = child;
	size_t old_pos = 0;
	do{
		rinfo->recvlen = srlen;
		rinfo->recvbuf_munit = child->recvbuf_munit;
		rinfo->recvfrom = child->id;
		switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
		{
		case -1:
			//printf("gdt_recv_event error\n");
			gdt_close_socket(&child->id, NULL);
			break;
		case 1:
			if (option->plane_recv_callback != NULL){
				option->plane_recv_callback((void*)rinfo);
			}
			else if (option->payload_recv_callback != NULL){
				option->payload_recv_callback(child->sockparam.payload_type, (uint8_t*)QS_GET_POINTER(option->memory_pool, rinfo->recvbuf_munit), rinfo->recvlen, rinfo);
			}
			if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT){
					gdt_close_socket(&child->id, NULL);
			}
			break;
		}
		*((char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
		//memset((char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
		if( child->sockparam.continue_pos > 0 ){
			if( old_pos >= child->sockparam.continue_pos ){
				printf("invalid packet\n");
				break;
			}
			old_pos = child->sockparam.continue_pos;
		}
	}while( child->sockparam.continue_pos > 0 && -1 != child->id );
}

void gdt_nonblocking_server(QS_SOCKET_OPTION *option)
{
	if (option->socket_type == SOCKET_TYPE_SERVER_UDP)
	{
		if (option->maxconnection > 1) {
			printf("change udp maxconnection %zd -> 1\n", option->maxconnection);
			option->maxconnection = 1;
		}
	}
	if( -1 == gdt_make_connection_info( option ) ){
		printf("gdt_make_connection_info error.");
		return;
	}
	gdt_set_block(option->sockid, 0);
	if (option->sockid6 != -1) {
		gdt_set_block(option->sockid6, 0);
	}
}

void gdt_server_update(QS_SOCKET_OPTION *option)
{
	if (option->mode != SOCKET_MODE_NONBLOCKING) {
		return;
	}
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	socklen_t srlen;
	QS_SERVER_CONNECTION_INFO *child;
	socklen_t len;
	if (option->socket_type == SOCKET_TYPE_SERVER_TCP)
	{
		QS_SOCKET_ID acc;
		int i;
		struct sockaddr_storage from;
		len = (socklen_t) sizeof(from);
		if (-1 == (acc = accept(option->sockid, (struct sockaddr *) &from, &len))) {
			//if (errno != 0 && errno != EINTR && errno != EAGAIN) {
#ifdef __WINDOWS__
			if (errno != 0 && errno != EAGAIN && errno != ENOENT && WSAGetLastError() != WSAEWOULDBLOCK)
#else
			if (errno != 0 && errno != EAGAIN)
#endif
			{
				perror("accept");
			}
		}
		if (acc == -1 && option->sockid6 != -1) {
			if (-1 == (acc = accept(option->sockid6, (struct sockaddr *) &from, &len))) {
				//if (errno != 0 && errno != EINTR && errno != EAGAIN) {
#ifdef __WINDOWS__
				if (errno != 0 && errno != EAGAIN && errno != ENOENT && WSAGetLastError() != WSAEWOULDBLOCK)
#else
				if (errno != 0 && errno != EAGAIN)
#endif
				{
					perror("accept");
				}
			}
		}
		if (acc != -1)
		{
			for (i = 0; i < option->maxconnection; i++)
			{
				child = (QS_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), i);
				if (child->id == -1)
				{
					if ( 0 != getnameinfo((struct sockaddr *) &from, len, child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV) )
					{
						printf( "getnameinfo error.\n" );
						break;
					}
					child->id = acc;
					child->sockparam.acc = child->id;
					if (option->socket_type == SOCKET_TYPE_SERVER_UDP){
						child->sockparam.type = SOCK_TYPE_NORMAL_UDP;
					}
					else{
						child->sockparam.type = SOCK_TYPE_NORMAL_TCP;
					}
					gdt_set_block(child->id, 0);
					gdt_initialize_connection_info(option, child);
					if (option->connection_start_callback != NULL){
						option->connection_start_callback((void*)child);
					}
					break;
				}
				else{
					if( i == option->maxconnection-1 ){
						printf("connection is full\n");
						gdt_close_socket(&acc, NULL);
					}
				}
			}
		}
		for (i = 0; i < option->maxconnection; i++)
		{
			child = (QS_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), i);
			if (child->id != -1)
			{
				if (option->user_recv_function != NULL) {
					srlen = option->user_recv_function(child, child->id, (char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0);
				}
				else {
					srlen = recv(child->id, (char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0);
				}
				
				if(-1==srlen){
#ifdef __WINDOWS__
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						perror("recv");
						gdt_close_socket(&child->id, NULL);
					}
#else
					if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK){
						perror("recv");
						gdt_close_socket(&child->id, NULL);
					}
#endif
				}
				gdt_recv_event(option,child,srlen);
				if (child->id == -1)
				{
					if (option->close_callback != NULL)
					{
						option->close_callback(child);
					}
					gdt_free_sockparam(option, &child->sockparam);
				}
			}
		}
	}
	else if (option->socket_type == SOCKET_TYPE_SERVER_UDP)
	{
		child = (QS_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), 0);
		child->sockparam.fromlen = sizeof(child->sockparam.from);
		if ((srlen = recvfrom(option->sockid, (char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0, (struct sockaddr *)&child->sockparam.from, &child->sockparam.fromlen)) == -1)
		{
			if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				perror("recvfrom");
			}
			return;
		}
		gdt_recv_event(option,child,srlen);
	}
}

void gdt_nonblocking_client(QS_SOCKET_OPTION *option)
{
	if(-1==option->sockid){
		return;
	}
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	gdt_set_block(option->sockid, 0);
	if( -1 == ( option->connection_munit = gdt_create_munit( option->memory_pool, sizeof( QS_SERVER_CONNECTION_INFO ), MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	QS_SERVER_CONNECTION_INFO * child = (QS_SERVER_CONNECTION_INFO*)QS_GET_POINTER(option->memory_pool,option->connection_munit);
	child->id = option->sockid;
	if( -1 == ( child->recvbuf_munit = gdt_create_munit( option->memory_pool, buffer_size, MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	if( -1 == ( child->recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( QS_RECV_INFO ), MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	if( -1 == ( child->recvmsg_munit = gdt_create_munit( option->memory_pool, sizeof( char) * ( option->msgbuffer_size ), MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	child->gdt_socket_option = option;
	child->index = 0;
	gdt_init_socket_param( &child->sockparam );
	child->sockparam.acc = option->sockid;
	if (option->socket_type == SOCKET_TYPE_CLIENT_TCP){
		child->sockparam.type = SOCK_TYPE_NORMAL_TCP;
	}
	else {
		child->sockparam.type = SOCK_TYPE_NORMAL_UDP;
	}
	if( option->connection_start_callback != NULL ){
		option->connection_start_callback( (void*)child );
	}
}

void gdt_client_update(QS_SOCKET_OPTION *option)
{
	if(option==NULL){
		return;
	}
	if(option->sockid==-1){
		return;
	}
	if (option->mode != SOCKET_MODE_CLIENT_NONBLOCKING) {
		return;
	}
	if( option->connection_munit == -1 )
	{
		return;
	}
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	socklen_t srlen;
	QS_SERVER_CONNECTION_INFO* child = (QS_SERVER_CONNECTION_INFO*)QS_GET_POINTER(option->memory_pool,option->connection_munit);
	if (option->socket_type == SOCKET_TYPE_CLIENT_TCP)
	{
		if (child->id != -1)
		{
			if (option->user_recv_function != NULL) {
				srlen = option->user_recv_function(child, child->id, (char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0);
			}
			else {
				srlen = recv(child->id, (char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0);
			}
			if(-1 == srlen){
#ifdef __WINDOWS__
				int error_id = WSAGetLastError();
				if (error_id != WSAEWOULDBLOCK) {
					printf("error : %d\n",error_id);
					perror("recv");
					gdt_close_socket(&child->id, NULL);
				}
#else
				if (errno != 0 && errno != EAGAIN)
				{
					perror("recv");
					gdt_close_socket(&child->id, NULL);
				}
#endif
			}
			gdt_recv_event(option,child,srlen);
			if (child->id == -1)
			{
				gdt_free_sockparam(option, &child->sockparam);
				if (option->close_callback != NULL)
				{
					option->close_callback(child);
				}
			}
		}
	}
	else if (option->socket_type == SOCKET_TYPE_CLIENT_UDP)
	{
		if (child->id == -1){
			return;
		}
		child = (QS_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), 0);
		child->sockparam.fromlen = sizeof(child->sockparam.from);
		if ((srlen = recvfrom(option->sockid, (char*)QS_GET_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0, (struct sockaddr *)&child->sockparam.from, &child->sockparam.fromlen)) == -1){
			if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				perror("recvfrom");
			}
			return;
		}
		gdt_recv_event(option,child,srlen);
	}
}

int gdt_client_is_connecting(QS_SOCKET_OPTION *option)
{
	if( option->sockid == -1 ){
		return 0;
	}
	if( option->connection_munit == -1 ){
		return 0;
	}
	QS_SERVER_CONNECTION_INFO* child = (QS_SERVER_CONNECTION_INFO*)QS_GET_POINTER(option->memory_pool,option->connection_munit);
	if (child->id == -1){
		return 0;
	}
	return 1;
}

void gdt_print_payload(uint32_t payload_type, uint8_t* payload, size_t payload_len,size_t view_max)
{
	printf("(payload_type:%d,payload_len:%d)", (int)payload_type, (int)payload_len);
	gdt_print_hex( payload, payload_len, view_max );
}

int gdt_pre_packetfilter( QS_SOCKET_OPTION *option, struct QS_RECV_INFO *rinfo, QS_SOCKPARAM *psockparam, int32_t recvmsg_munit )
{
	int ret = 0;
	char *pbuf;
	char *msgpbuf;
	ssize_t _tmpmsglen;
	pbuf = (char*)gdt_upointer( option->memory_pool, rinfo->recvbuf_munit );
	if( rinfo->recvlen > 0 )
	{
		if( option->socket_type == SOCKET_TYPE_SERVER_UDP || option->socket_type == SOCKET_TYPE_CLIENT_UDP )
		{
			psockparam->type = SOCK_TYPE_NORMAL_UDP;
			psockparam->c_status = PROTOCOL_STATUS_OTHER;
		}
		else{
			psockparam->type = SOCK_TYPE_NORMAL_TCP;
			psockparam->c_status = PROTOCOL_STATUS_OTHER;
		}
		if( option->protocol == PROTOCOL_SIMPLE )
		{
			pbuf[rinfo->recvlen] = '\0';
			_tmpmsglen = gdt_parse_socket_binary( option, psockparam, (uint8_t*)pbuf, rinfo->recvlen, recvmsg_munit );
			msgpbuf = (char*)gdt_upointer( option->memory_pool, recvmsg_munit );
			if( _tmpmsglen < 0 ){
				ret = -1;
			}
			else{
				if( psockparam->tmpmsglen == 0 && psockparam->fin == 1 )
				{
					rinfo->recvlen = _tmpmsglen;
					msgpbuf[rinfo->recvlen] = '\0';
					rinfo->recvbuf_munit = recvmsg_munit;
					ret = 1;
				}
			}
		}else{
			ret = 1;
		}
	}
	return ret;
}

uint64_t get_parse_header(QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size)
{
	uint8_t* header = u8buf;
	size_t header_size = size;
	uint8_t cp = 1;
	size_t tmp_size = 0;
	if(psockparam->header_size>0){
		tmp_size=psockparam->header_size;
		size_t cp_size = size;
		if(cp_size>(sizeof(psockparam->header)-psockparam->header_size)){
			cp_size = sizeof(psockparam->header)-psockparam->header_size;
		}
		memcpy(&psockparam->header[psockparam->header_size],u8buf,cp_size);
		psockparam->header_size+=cp_size;
		header=psockparam->header;
		header_size=psockparam->header_size;
		cp=0;
	}
	do{
		psockparam->maskindex = 32; // max size(32byte)
		if(header_size<2){
			psockparam->fin=0;
			break;
		}
		psockparam->fin=0; // header[0] >> 7;
		psockparam->rsv = ( header[0] & 0x70 ) >> 4;
		psockparam->opcode = ( header[0] & 0x0f );
		psockparam->mask = header[1] >> 7;
		psockparam->ckpayloadlen = ( header[1] & 0x7f );
		if( psockparam->opcode == 8 ){
			return QS_SYSTEM_ERROR;
		}
		if(psockparam->ckpayloadlen < 126 ){
			psockparam->maskindex = 6;
			psockparam->payloadlen = psockparam->ckpayloadlen;
		}
		else if(psockparam->ckpayloadlen < 127 ){
			psockparam->maskindex = 8;
			if(header_size<4){
				break;
			}
			psockparam->payloadlen |= header[2] << 8;
			psockparam->payloadlen |= header[3] << 0;
		}
		else{
			psockparam->maskindex = 14;
			if(header_size<10){
				break;
			}
			psockparam->payloadlen |= (uint64_t)header[2] << 56;
			psockparam->payloadlen |= (uint64_t)header[3] << 48;
			psockparam->payloadlen |= (uint64_t)header[4] << 40;
			psockparam->payloadlen |= (uint64_t)header[5] << 32;
			psockparam->payloadlen |= (uint64_t)header[6] << 24;
			psockparam->payloadlen |= (uint64_t)header[7] << 16;
			psockparam->payloadlen |= (uint64_t)header[8] << 8;
			psockparam->payloadlen |= (uint64_t)header[9] << 0;
		}
		psockparam->maskindex = psockparam->maskindex - tmp_size;
	}while(false);
	if( cp==1 ){
		size_t cp_size = size;
		if(cp_size>(sizeof(psockparam->header)-psockparam->header_size)){
			cp_size = sizeof(psockparam->header)-psockparam->header_size;
		}
		memcpy(&psockparam->header[psockparam->header_size],u8buf,cp_size);
		psockparam->header_size+=cp_size;
		return QS_SYSTEM_OK;
	}
	return QS_SYSTEM_OK;
}

ssize_t gdt_parse_socket_binary( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit )
{
	int i, startpos;
	int64_t cnt = 0;
	char* msg = (char*)gdt_upointer( option->memory_pool,basebuf_munit );
	ssize_t retsize = -1;
	size_t tmp_continue_pos = psockparam->continue_pos;
	do{
		if( psockparam->header_size<psockparam->maskindex || ( psockparam->fin == 1 && psockparam->tmpmsglen == 0 ) ){
			if( psockparam->continue_pos > 0 ){
				u8buf+=psockparam->continue_pos;
				size-=psockparam->continue_pos;
				psockparam->continue_pos = 0;
			}
			psockparam->payloadlen = 0;
			if(QS_SYSTEM_ERROR==get_parse_header(option, psockparam, u8buf, size)){
				psockparam->payloadlen = -1;
				break;
			}
			if( psockparam->payloadlen >= gdt_usize( option->memory_pool, basebuf_munit ) ){
				psockparam->payloadlen = -1;
				break;
			}
			if( size < psockparam->maskindex ){
				retsize=0;
				break;
			}
			psockparam->payloadmask = 0x00000000;
			startpos = psockparam->maskindex;
			if( option->memory_pool->endian == QS_LITTLE_ENDIAN ){
				psockparam->payload_type = BYTE_SWAP_BIT32( *( (uint32_t*)(u8buf+psockparam->maskindex-4) ) );
			}
			else{
				psockparam->payload_type = *( (uint32_t*)(u8buf+psockparam->maskindex-4) );
			}
		}
		else{
			startpos = 0;
			cnt = psockparam->tmpmsglen;
		}
		for( i = startpos; i < size; i++ )
		{
			msg[cnt++] = u8buf[i];
			if( cnt == psockparam->payloadlen && i != size-1 ){
				//printf("out of range : %d, %d, %d, %d\n",(int)startpos,(int)psockparam->payloadlen,(int)size, i);
				psockparam->continue_pos += tmp_continue_pos+(i+1);
				break;
			}
		}
		psockparam->tmpmsglen += ( cnt - psockparam->tmpmsglen );
		if( psockparam->tmpmsglen >= psockparam->payloadlen )
		{
			psockparam->fin					= 1;
			psockparam->rsv					= 0;
			psockparam->opcode				= 0;
			psockparam->mask				= 0;
			psockparam->ckpayloadlen		= 0;
			psockparam->maskindex			= 0;
			psockparam->payloadmask			= 0;
			psockparam->tmpmsglen			= 0;
			psockparam->tmpbitsift			= -1;
			psockparam->appdata32bit = 0x00000000;
			msg[psockparam->payloadlen] = '\0';
			retsize = psockparam->payloadlen;
			psockparam->header_size = 0;
			memset(psockparam->header,0,sizeof(psockparam->header));
		}
		else{
			msg[psockparam->tmpmsglen] = '\0';
			retsize = psockparam->tmpmsglen;
			if( psockparam->tmpmsglen == 0 ){
				psockparam->fin = 0;
			}
		}
	}while( false );
	return retsize;
}

uint32_t gdt_make_size_header(uint8_t* head_ptr,ssize_t size)
{
	uint32_t headersize = 0;
	if( size < 125 ){
		headersize = 6;
		if( head_ptr != NULL ){
			*head_ptr = 0x00 | (uint8_t)size;
		}
	}
	else if( size > 126 && size <= 65536 ){
		headersize = 8;
		if( head_ptr != NULL ){
			*head_ptr = 0x00 | 126;
		}
	}
	else{
		headersize = 14;
		if( head_ptr != NULL ){
			*head_ptr = 0x00 | 127;
		}
	}
	return headersize;
}

int32_t gdt_make_message_buffer(QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, size_t size)
{
	size_t tmp_size = size + gdt_make_size_header(NULL,size);
	if( -1 == psockparam->buf_munit || gdt_usize( option->memory_pool, psockparam->buf_munit ) <= tmp_size ){
		if( psockparam->buf_munit >= 0 ){
			gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
		}
		if( ( psockparam->buf_munit = gdt_create_munit( option->memory_pool, sizeof( uint8_t ) * QS_ALIGNUP( tmp_size, option->msgbuffer_size ), MEMORY_TYPE_DEFAULT ) ) == -1 )
		{
			printf( "[gdt_make_message_buffer]size over: %zd byte\n", size );
			return QS_SYSTEM_ERROR;
		}
	}
	return QS_SYSTEM_OK;
}

size_t gdt_make_msg( QS_SOCKET_OPTION* option, QS_SOCKPARAM* psockparam, void* sendbin, const char* msg, ssize_t size, uint32_t payload_type,int is_binary )
{
	uint8_t head1;
	uint8_t head2;
	uint8_t *ptr;
	char* cptr;
	uint32_t headersize = 0;
	head1 = 0x80 | ( (is_binary) ? 0x02 : 0x01 );
	headersize = gdt_make_size_header(&head2,size);
	ptr = (uint8_t*) sendbin;
	*ptr = 0;
	*(ptr+1) = 0;
	*(ptr++) |= head1;
	*(ptr++) |= head2;
	if( headersize == 8 ){
		*(ptr++) = (uint8_t)(size >> 8);
		*(ptr++) = (uint8_t)(size & 0x00ff);
	}
	else if( headersize == 14 ){
		*(ptr++) = (uint8_t)(( size & 0xff00000000000000) >> 56);
		*(ptr++) = (uint8_t)(( size & 0x00ff000000000000) >> 48);
		*(ptr++) = (uint8_t)(( size & 0x0000ff0000000000) >> 40);
		*(ptr++) = (uint8_t)(( size & 0x000000ff00000000) >> 32);
		*(ptr++) = (uint8_t)(( size & 0x00000000ff000000) >> 24);
		*(ptr++) = (uint8_t)(( size & 0x0000000000ff0000) >> 16);
		*(ptr++) = (uint8_t)(( size & 0x000000000000ff00) >> 8);
		*(ptr++) = (uint8_t)(( size & 0x00000000000000ff) >> 0);
	}
	MEMORY_PUSH_BIT32_B( option->memory_pool, ptr, payload_type );
	cptr = (char*)ptr;
	memcpy( cptr, msg, size );
	return (size_t) ( ( sizeof( uint8_t ) * size ) + headersize );
}

ssize_t gdt_send_msg( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type )
{
	int is_binary = 0;
	ssize_t len = 0;
	void *sendbin = NULL;
	if(QS_SYSTEM_ERROR == gdt_make_message_buffer(option,psockparam,size)){
		return 0;
	}
	sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
	size_t binsize = gdt_make_msg(option,psockparam,sendbin,msg,size,payload_type,is_binary);
	len = gdt_send_all( psockparam->acc, (char*)sendbin, binsize, 0 );
	return len;
}

size_t gdt_make_udpmsg( void* sendbin, const char* msg, ssize_t size, uint32_t payload_type )
{
	uint8_t head1;
	uint8_t head2;
	uint8_t *ptr;
	char* cptr;
	uint32_t headersize = 0;
	head1 = 0x80;
	headersize = gdt_make_size_header(&head2,size);
	ptr = (uint8_t*) sendbin;
	*ptr = 0;
	*(ptr+1) = 0;
	*(ptr++) |= head1;
	*(ptr++) |= head2;
	if( headersize == 8 ){
		*(ptr++) = size >> 8;
		*(ptr++) = size & 0x00ff;
	}
	else if( headersize == 14 ){
		*(ptr++) = ( size & 0xff00000000000000) >> 56;
		*(ptr++) = ( size & 0x00ff000000000000) >> 48;
		*(ptr++) = ( size & 0x0000ff0000000000) >> 40;
		*(ptr++) = ( size & 0x000000ff00000000) >> 32;
		*(ptr++) = ( size & 0x00000000ff000000) >> 24;
		*(ptr++) = ( size & 0x0000000000ff0000) >> 16;
		*(ptr++) = ( size & 0x000000000000ff00) >> 8;
		*(ptr++) = ( size & 0x00000000000000ff) >> 0;
	}
	MEMORY_PUSH_BIT32_B2( gdt_endian(), ptr, payload_type );
	cptr = (char*)ptr;
	memcpy( cptr, msg, size );
	return (size_t) ( ( sizeof( char ) * size ) + headersize );
}

ssize_t gdt_send_udpmsg( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type, struct sockaddr *pfrom, socklen_t fromlen  )
{
	void *sendbin;
	ssize_t len = 0;
	if(QS_SYSTEM_ERROR == gdt_make_message_buffer(option,psockparam,size)){
		return 0;
	}
	sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
	size_t send_size = gdt_make_udpmsg( sendbin, msg, size, payload_type );
	len = gdt_sendto_all( psockparam->acc, (char*)sendbin, send_size, 0, pfrom, fromlen );
	return len;
}
