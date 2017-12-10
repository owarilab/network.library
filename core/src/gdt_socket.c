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

#include "gdt_socket.h"

GDT_SOCKET_OPTION* gdt_create_tcp_server(char* hostname, char* portnum)
{
	GDT_MEMORY_POOL* memory_pool = NULL;
	GDT_SOCKET_OPTION *option;
	size_t maxconnection = 1024;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 128, SIZE_MBYTE * 128, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		printf("gdt_initialize_memory error\n");
		return NULL;
	}
	int32_t option_munit = gdt_create_munit( memory_pool, sizeof( GDT_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
	if( option_munit == -1 ){
		return NULL;
	}
	option = (GDT_SOCKET_OPTION*)GDT_POINTER(memory_pool,option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

GDT_SOCKET_OPTION* gdt_create_udp_server(char* hostname, char* portnum)
{
	GDT_MEMORY_POOL* memory_pool = NULL;
	GDT_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 1, SIZE_MBYTE * 1, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 1) <= 0) {
		printf("gdt_initialize_memory error\n");
		return NULL;
	}
	int32_t option_munit = gdt_create_munit( memory_pool, sizeof( GDT_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
	if( option_munit == -1 ){
		return NULL;
	}
	option = (GDT_SOCKET_OPTION*)GDT_POINTER(memory_pool,option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_SERVER_UDP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)){
		return NULL;
	}
	return option;
}

GDT_SOCKET_OPTION* gdt_create_tcp_client(char* hostname, char* portnum)
{
	GDT_MEMORY_POOL* memory_pool = NULL;
	GDT_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		printf("gdt_initialize_memory error\n");
		return NULL;
	}
	int32_t option_munit = gdt_create_munit(memory_pool, sizeof(GDT_SOCKET_OPTION), MEMORY_TYPE_DEFAULT);
	if (option_munit == -1) {
		return NULL;
	}
	option = (GDT_SOCKET_OPTION*)GDT_POINTER(memory_pool, option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_CLIENT_TCP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

GDT_SOCKET_OPTION* gdt_create_udp_client(char* hostname, char* portnum)
{
	GDT_MEMORY_POOL* memory_pool = NULL;
	GDT_SOCKET_OPTION *option;
	size_t maxconnection = 1;
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		printf("gdt_initialize_memory error\n");
		return NULL;
	}
	int32_t option_munit = gdt_create_munit(memory_pool, sizeof(GDT_SOCKET_OPTION), MEMORY_TYPE_DEFAULT);
	if (option_munit == -1) {
		return NULL;
	}
	option = (GDT_SOCKET_OPTION*)GDT_POINTER(memory_pool, option_munit);
	if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_CLIENT_UDP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
		return NULL;
	}
	return option;
}

ssize_t gdt_send_message(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	ssize_t ret = 0;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
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

ssize_t gdt_send_message_broadcast(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option != NULL ){
		return gdt_send_broadcast( option, payload, payload_len, payload_type );
	}
	return 0;
}

ssize_t gdt_send_message_multicast(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info, GDT_MEMORY_POOL* array_memory, int32_t array_munit)
{
	ssize_t ret = 0;
	if( array_munit == -1 ){
		return ret;
	}
	GDT_SOCKET_OPTION *option = (GDT_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option->connection_munit <= 0 ){
		return ret;
	}
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int i;
	char* pbuf;
	parray = (GDT_ARRAY*)GDT_POINTER( array_memory, array_munit );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( array_memory, parray->munit );
	for( i = 0; i < parray->len; i++ )
	{
		pbuf = (char*)GDT_POINTER(array_memory,(elm+i)->munit);
		if( strcmp(pbuf,"") ){
			int offset = atoi(pbuf);
			GDT_SERVER_CONNECTION_INFO *tmptinfo;
			if( offset >= 0 && offset < option->maxconnection )
			{
				tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), offset );
				if( tmptinfo->sockparam.acc > 0 )
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

ssize_t gdt_client_send_message(uint32_t payload_type, char* payload, size_t payload_len, GDT_SOCKET_OPTION *option)
{
	ssize_t ret = 0;
	if( option == NULL ){
		return ret;
	}
	if( option->connection_munit != -1 )
	{
		GDT_SERVER_CONNECTION_INFO* gdt_connection_info = (GDT_SERVER_CONNECTION_INFO*)GDT_POINTER(option->memory_pool, option->connection_munit);
		if( gdt_connection_info != NULL && gdt_connection_info->sockparam.acc != -1 ){
			if( -1 == ( ret = gdt_send(option, &gdt_connection_info->sockparam, payload, payload_len, payload_type) ) ){
				if( option->close_callback != NULL ){
					option->close_callback( gdt_connection_info );
				}
				gdt_free_sockparam( option, &gdt_connection_info->sockparam );
			}
		}
	}
	return ret;
}

int32_t gdt_set_client_id(GDT_SOCKET_OPTION *option,uint32_t id)
{
	if( option == NULL ){
		return GDT_SYSTEM_ERROR;
	}
	if( option->connection_munit != -1 )
	{
		GDT_SERVER_CONNECTION_INFO* gdt_connection_info = (GDT_SERVER_CONNECTION_INFO*)GDT_POINTER(option->memory_pool, option->connection_munit);
		if( gdt_connection_info != NULL && gdt_connection_info->sockparam.acc != -1 ){
			gdt_connection_info->index = id;
			return GDT_SYSTEM_OK;
		}
	}
	return GDT_SYSTEM_ERROR;
}

int gdt_initialize_socket_option( 
	  GDT_SOCKET_OPTION *option
	, char* hostname
	, char* portnum
	, uint8_t socket_type
	, uint8_t mode
	, uint8_t protocol
	, size_t maxconnection
	, GDT_MEMORY_POOL* memory_pool
	, GDT_MEMORY_POOL* mmap_memory_pool
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
	option->t_sec				= 0;	// default 0sec
	option->t_usec				= 0;	// default 0usec
	option->s_sec				= 5;	// default 5sec
	option->s_usec				= 0;	// default 0usec
	option->connection_start_callback	= NULL;
	option->send_finish_callback		= NULL;
	option->plane_recv_callback				= NULL;
	option->payload_recv_callback = NULL;
	option->close_callback				= NULL;
	option->timeout_callback			= NULL;
	option->user_recv_function			= NULL;
	option->user_send_function			= NULL;
	option->recvbuffer_size				= SIZE_KBYTE*16;
	option->sendbuffer_size				= SIZE_KBYTE*16;
	option->msgbuffer_size				= SIZE_KBYTE*8;
	option->connection_munit			= -1;
	if( hostname == NULL ){
		option->host_name_munit			= -1;
	}
	else{
		if( ( option->host_name_munit	= gdt_create_munit( memory_pool, SIZE_BYTE * ( 128 ), MEMORY_TYPE_DEFAULT ) ) > 0 )
		{
			(void)snprintf( (char *)gdt_upointer( memory_pool, option->host_name_munit ), gdt_usize( memory_pool, option->host_name_munit ), "%s", hostname );
		}
	}
	if( portnum == NULL ){
		option->port_num_munit		= -1;
	}else{
		if( ( option->port_num_munit	= gdt_create_munit( memory_pool, SIZE_BYTE * ( 128 ), MEMORY_TYPE_DEFAULT ) ) > 0 )
		{
			(void) snprintf( (char *)gdt_upointer( memory_pool, option->port_num_munit ), gdt_usize( memory_pool, option->port_num_munit ), "%s", portnum );
		}
	}
	option->lock_file_fd		= -1;
	option->lock_file_munit		= -1;
	option->memory_pool			= memory_pool;
	option->mmap_memory_pool	= mmap_memory_pool;
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

void set_on_connect_event( GDT_SOCKET_OPTION *option, GDT_CONNECTION_EVENT_CALLBACK func )
{
	option->connection_start_callback = func;
}
void set_on_sent_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func )
{
	option->send_finish_callback = func;
}
void set_on_packet_recv_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func )
{
	option->plane_recv_callback = func;
}
void set_on_payload_recv_event( GDT_SOCKET_OPTION *option, GDT_ON_RECV func )
{
	option->payload_recv_callback = func;
}
void set_on_close_event( GDT_SOCKET_OPTION *option, GDT_CONNECTION_EVENT_CALLBACK func )
{
	option->close_callback = func;
}
void set_user_recv_event( GDT_SOCKET_OPTION *option, GDT_USER_RECV func )
{
	option->user_recv_function = func;
}
void set_user_send_event(GDT_SOCKET_OPTION *option, GDT_USER_SEND func)
{
	option->user_send_function = func;
}
void gdt_set_timeout_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func )
{
	option->timeout_callback = func;
}
void gdt_set_connection_timeout( GDT_SOCKET_OPTION *option, int32_t sec, int32_t usec )
{
	option->t_sec = sec;
	option->t_usec = usec;
}
void gdt_set_select_timeout( GDT_SOCKET_OPTION *option, int32_t sec, int32_t usec )
{
	option->s_sec = sec;
	option->s_usec = usec;
}
void set_message_buffer( GDT_SOCKET_OPTION *option, size_t buffer_size)
{
	option->msgbuffer_size = buffer_size;
}

void gdt_init_socket_param( GDT_SOCKPARAM *psockparam )
{
	psockparam->http_header_munit	= -1;
	psockparam->wsockkey_munit		= -1;
	psockparam->c_status			= PROTOCOL_STATUS_DEFAULT;
	psockparam->acc					= -1;
	psockparam->type				= 0;
	psockparam->phase				= 0;
	psockparam->ws_msg_mode			= 1;// // ( 1: text, 2 : binary )
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
	//psockparam->fromlen = sizeof(psockparam->from);
}

void gdt_free_sockparam( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam )
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
	if( option->memory_pool != NULL && psockparam->buf_munit >= 0 ){
		gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
	}
	psockparam->buf_munit			= -1;
}

int gdt_close_socket(GDT_SOCKET_ID* sock, char* error )
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

int32_t gdt_make_connection_info( GDT_SOCKET_OPTION *option )
{
	option->connection_munit = gdt_create_munit( option->memory_pool, sizeof( GDT_SERVER_CONNECTION_INFO ) * ( option->maxconnection ), MEMORY_TYPE_DEFAULT );
	if( option->connection_munit > 0 )
	{
		int i;
		GDT_SERVER_CONNECTION_INFO * child;
		for( i = 0; i < option->maxconnection; i++ )
		{
			child = (GDT_SERVER_CONNECTION_INFO *)gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
			gdt_make_connection_info_core(option,child,i);
		}
	}
	return option->connection_munit;
}

int32_t gdt_make_connection_info_core( GDT_SOCKET_OPTION *option, GDT_SERVER_CONNECTION_INFO* tinfo, int index )
{
	size_t recvbuffer_size = sizeof( char ) * ( option->recvbuffer_size );
	size_t msgbuffer_size = sizeof( char ) * ( option->msgbuffer_size );
	tinfo->id				= -1;
	tinfo->index			= index;
	tinfo->user_data		= NULL;
	tinfo->gdt_socket_option= option;
	if( -1 == ( tinfo->recvbuf_munit = gdt_create_munit( option->memory_pool, recvbuffer_size, MEMORY_TYPE_DEFAULT ) ) ){
		return GDT_SYSTEM_ERROR;
	}
	if( -1 == ( tinfo->recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT ) ) ){
		return GDT_SYSTEM_ERROR;
	}
	if( -1 == ( tinfo->recvmsg_munit = gdt_create_munit( option->memory_pool, msgbuffer_size, MEMORY_TYPE_DEFAULT ) ) ){
		return GDT_SYSTEM_ERROR;
	}
	gdt_init_socket_param( &tinfo->sockparam );
	return GDT_SYSTEM_OK;
}

/*
 * initialize GDT_SERVER_CONNECTION_INFO
 */
void gdt_initialize_connection_info( GDT_SOCKET_OPTION *option, struct GDT_SERVER_CONNECTION_INFO* tinfo )
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
	if( tinfo->user_data != NULL ){
		tinfo->user_data = NULL;
	}
}

ssize_t gdt_send_broadcast( GDT_SOCKET_OPTION *option, char *buf, size_t size, uint32_t payload_type )
{
	ssize_t ret = 0;
	GDT_SERVER_CONNECTION_INFO *tmptinfo;
	int i;
	if( option->connection_munit > 0 )
	{
		for( i = 0; i < option->maxconnection; i++ )
		{
			tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
			if( tmptinfo->sockparam.acc > 0 )
			{
				if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, buf, size, payload_type ) ) ){
					if( option->close_callback != NULL ){
						option->close_callback( tmptinfo );
					}
					gdt_free_sockparam( option, &tmptinfo->sockparam );
				}
			}
		}
	}
	return ret;
}

/*
 * send packets
 */
ssize_t gdt_send( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char *buf, size_t size, uint32_t payload_type )
{
	ssize_t len = 0;
	int error_code = GDT_SYSTEM_OK;
	if( psockparam->type == SOCK_TYPE_NORMAL_UDP )
	{
		struct sockaddr* addr = NULL;
		size_t fromlen = 0;
		if (option->socket_type == SOKET_TYPE_SERVER_UDP) {
			addr = (struct sockaddr *)&psockparam->from;
			fromlen = psockparam->fromlen;
		}
		else{
			if( ( option->inetflag & INET_FLAG_BIT_CONNECT_UDP ) == 0 ){
				struct sockaddr_storage saddr;
				socklen_t saddr_len = 0;
				if( GDT_SYSTEM_OK != gdt_get_sockaddr_info(option,&saddr,&saddr_len) )
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
				error_code = GDT_SYSTEM_ERROR;
			}
		}
		else{
			if( ( len = gdt_sendto_all( psockparam->acc, buf, size, 0, addr, fromlen ) ) == -1 )
			{
				perror( "gdt_sendto_all" );
				error_code = GDT_SYSTEM_ERROR;
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
				error_code = GDT_SYSTEM_ERROR;
			}
		}
		else{
			if( ( len = gdt_send_all( psockparam->acc, buf, size, 0 ) ) == -1 )
			{
				perror( "gdt_send_all" );
				error_code = GDT_SYSTEM_ERROR;
			}
		}
	}
	else{
		perror( "invalid send protocol" );
		len = -1;
		error_code = GDT_SYSTEM_ERROR;
	}
	if( error_code == GDT_SYSTEM_OK ){
		if( option->send_finish_callback != NULL ){
			option->send_finish_callback( (void*)psockparam );
		}
	}
//	else{
//		if( option->close_callback != NULL )
//		{
//			option->close_callback( NULL );
//		}
//		gdt_free_sockparam( option, psockparam );
//	}
	return len;
}

/*
 * send tcp packets all
 */
ssize_t gdt_send_all(GDT_SOCKET_ID soc, char *buf, size_t size, int flag )
{
	int32_t len, lest;
	char *ptr;
	do{
		for( ptr = buf, lest = size; lest > 0; ptr += len, lest -= len )
		{
			if( ( len = send( soc, ptr, lest, flag ) ) == -1 )
			{
				//  && errno != EWOULDBLOCK
				if( errno != EAGAIN  ){
					return (-1);
				}
				else{
#ifdef __WINDOWS__

#else
					usleep(1000);
#endif
				}
				len = 0;
			}
			else{
#ifdef __GDT_DEBUG__
				//printf( "send progress:%d/%zu\n", len, size );
				//if( lest-len > 0 ){
				//	usleep(1000);
				//	printf( "send progress:%d/%zu\n", len, size );
				//}
#endif
			}
		}
	}while( false );
	return (size);
}

/*
 * send udp packets all
 */
ssize_t gdt_sendto_all(GDT_SOCKET_ID soc, char *buf, size_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen )
{
	int32_t len, lest;
	char *ptr;
	do{
		for( ptr = buf, lest = size; lest > 0; ptr += len, lest -= len )
		{
			if( ( len = sendto( soc, ptr, lest, flag, pfrom, fromlen ) ) == -1 )
			//if( ( len = sendto( soc, ptr, lest, flag, NULL, 0 ) ) == -1 )
			{
				if( errno != EAGAIN )
				{
					return (-1);
				}
				len = 0;
			}
			else{
#ifdef __GDT_DEBUG__
				//printf( "send progress:%d/%zu\n", len, size );
#endif
			}
		}
	}while( false );
	return (size);
}

/*
 * ソケットオプションの設定
 */
void gdt_set_sock_option( GDT_SOCKET_OPTION *option )
{
	socklen_t len;
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_RCVBUF, &option->recvbuffer_size, sizeof( option->recvbuffer_size ) );
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_SNDBUF, &option->sendbuffer_size, sizeof( option->sendbuffer_size ) );
	len = sizeof( option->recvbuffer_size );
	if( getsockopt( option->sockid, SOL_SOCKET, SO_RCVBUF, &option->recvbuffer_size, &len ) == -1 ){
		perror("getsockopt");
	}
	len = sizeof( option->sendbuffer_size );
	if( getsockopt( option->sockid, SOL_SOCKET, SO_SNDBUF, &option->sendbuffer_size, &len ) == -1 ){
		perror("getsockopt");
	}
//#ifdef __GDT_DEBUG__
//	printf( "default:SO_RCVBUF=%zd\n", option->recvbuffer_size );
//	printf( "default:SO_SNDBUF=%zd\n", option->sendbuffer_size );
//#endif
}

int gdt_set_block(GDT_SOCKET_ID fd, int flag )
{
#ifdef __WINDOWS__
#else
	int flags;
#endif
	int error = 0;
	do{
#ifdef __WINDOWS__
		flag = !(flag);
		ioctlsocket(fd, FIONBIO, &flag);
#else
		if( ( flags = fcntl( fd, F_GETFL, 0 ) ) == -1 )
		{
			perror("fcntl");
			error = -1;
			break;
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
	}while( false );
	return error;
}

int gdt_get_sockaddr_info( GDT_SOCKET_OPTION *option, struct sockaddr_storage *saddr, socklen_t *addr_len )
{
	char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct addrinfo hints, *res0;
	int errcode;
//#ifdef __GDT_DEBUG__
//	printf( "gdt_get_sockaddr_info: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
//#endif
	do{
		(void) memset( &hints, 0, sizeof( hints ) );
		if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) == 0 ){
			hints.ai_family = AF_INET;
		}
		else{
			hints.ai_family = AF_INET6;
		}
		hints.ai_socktype = SOCK_STREAM;
		if( option->socket_type == SOKET_TYPE_CLIENT_TCP )
		{
			hints.ai_socktype = SOCK_STREAM;
		}
		else if( option->socket_type == SOKET_TYPE_CLIENT_UDP )
		{
			hints.ai_socktype = SOCK_DGRAM;
		}
		hints.ai_flags	= AI_PASSIVE;
		//hints.ai_flags = AI_NUMERICSERV;
		if( ( errcode = getaddrinfo( (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) , (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), &hints, &res0 ) ) != 0 ){
			printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
			return GDT_SYSTEM_ERROR;
		}
		if( ( errcode = getnameinfo( res0->ai_addr, res0->ai_addrlen, 
			nbuf, sizeof( nbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV ) ) != 0 )
		{
			printf( "getnameinfo():%s\n",gai_strerror( errcode ) );
			freeaddrinfo( res0 );
			return GDT_SYSTEM_ERROR;
		}
//#ifdef __GDT_DEBUG__
//		printf( "gdt_get_sockaddr_info:addr=%s\n", nbuf );
//		printf( "gdt_get_sockaddr_info:port=%s\n", sbuf );
//#endif
		memcpy(saddr,res0->ai_addr,res0->ai_addrlen);
		*addr_len = res0->ai_addrlen;
		freeaddrinfo( res0 );
	}while( false );
	return GDT_SYSTEM_OK;
}

/*
 * create server socket
 * @param option
 * @return GDT_SOCKET_ID( error : -1 )
 *
 * ※bindで 「bind: Address already in use」と出る場合は
 * ipv4射影アドレス機能( ipv6でipv4も使える )が有効になっているので以下の設定をする
 * 
 * echo "1" > /proc/sys/net/ipv6/bindv6only
 */
GDT_SOCKET_ID gdt_server_socket( GDT_SOCKET_OPTION *option, int is_ipv6 )
{
	GDT_SOCKET_ID sock = -1;
	char nbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	struct addrinfo hints, *res0;
	int opt , errcode;
	socklen_t opt_len;
	char* hostname = ( option->host_name_munit > 0 ) ? (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) : NULL;
	char* port = ( option->port_num_munit > 0 ) ? (char *)gdt_upointer( option->memory_pool,option->port_num_munit ) : NULL;
#ifdef __GDT_DEBUG__
//	printf( "server_socket: port=%s, host=%s\n", port, hostname  );
#endif
	do{
		(void) memset( &hints, 0, sizeof( hints ) );
		if( is_ipv6 == 1 ){
			hints.ai_family   = AF_INET6;
		}
		else{
			hints.ai_family   = AF_INET;
		}
		hints.ai_socktype = SOCK_STREAM;
		if( option->socket_type == SOKET_TYPE_SERVER_TCP )
		{
			hints.ai_socktype = SOCK_STREAM;
		}
		else if( option->socket_type == SOKET_TYPE_SERVER_UDP )
		{
			hints.ai_socktype = SOCK_DGRAM;	
		}
		hints.ai_flags	= AI_PASSIVE;
		if( ( errcode = getaddrinfo( hostname , port, &hints, &res0 ) ) != 0 ){
			printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
			break;
		}
		if( ( errcode = getnameinfo( res0->ai_addr, res0->ai_addrlen, 
			nbuf, sizeof( nbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV ) ) != 0 )
		{
			printf( "getnameinfo():%s\n",gai_strerror( errcode ) );
			freeaddrinfo( res0 );
			break;
		}
//#ifdef __GDT_DEBUG__
//		printf( "addr=%s\n", nbuf );
//		printf( "port=%s\n", sbuf );
//#endif
		if( ( sock = socket( res0->ai_family, res0->ai_socktype, res0-> ai_protocol ) ) == -1 )
		{
			freeaddrinfo( res0 );
			gdt_error("socket");
			break;
		}
		opt = 1;
		opt_len = sizeof( opt );
		if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len ) == -1 )
		{
			gdt_close_socket( &sock, "setsockopt" );
			freeaddrinfo( res0 );
			break;
		}
		if( bind( sock, res0->ai_addr, res0->ai_addrlen) == -1 )
		{
			gdt_close_socket( &sock, "bind" );
			freeaddrinfo( res0 );
			break;
		}
		if( option->socket_type == SOKET_TYPE_SERVER_TCP )
		{
			if( listen( sock, SOMAXCONN ) == -1 )
			{
				gdt_close_socket( &sock, "listen" );
				freeaddrinfo( res0 );
				break;
			}
		}
		freeaddrinfo( res0 );
	}while( false );
	return ( sock );
}

/*
 * create client socket
 * @param option
 * @return GDT_SOCKET_ID( error : -1 )
 */
GDT_SOCKET_ID gdt_client_socket( GDT_SOCKET_OPTION *option )
{
	GDT_SOCKET_ID sock = -1;
	int connectSuccess = false;
	char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct addrinfo hints, *res0, *local_info;
	struct timeval timeout;
	int errcode, width, val;
	socklen_t len;
	fd_set mask, write_mask, read_mask;
	do{
		char* hostname = (char *)gdt_upointer( option->memory_pool,option->host_name_munit );
		char* portnum = (char *)gdt_upointer( option->memory_pool,option->port_num_munit );
		(void) memset( &hints, 0, sizeof( hints ) );
		if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) == 0 ){
			hints.ai_family = AF_INET;
		}
		else{
			hints.ai_family = AF_INET6;
		}
		hints.ai_socktype = SOCK_STREAM;
		if( option->socket_type == SOKET_TYPE_CLIENT_TCP )
		{
			hints.ai_socktype = SOCK_STREAM;
		}
		else if( option->socket_type == SOKET_TYPE_CLIENT_UDP )
		{
			hints.ai_socktype = SOCK_DGRAM;
		}
		hints.ai_flags	= AI_PASSIVE;
		//hints.ai_flags = AI_NUMERICSERV;
		if( ( errcode = getaddrinfo( hostname , portnum, &hints, &res0 ) ) != 0 ){
			printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
			break;
		}
		if( ( errcode = getnameinfo( res0->ai_addr, res0->ai_addrlen, 
			nbuf, sizeof( nbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV ) ) != 0 )
		{
			printf( "getnameinfo():%s\n",gai_strerror( errcode ) );
			freeaddrinfo( res0 );
			break;
		}
//#ifdef __GDT_DEBUG__
//		printf( "addr=%s\n", nbuf );
//		printf( "port=%s\n", sbuf );
//#endif
		if( ( sock = socket( res0->ai_family, res0->ai_socktype, res0->ai_protocol ) ) == -1 )
		{
			freeaddrinfo( res0 );
			(void) gdt_error("socket");
			break;
		}
		if( option->socket_type == SOKET_TYPE_CLIENT_UDP )
		{
			if( ( option->inetflag & INET_FLAG_BIT_CONNECT_UDP ) == 0 ){
				if( ( errcode = getaddrinfo( NULL , "0", &hints, &local_info ) ) != 0 ){
					printf( "getaddrinfo():%s\n",gai_strerror( errcode ) );
					break;
				}
				if( bind( sock, (struct sockaddr *)local_info->ai_addr, sizeof(struct sockaddr)) == -1 )
				{
					freeaddrinfo( res0 );
					freeaddrinfo( local_info );
					(void) gdt_error("socket");
					break;
				}
				freeaddrinfo( local_info );
				freeaddrinfo( res0 );
				break;
			}
		}
		if( option->t_sec <= 0 && option->t_usec <= 0 )
		{
			if( connect( sock, res0->ai_addr, res0->ai_addrlen ) == -1 ){
				gdt_close_socket( &sock, "connect" );
				freeaddrinfo( res0 );
				break;
			}
			freeaddrinfo( res0 );
		}
		else{
			( void ) gdt_set_block( sock, 0 );
			if( connect( sock, res0->ai_addr, res0->ai_addrlen ) == -1 )
			{
#ifdef __WINDOWS__
				if( errno != 0 )
#else
				if( errno != EINPROGRESS )
#endif
				{
					gdt_close_socket( &sock, "select" );
					freeaddrinfo( res0 );
					break;
				}
			}
			else{
				( void ) gdt_set_block( sock, 1 );
				freeaddrinfo( res0 );
				break;
			}
			width = 0;
			FD_ZERO( &mask );
			FD_SET( sock, &mask );
			width = (int)(sock + 1);
			timeout.tv_sec = option->t_sec;
			timeout.tv_usec = option->t_usec;
			for(;;)
			{
				write_mask = mask;
				read_mask = mask;
				switch( select( width, &read_mask, &write_mask, NULL, &timeout ) )
				{
					case -1:
						if( errno != EINTR )
						{
							gdt_close_socket( &sock, "select" );
							freeaddrinfo( res0 );
						}
						break;
					case 0:
						gdt_close_socket( &sock, NULL );
						printf( "select:timeout\n" );
						freeaddrinfo( res0 );
						break;
					default:
						if( FD_ISSET( sock, &write_mask ) || FD_ISSET( sock, &read_mask ) )
						{
							len = sizeof( len );
							if( getsockopt( sock, SOL_SOCKET, SO_ERROR, &val, &len ) != -1 )
							{
								if( val == 0 )
								{
									(void) gdt_set_block( sock, 1 );
									freeaddrinfo( res0 );
									connectSuccess = true;
								}
								else{
#ifdef __WINDOWS__
									char errbuf[256];
									strerror_s(errbuf,255,val);
#else
									(void)fprintf(stderr, "getsockopt:%d:%s\n", val, strerror(val));
#endif
									gdt_close_socket( &sock, NULL );
									freeaddrinfo( res0 );
								}
							}
							else{
								gdt_close_socket( &sock, "select" );
								freeaddrinfo( res0 );
							}
						}
						break;
				}
				if( sock < 0 || connectSuccess == true ){
					break;
				}
			}
		}
	}while( false );
	return sock;
}

void gdt_disconnect( GDT_SOCKPARAM *psockparam )
{
	psockparam->c_status = PROTOCOL_STATUS_DEFAULT;
}

void* gdt_socket( GDT_SOCKET_OPTION *option )
{
	do{
		if( option->memory_pool == NULL ){
			printf("option->memory_pool == NULL\n");
			break;
		}
		switch( option->socket_type )
		{
			case SOKET_TYPE_SERVER_TCP:
			case SOKET_TYPE_SERVER_UDP:
				if( ( option->sockid = gdt_server_socket( option, 0 ) ) <= 0 )
				{
					printf( "gdt_server_socket error: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
					break;
				}
				if( ( option->inetflag & INET_FLAG_BIT_IPV6 ) != 0 )
				{
					if( ( option->sockid6 = gdt_server_socket( option, 1 ) ) <= 0 )
					{
						printf( "gdt_server_socket ipv6 error: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
						option->sockid6 = -1;
					}
				}
				gdt_set_sock_option( option );
#ifdef __WINDOWS__
				switch( option->mode )
				{
					case SOCKET_MODE_SINGLE:	// single connection server mode
						//gdt_single_task_server( option );
						//break;
					case SOCKET_MODE_SELECT:	// select server mode
						//gdt_select_server( option );
						//break;
						case SOCKET_MODE_THREAD:	// thread server mode
						//gdt_thread_server( option );
						//break;
					case SOCKET_MODE_POOL:		// pool server mode
					case SOCKET_MODE_EPOOL:		// epool server mode
					case SOCKET_MODE_KQUEUE:	// kqueue server mode
					case SOCKET_MODE_PREFORK:	// prefork serve mode
						printf("not suppoerted windows server mode\n");
						break;
					case SOCKET_MODE_NONBLOCKING:
						gdt_nonblocking_server(option);
						break;
					default:
						break;
				}
#else
				switch( option->mode )
				{
					case SOCKET_MODE_NONBLOCKING:
						gdt_nonblocking_server(option);
						break;
					case SOCKET_MODE_SINGLE:	// single connection server mode
						//gdt_single_task_server( option );
						//break;
					case SOCKET_MODE_SELECT:	// select server mode
						//gdt_select_server( option );
						//break;
					case SOCKET_MODE_POOL:		// pool server mode
						//gdt_pool_server( option );
						//break;
					case SOCKET_MODE_EPOOL:		// epool server mode
						//gdt_epool_server( option );
						//break;
					case SOCKET_MODE_PREFORK:	// prefork server mode
						//gdt_prefork_server( option );
						//break;
					case SOCKET_MODE_THREAD:	// thread server mode
						//gdt_thread_server( option );
						//break;
					case SOCKET_MODE_KQUEUE:	// kqueue server mode
						//gdt_kqueue_server( option );
						//break;
					default:
						printf("not suppoerted server mode\n");
						break;
				}
#endif
				break;
			case SOKET_TYPE_CLIENT_TCP:
			case SOKET_TYPE_CLIENT_UDP:
				if( ( option->sockid = gdt_client_socket( option ) ) <= 0 )
				{
					printf( "gdt_client_socket error: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
					break;
				}
				gdt_set_sock_option( option );
				switch( option->mode )
				{
					case SOCKET_MODE_SIMPLE_TERM:
						//gdt_send_recv_client_term( option );
						break;
					case SOCKET_MODE_CLIENT_THREAD:
						//gdt_thread_client( option );
						break;
					case SOCKET_MODE_CLIENT_NONBLOCKING:
						gdt_nonblocking_client(option);
						break;
					default:
						break;
				}
				break;
			default:
				printf( "socket_type error\n" );
				break;
		}
	}while( false );
	return ( (void *) NULL );
}

/*
 * close socket handle
 */
void gdt_free_socket( GDT_SOCKET_OPTION *option )
{
#ifdef __WINDOWS__
	if( option->sockid > -1 ){
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
}

void gdt_nonblocking_server(GDT_SOCKET_OPTION *option)
{
#ifdef __GDT_DEBUG__
	printf("gdt_nonblocking_server: soc4=%d, soc6=%d\n", (int)option->sockid, (int)option->sockid6);
#endif
	if (option->socket_type == SOKET_TYPE_SERVER_UDP)
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

void gdt_server_update(GDT_SOCKET_OPTION *option)
{
	if (option->mode != SOCKET_MODE_NONBLOCKING) {
		return;
	}
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	socklen_t srlen;
	struct GDT_RECV_INFO *rinfo;
	GDT_SERVER_CONNECTION_INFO *child;
	socklen_t len;
	if (option->socket_type == SOKET_TYPE_SERVER_TCP)
	{
		GDT_SOCKET_ID acc;
		int i;
		struct sockaddr_storage from;
		len = (socklen_t) sizeof(from);
		if (-1 == (acc = accept(option->sockid, (struct sockaddr *) &from, &len))) {
			//if (errno != 0 && errno != EINTR && errno != EAGAIN) {
			if (errno != 0 && errno != EAGAIN) {
				perror("accept");
			}
		}
		if (acc == -1 && option->sockid6 != -1) {
			if (-1 == (acc = accept(option->sockid6, (struct sockaddr *) &from, &len))) {
				//if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				if (errno != 0 && errno != EAGAIN) {
					perror("accept");
				}
			}
		}
		if (acc != -1)
		{
			for (i = 0; i < option->maxconnection; i++)
			{
				child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(GDT_SERVER_CONNECTION_INFO), i);
				if (child->id == -1)
				{
					(void)getnameinfo((struct sockaddr *) &from, len,
						child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
					child->id = acc;
					child->sockparam.acc = child->id;
					if (option->socket_type == SOKET_TYPE_SERVER_UDP){
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
					}
				}
			}
		}
		for (i = 0; i < option->maxconnection; i++)
		{
			child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(GDT_SERVER_CONNECTION_INFO), i);
			if (child->id != -1)
			{
				if (option->user_recv_function != NULL) {
					if (-1 == (srlen = option->user_recv_function(child, child->id, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0))) {
#ifdef __WINDOWS__
						if (WSAGetLastError() != WSAEWOULDBLOCK) {
							perror("recv");
							gdt_close_socket(&child->id, NULL);
						}
#else
						if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
						{
							perror("recv");
							gdt_close_socket(&child->id, NULL);
						}
#endif
					}
				}
				else {
					if ((srlen = recv(child->id, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0)) == -1) {
#ifdef __WINDOWS__
						if (WSAGetLastError() != WSAEWOULDBLOCK) {
							perror("recv");
							gdt_close_socket(&child->id, NULL);
						}
#else
						if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
						{
							perror("recv");
							printf("errno : %d\n",errno);
							gdt_close_socket(&child->id, NULL);
						}
						//if( errno == EAGAIN ){
						//	printf( ".\n" );
						//	usleep( 1000 );
						//}
#endif
					}
				}
				if (srlen == -1) {
				}
				else if (srlen == 0) {
#ifdef __WINDOWS__
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						perror("recv 0byte");
						gdt_close_socket(&child->id, NULL);
					}
#else
					if (errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK){
						perror("recv 0byte");
					}
					gdt_close_socket(&child->id, NULL);
#endif

				}
				else if (option->plane_recv_callback != NULL)
				{
					rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
					child->sockparam.acc = child->id;
					rinfo->tinfo = child;
					rinfo->recvlen = srlen;
					rinfo->recvbuf_munit = child->recvbuf_munit;
					rinfo->recvfrom = child->id;
					switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
					{
					case -1:
						gdt_close_socket(&child->id, NULL);
						break;
					case 1:
						option->plane_recv_callback((void*)rinfo);
						if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
						{
							gdt_close_socket(&child->id, NULL);
						}
						break;
					}
					*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
					//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
				}
				else if (option->payload_recv_callback != NULL)
				{
					rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
					child->sockparam.acc = child->id;
					rinfo->tinfo = child;
					size_t old_pos = 0;
					do{
						rinfo->recvlen = srlen;
						rinfo->recvbuf_munit = child->recvbuf_munit;
						rinfo->recvfrom = child->id;
						switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
						{
						case -1:
							gdt_close_socket(&child->id, NULL);
							break;
						case 1:
							option->payload_recv_callback(child->sockparam.payload_type, (uint8_t*)GDT_POINTER(option->memory_pool, rinfo->recvbuf_munit), rinfo->recvlen, rinfo);
							if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT){
								gdt_close_socket(&child->id, NULL);
							}
							break;
						}
						*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
						//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
						if( child->sockparam.continue_pos > 0 ){
#ifdef __WINDOWS__

#else
							usleep(1000);
#endif
							if( old_pos >= child->sockparam.continue_pos ){
								printf("invalid packet\n");
								break;
							}
							// printf("continue_pos : %d, %d, %d\n",(int)child->sockparam.continue_pos,(int)child->sockparam.tmpmsglen,child->sockparam.fin);
							old_pos = child->sockparam.continue_pos;
						}
					}while( child->sockparam.continue_pos > 0 && child->id > 0 );
				}
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
	else if (option->socket_type == SOKET_TYPE_SERVER_UDP)
	{
		child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(GDT_SERVER_CONNECTION_INFO), 0);
		child->sockparam.fromlen = sizeof(child->sockparam.from);
		if ((len = recvfrom(option->sockid, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0, (struct sockaddr *)&child->sockparam.from, &child->sockparam.fromlen)) == -1)
		{
			if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				perror("recvfrom");
			}
			return;
		}
		if (len == -1) {
		}
		else if (len == 0) {
			perror("recv");
			gdt_close_socket(&child->id, NULL);
		}
		else if (option->plane_recv_callback != NULL)
		{
			(void)getnameinfo((struct sockaddr *) &child->sockparam.from, child->sockparam.fromlen,
				child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
			rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
			rinfo->tinfo = child;
			rinfo->recvbuf_munit = child->recvbuf_munit;
			rinfo->recvlen = len;
			rinfo->recvfrom = child->sockparam.acc;
			switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
			{
			case -1:
				gdt_close_socket(&child->id, NULL);
				break;
			case 1:
				option->plane_recv_callback((void*)rinfo);
				if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
				{
					gdt_close_socket(&child->id, NULL);
				}
				break;
			}
			*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
			//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
		}
		else if (option->payload_recv_callback != NULL)
		{
//#ifdef __WINDOWS__
//#else
//			struct sockaddr_in* pfrom = (struct sockaddr_in*)&child->sockparam.from;
//			printf("(%d)%s:%d\n",(int)len,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port));
//#endif
			(void)getnameinfo((struct sockaddr *) &child->sockparam.from, child->sockparam.fromlen,
				child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
			rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
			rinfo->tinfo = child;
			rinfo->recvbuf_munit = child->recvbuf_munit;
			rinfo->recvlen = len;
			rinfo->recvfrom = child->sockparam.acc;
			switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
			{
			case -1:
				gdt_close_socket(&child->id, NULL);
				break;
			case 1:
				option->payload_recv_callback(child->sockparam.payload_type, (uint8_t*)GDT_POINTER(option->memory_pool, rinfo->recvbuf_munit), rinfo->recvlen, rinfo);
				if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
				{
					gdt_close_socket(&child->id, NULL);
				}
				break;
			}
			*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
			//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
		}
	}
}

void gdt_nonblocking_client(GDT_SOCKET_OPTION *option)
{
//#ifdef __GDT_DEBUG__
//	printf("gdt_nonblocking_client: soc4=%d, soc6=%d\n", (int)option->sockid, (int)option->sockid6);
//#endif
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	gdt_set_block(option->sockid, 0);
#ifdef __WINDOWS__
	struct linger l;
	l.l_onoff=0;
	l.l_linger=0;
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof( l ) );
#endif
	option->connection_munit = gdt_create_munit( option->memory_pool, sizeof( GDT_SERVER_CONNECTION_INFO ), MEMORY_TYPE_DEFAULT );
	if( option->connection_munit == -1 ){
		return;
	}
	GDT_SERVER_CONNECTION_INFO * child = (GDT_SERVER_CONNECTION_INFO*)GDT_POINTER(option->memory_pool,option->connection_munit);
	child->id = option->sockid;
	child->recvbuf_munit = gdt_create_munit( option->memory_pool, buffer_size, MEMORY_TYPE_DEFAULT );
	child->recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
	child->recvmsg_munit = gdt_create_munit( option->memory_pool, sizeof( char) * ( option->msgbuffer_size ), MEMORY_TYPE_DEFAULT );
	child->gdt_socket_option = option;
	child->index = 0;
	gdt_init_socket_param( &child->sockparam );
	child->sockparam.acc = option->sockid;
	if (option->socket_type == SOKET_TYPE_CLIENT_TCP){
		child->sockparam.type = SOCK_TYPE_NORMAL_TCP;
	}
	else {
		child->sockparam.type = SOCK_TYPE_NORMAL_UDP;
	}
	if( option->connection_start_callback != NULL ){
		option->connection_start_callback( (void*)child );
	}
}

void gdt_client_update(GDT_SOCKET_OPTION *option)
{
	if (option->mode != SOCKET_MODE_CLIENT_NONBLOCKING) {
		return;
	}
	if( option->connection_munit == -1 )
	{
		return;
	}
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	socklen_t srlen;
	struct GDT_RECV_INFO *rinfo;
	GDT_SERVER_CONNECTION_INFO* child = (GDT_SERVER_CONNECTION_INFO*)GDT_POINTER(option->memory_pool,option->connection_munit);
	if (option->socket_type == SOKET_TYPE_CLIENT_TCP)
	{
		if (child->id != -1)
		{
			if (option->user_recv_function != NULL) {
				if (-1 == (srlen = option->user_recv_function(child, child->id, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0))) {
					if (errno != 0 && errno != EAGAIN)
					{
						perror("recv");
						gdt_close_socket(&child->id, NULL);
					}
				}
			}
			else {
				if ((srlen = recv(child->id, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0)) == -1) {
#ifdef __WINDOWS__
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
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
			}
			if (srlen == -1) {
				//printf("srlen == -1\n");
			}
			else if (srlen == 0) {
				perror("recv");
				gdt_close_socket(&child->id, NULL);
			}
			else if (option->plane_recv_callback != NULL)
			{
				if( *((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) == '\0' ){
					printf("gdt_recv invalid packet\n");
					return;
				}
				rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
				child->sockparam.acc = child->id;
				rinfo->tinfo = child;
				size_t old_pos = 0;
				do{
					rinfo->recvlen = srlen;
					rinfo->recvbuf_munit = child->recvbuf_munit;
					rinfo->recvfrom = child->id;
					switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
					{
					case -1:
							gdt_close_socket(&child->id, NULL);
							break;
					case 1:
							option->plane_recv_callback((void*)rinfo);
							if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
							{
									gdt_close_socket(&child->id, NULL);
							}
							break;
					}
					*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
					//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
					if( child->sockparam.continue_pos > 0 ){
						//usleep(10000);
						if( old_pos >= child->sockparam.continue_pos ){
							printf("invalid packet\n");
							break;
						}
						// printf("continue_pos : %d, %d, %d\n",(int)child->sockparam.continue_pos,(int)child->sockparam.tmpmsglen,child->sockparam.fin);
						old_pos = child->sockparam.continue_pos;
					}
				}while( child->sockparam.continue_pos > 0 && child->id > 0 );
			}
			else if (option->payload_recv_callback != NULL)
			{
				rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
				child->sockparam.acc = child->id;
				rinfo->tinfo = child;
				size_t old_pos = 0;
				do{
					rinfo->recvlen = srlen;
					rinfo->recvbuf_munit = child->recvbuf_munit;
					rinfo->recvfrom = child->id;
					switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
					{
					case -1:
						gdt_close_socket(&child->id, NULL);
						break;
					case 1:
						option->payload_recv_callback(child->sockparam.payload_type, (uint8_t*)GDT_POINTER(option->memory_pool, rinfo->recvbuf_munit), rinfo->recvlen, rinfo);
						if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
						{
							gdt_close_socket(&child->id, NULL);
						}
						break;
					}
					
					*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
					// memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
					if( child->sockparam.continue_pos > 0 ){
						//usleep(10000);
						if( old_pos >= child->sockparam.continue_pos ){
							printf("invalid packet\n");
							break;
						}
						// printf("continue_pos : %d, %d, %d\n",(int)child->sockparam.continue_pos,(int)child->sockparam.tmpmsglen,child->sockparam.fin);
						old_pos = child->sockparam.continue_pos;
					}
				}while( child->sockparam.continue_pos > 0 && child->id > 0 );
			}
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
	else if (option->socket_type == SOKET_TYPE_CLIENT_UDP)
	{
		if (child->id == -1)
		{
			return;
		}
		//memset(&child->sockparam.from,0,sizeof(child->sockparam.from));
		child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(GDT_SERVER_CONNECTION_INFO), 0);
		child->sockparam.fromlen = sizeof(child->sockparam.from);
		if ((srlen = recvfrom(option->sockid, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0, (struct sockaddr *)&child->sockparam.from, &child->sockparam.fromlen)) == -1)
		//if ((srlen = recvfrom(option->sockid, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0, NULL, NULL)) == -1)
		{
			if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				perror("recvfrom");
			}
			return;
		}
		if (srlen == -1) {
		}
		else if (srlen == 0) {
			perror("recvfrom");
			gdt_close_socket(&child->id, NULL);
		}
		else if (option->plane_recv_callback != NULL)
		{
//#ifdef __WINDOWS__
//#else
//			struct sockaddr_in* pfrom = (struct sockaddr_in*)&child->sockparam.from;
//			printf("(%d)%s:%d\n", (int)srlen, inet_ntoa(pfrom->sin_addr), ntohs(pfrom->sin_port));
//#endif
			(void)getnameinfo((struct sockaddr *) &child->sockparam.from, child->sockparam.fromlen,
				child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
			rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
			rinfo->tinfo = child;
			rinfo->recvbuf_munit = child->recvbuf_munit;
			rinfo->recvlen = srlen;
			rinfo->recvfrom = child->sockparam.acc;
			switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
			{
			case -1:
				gdt_close_socket(&child->id, NULL);
				break;
			case 1:
				option->plane_recv_callback((void*)rinfo);
				if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
				{
					gdt_close_socket(&child->id, NULL);
				}
				break;
			}
			*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
			//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
		}
		else if (option->payload_recv_callback != NULL)
		{
//#ifdef __WINDOWS__
//#else
//			struct sockaddr_in* pfrom = (struct sockaddr_in*)&child->sockparam.from;
//			printf("(%d)%s:%d\n", (int)srlen, inet_ntoa(pfrom->sin_addr), ntohs(pfrom->sin_port));
//#endif
			(void)getnameinfo((struct sockaddr *) &child->sockparam.from, child->sockparam.fromlen,
				child->hbuf, sizeof(child->hbuf), child->sbuf, sizeof(child->sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
			rinfo = (struct GDT_RECV_INFO *)gdt_upointer(option->memory_pool, child->recvinfo_munit);
			rinfo->tinfo = child;
			rinfo->recvbuf_munit = child->recvbuf_munit;
			rinfo->recvlen = srlen;
			rinfo->recvfrom = child->sockparam.acc;
			switch (gdt_pre_packetfilter(option, rinfo, &child->sockparam, child->recvmsg_munit))
			{
			case -1:
				gdt_close_socket(&child->id, NULL);
				break;
			case 1:
				option->payload_recv_callback(child->sockparam.payload_type, (uint8_t*)GDT_POINTER(option->memory_pool, rinfo->recvbuf_munit), rinfo->recvlen, rinfo);
				if (child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT)
				{
					gdt_close_socket(&child->id, NULL);
				}
				break;
			}
			*((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) = '\0';
			//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
		}
	}
}

void gdt_print_payload(uint32_t payload_type, uint8_t* payload, size_t payload_len,size_t view_max)
{
	printf("(payload_type:%d,payload_len:%d)", (int)payload_type, (int)payload_len);
	gdt_print_hex( payload, payload_len, view_max );
}

int gdt_pre_packetfilter( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam, int32_t recvmsg_munit )
{
	int ret = 0;
	char *pbuf;
	char *msgpbuf;
	ssize_t _tmpmsglen;
	pbuf = (char*)gdt_upointer( option->memory_pool, rinfo->recvbuf_munit );
	if( rinfo->recvlen > 0 )
	{
		if( option->socket_type == SOKET_TYPE_SERVER_UDP || option->socket_type == SOKET_TYPE_CLIENT_UDP )
		{
			psockparam->type		= SOCK_TYPE_NORMAL_UDP;
			psockparam->c_status	= PROTOCOL_STATUS_OTHER;
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
				if( psockparam->tmpmsglen == 0 )
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

ssize_t gdt_parse_socket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit )
{
	int i, startpos;
	int64_t cnt = 0;
	uint64_t tmppayloadlen = 0;
	char* msg = (char*)gdt_upointer( option->memory_pool,basebuf_munit );
	ssize_t retsize = -1;
	size_t tmp_continue_pos = psockparam->continue_pos;
	do{
		if( ( psockparam->fin == 1 && psockparam->tmpmsglen == 0 ) ){
			if( psockparam->continue_pos > 0 ){
				u8buf+=psockparam->continue_pos;
				size-=psockparam->continue_pos;
				psockparam->continue_pos = 0;
			}
			memset( msg, 0, gdt_usize( option->memory_pool, basebuf_munit ) );
			psockparam->payloadlen = 0;
			if(size<2){
				psockparam->payloadlen = -1;
				break;
			}
			psockparam->fin				= u8buf[0] >> 7;
			psockparam->rsv				= ( u8buf[0] & 0x70 ) >> 4;
			psockparam->opcode			= ( u8buf[0] & 0x0f );
			psockparam->mask			= u8buf[1] >> 7;
			psockparam->ckpayloadlen	= ( u8buf[1] & 0x7f );
			if( psockparam->opcode == 8 ){
				//printf("opecode : 8(close)\n");
				break;
			}
			if(psockparam->ckpayloadlen < 126 ){
				psockparam->maskindex = 6;
				tmppayloadlen = psockparam->ckpayloadlen;
			}
			else if(psockparam->ckpayloadlen < 127 ){
				psockparam->maskindex = 8;
				if(size<4){
					psockparam->payloadlen = -1;
					break;
				}
				tmppayloadlen |= u8buf[2] << 8;
				tmppayloadlen |= u8buf[3] << 0;
			}
			else{
				psockparam->maskindex = 14;
				if(size<10){
					psockparam->payloadlen = -1;
					break;
				}
				tmppayloadlen |= (uint64_t)u8buf[2] << 56;
				tmppayloadlen |= (uint64_t)u8buf[3] << 48;
				tmppayloadlen |= (uint64_t)u8buf[4] << 40;
				tmppayloadlen |= (uint64_t)u8buf[5] << 32;
				tmppayloadlen |= (uint64_t)u8buf[6] << 24;
				tmppayloadlen |= (uint64_t)u8buf[7] << 16;
				tmppayloadlen |= (uint64_t)u8buf[8] << 8;
				tmppayloadlen |= (uint64_t)u8buf[9] << 0;
			}
			if( psockparam->fin == 0 ){
				//printf("invalid packet?\n");
				//exit(0);
			}
			//psockparam->payloadlen += tmppayloadlen;
			psockparam->payloadlen = tmppayloadlen;
			if( psockparam->payloadlen >= gdt_usize( option->memory_pool, basebuf_munit ) ){
				printf( "payloadlen buffersize over[%"PRIu64"][%zd]\n", psockparam->payloadlen, gdt_usize( option->memory_pool, basebuf_munit ) );
				psockparam->payloadlen = -1;
				break;
			}
			psockparam->payloadmask = 0x00000000;
			startpos = psockparam->maskindex;
			if( option->memory_pool->endian == GDT_LITTLE_ENDIAN ){
				psockparam->payload_type = BYTE_SWAP_BIT32( *( (uint32_t*)(u8buf+psockparam->maskindex-4) ) );
			}
			else{
				psockparam->payload_type = *( (uint32_t*)(u8buf+psockparam->maskindex-4) );
			}
		}
		else{
			if( psockparam->fin == 0 ){
				psockparam->fin = 1;
			}
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
		if( psockparam->fin && psockparam->tmpmsglen >= psockparam->payloadlen )
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
			//psockparam->continue_pos		= 0;
			msg[psockparam->payloadlen] = '\0';
			retsize = psockparam->payloadlen;
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

ssize_t gdt_send_msg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type )
{
	void *sendbin;
	uint8_t head1;
	uint8_t head2;
	uint8_t *ptr;
	char* cptr;
	uint32_t headersize = 0;
	ssize_t len = 0;
	do{
		head1 = 0x80 | psockparam->ws_msg_mode; // 0x81:text mode, 0x82:binary mode
		headersize = gdt_make_size_header(&head2,size);
		size_t binsize = (size_t) ( ( sizeof( uint8_t ) * size ) + headersize );
		if( psockparam->buf_munit < 0 || gdt_usize( option->memory_pool, psockparam->buf_munit ) <= size + headersize )
		{
			if( psockparam->buf_munit >= 0 ){
				gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
			}
			if( ( psockparam->buf_munit = gdt_create_munit( option->memory_pool, binsize+1, MEMORY_TYPE_DEFAULT ) ) == -1 )
			{
				printf( "[gdt_send_msg]size over: %zd byte\n", size );
				break;
			}
		}
		sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
		//memset( sendbin, 0, gdt_usize( option->memory_pool, psockparam->buf_munit ) );
		//memset( sendbin, 0, headersize );
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
		len = gdt_send_all( psockparam->acc, (char*)sendbin, binsize, 0 );
	}while( false );
	return len;
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

size_t gdt_make_udpmsg( void* sendbin, const char* msg, ssize_t size, uint32_t payload_type )
{
	uint8_t head1;
	uint8_t head2;
	uint8_t *ptr;
	char* cptr;
	uint32_t headersize = 0;
	do{
		head1 = 0x80; // mode ?
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
	}while( false );
	return (size_t) ( ( sizeof( char ) * size ) + headersize );
}

ssize_t gdt_send_udpmsg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type, struct sockaddr *pfrom, socklen_t fromlen  )
{
	void *sendbin;
	ssize_t len = 0;
	do{
		size_t tmp_size = size + gdt_make_size_header(NULL,size);
		if( psockparam->buf_munit < 0 || gdt_usize( option->memory_pool, psockparam->buf_munit ) <= tmp_size )
		{
			if( psockparam->buf_munit >= 0 ){
				gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
			}
			if( ( psockparam->buf_munit = gdt_create_munit( option->memory_pool, sizeof( uint8_t ) * GDT_ALIGNUP( tmp_size, option->msgbuffer_size ), MEMORY_TYPE_DEFAULT ) ) == -1 )
			{
				printf( "[gdt_send_udpmsg]size over: %zd byte\n", size );
				break;
			}
		}
		sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
		memset( sendbin, 0, gdt_usize( option->memory_pool, psockparam->buf_munit ) );
		size_t send_size = gdt_make_udpmsg( sendbin, msg, size, payload_type );
		len = gdt_sendto_all( psockparam->acc, (char*)sendbin, send_size, 0, pfrom, fromlen );
	}while( false );
	return len;
}
