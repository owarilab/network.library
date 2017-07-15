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
	if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
		printf("gdt_initialize_memory error\n");
		return NULL;
	}
	int32_t option_munit = gdt_create_munit( memory_pool, sizeof( GDT_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
	if( option_munit == -1 ){
		return NULL;
	}
	option = (GDT_SOCKET_OPTION*)GDT_POINTER(memory_pool,option_munit);
	gdt_initialize_socket_option( option, hostname, portnum, SOKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL );
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
	gdt_initialize_socket_option( option, hostname, portnum, SOKET_TYPE_SERVER_UDP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL );
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
	gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_CLIENT_TCP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL);
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
	gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_CLIENT_UDP, SOCKET_MODE_CLIENT_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL);
	return option;
}

ssize_t gdt_send_message(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option != NULL ){
		if( gdt_recv_info->tinfo->sockparam.acc != -1 ){
			return gdt_send(option, &gdt_recv_info->tinfo->sockparam, payload, payload_len, payload_type);
		}
	}
	return 0;
}

ssize_t gdt_send_message_broadcast(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option;
	if( option != NULL ){
		return gdt_send_broadcast( option, payload, payload_len, payload_type );
	}
	return 0;
}

ssize_t gdt_client_send_message(uint32_t payload_type, char* payload, size_t payload_len, GDT_SOCKET_OPTION *option)
{
    if( option == NULL ){
        return 0;
    }
	if( option->connection_munit != -1 )
	{
		GDT_SERVER_CONNECTION_INFO* gdt_connection_info = (GDT_SERVER_CONNECTION_INFO*)GDT_POINTER(option->memory_pool, option->connection_munit);
		if( gdt_connection_info != NULL && gdt_connection_info->sockparam.acc != -1 ){
			return gdt_send(option, &gdt_connection_info->sockparam, payload, payload_len, payload_type);
		}
	}
	return 0;
}

/*
 * initialize GDT_SOCKET_OPTION
 * @param *option GDT_SOCKET_OPTION pointer
 * @return int( 0:success, other:failed )
 */
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
	option->recvtimeoutmode		= TIMEOUT_MODE_NONE;
	option->maxconnection		= maxconnection;
	option->socket_type			= socket_type;
	option->mode				= mode;
	option->protocol			= protocol;
	option->t_sec				= 0;
	option->t_usec				= 0;
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
	option->recvbuffer_size				= 2048;
	option->sendbuffer_size				= 2048;
	option->msgbuffer_size				= 4096;
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
	else{
		// TODO : support
		//gethostname(option->hbuf, (int)sizeof(option->hbuf));
		//option->phostent = gethostbyname( option->hbuf );
		//memcpy(&option->inaddr, option->phostent->h_addr_list[0], 4);
		//gdt_strcopy(option->ipbuf, inet_ntoa(option->inaddr), sizeof(option->ipbuf));
		//gdt_strcopy(option->ipbuf, "dummy ip", sizeof(option->ipbuf));
		//gdt_strcopy(option->hbuf, "dummy host", sizeof(option->ipbuf));
		//printf("wVersion = %u\n", option->wsdata.wVersion);
		//printf("wHighVersion = %u\n", option->wsdata.wHighVersion);
		//printf("szDescription = %s\n", option->wsdata.szDescription);
		//printf("szSystemStatus = %s\n", option->wsdata.szSystemStatus);
		//printf("iMaxSockets = %d\n", option->wsdata.iMaxSockets);
		//printf("iMaxUdpDg = %d\n", option->wsdata.iMaxUdpDg);
		//printf("host name=%s\n", option->hbuf);
		//printf("ip addr=%s\n", option->ipbuf);
	}
#endif
	return result;
}

void set_on_connect_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func )
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
void set_on_close_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func )
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
void gdt_set_select_timeout( GDT_SOCKET_OPTION *option, int32_t sec, int32_t usec )
{
	option->s_sec = sec;
	option->s_usec = usec;
}
void set_message_buffer( GDT_SOCKET_OPTION *option, size_t buffer_size)
{
	option->msgbuffer_size = buffer_size;
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
	tinfo->user_data_munit	= -1;
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
	if( tinfo->user_data_munit > 0 ){
		gdt_free_memory_unit( option->memory_pool, &tinfo->user_data_munit );
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
				ret = gdt_send( option, &tmptinfo->sockparam, buf, size, payload_type );
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
	// UDPパケットの送信
	if( psockparam->type == SOCK_TYPE_NORMAL_UDP )
	{
		struct sockaddr* addr = NULL;
		size_t fromlen = 0;
		if (option->socket_type == SOKET_TYPE_SERVER_UDP) {
			addr = (struct sockaddr *)&psockparam->from;
			fromlen = psockparam->fromlen;
		}
		if( option->protocol == PROTOCOL_SIMPLE )
		{
			if( ( len = gdt_send_udpmsg( option, psockparam, buf, size, payload_type, addr, fromlen ) ) == -1 )
			{
				perror( "send" );
				if( option->close_callback != NULL )
				{
					option->close_callback( NULL );
				}
				gdt_free_sockparam( option, psockparam );
			}
		}
		else{
			if( ( len = gdt_sendto_all( psockparam->acc, buf, size, 0, addr, fromlen ) ) == -1 )
			{
				perror( "send" );
				if( option->close_callback != NULL )
				{
					option->close_callback( NULL );
				}
				gdt_free_sockparam( option, psockparam );
			}
		}
	}
	// TCPパケットの送信
	else if( psockparam->type == SOCK_TYPE_NORMAL )
	{
		if( option->protocol == PROTOCOL_SIMPLE )
		{
			if( ( len = gdt_send_msg( option, psockparam, buf, size, payload_type ) ) == -1 )
			{
				perror( "send" );
				if( option->close_callback != NULL )
				{
					option->close_callback( NULL );
				}
				gdt_free_sockparam( option, psockparam );
			}
		}
		else{
			if( ( len = gdt_send_all( psockparam->acc, buf, size, 0 ) ) == -1 )
			{
				perror( "send" );
				if( option->close_callback != NULL )
				{
					option->close_callback( NULL );
				}
				gdt_free_sockparam( option, psockparam );
			}
		}
	}
	// http用のTCPパケットの送信
	else if( psockparam->type == SOCK_TYPE_HTTP )
	{
		if( ( len = gdt_send_all( psockparam->acc, buf, size, 0 ) ) == -1 )
		{
			perror( "send" );
			if( option->close_callback != NULL )
			{
				option->close_callback( NULL );
			}
			gdt_free_sockparam( option, psockparam );
		}
	}
	// websocket用のTCPパケットの送信
	else if( psockparam->type == SOCK_TYPE_WEBSOCKET )
	{
		if( ( len = gdt_send_websocket_msg( option, psockparam, buf, size ) ) == -1 )
		{
			perror( "send" );
			if( option->close_callback != NULL )
			{
				option->close_callback( NULL );
			}
			gdt_free_sockparam( option, psockparam );
		}
	}
	else{
		perror( "invalid send protocol" );
	}
	// 送信完了完了コールバック関数の呼び出し
	if( option->send_finish_callback != NULL )
	{
		option->send_finish_callback( (void*)psockparam );
	}
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
				if( errno != EAGAIN )
				{
					return (-1);
				}
				len = 0;
			}
//#ifdef __GDT_DEBUG__
//			else{
//				printf( "send:%zu\n", len );
//			}
//#endif
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
//#ifdef __GDT_DEBUG__
//			else{
//				printf( "sendto:%zu\n", len );
//			}
//#endif
		}
	}while( false );
	return (size);
}

/*
 * ソケットオプションの設定
 */
void gdt_set_sock_option( GDT_SOCKET_OPTION *option )
{
	int n;
	socklen_t len;
	len = sizeof( n );
	if( getsockopt( option->sockid, SOL_SOCKET, SO_RCVBUF, &n, &len ) == 0 )
	{
		//printf( "default:SO_RCVBUF=%d\n", n );
	}
	if( getsockopt( option->sockid, SOL_SOCKET, SO_SNDBUF, &n, &len ) == 0 )
	{
		//printf( "default:SO_SNDBUF=%d\n", n );
	}
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_RCVBUF, &option->recvbuffer_size, sizeof( option->recvbuffer_size ) );
	(void) setsockopt( option->sockid, SOL_SOCKET, SO_SNDBUF, &option->sendbuffer_size, sizeof( option->sendbuffer_size ) );
	(void) getsockopt( option->sockid, SOL_SOCKET, SO_RCVBUF, &option->recvbuffer_size, &len );
	(void) getsockopt( option->sockid, SOL_SOCKET, SO_SNDBUF, &option->sendbuffer_size, &len );
	//printf( "default:SO_RCVBUF=%zd\n", option->recvbuffer_size );
	//printf( "default:SO_SNDBUF=%zd\n", option->sendbuffer_size );
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
GDT_SOCKET_ID gdt_server_socket( GDT_SOCKET_OPTION *option, int inetflag )
{
	GDT_SOCKET_ID sock = -1;
	char nbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	struct addrinfo hints, *res0;
	int opt , errcode;
	socklen_t opt_len;
	char* hostname = ( option->host_name_munit > 0 ) ? (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) : NULL;
	char* port = ( option->port_num_munit > 0 ) ? (char *)gdt_upointer( option->memory_pool,option->port_num_munit ) : NULL;
#ifdef __GDT_DEBUG__
	printf( "server_socket: port=%s, host=%s\n", port, hostname  );
#endif
	do{
		(void) memset( &hints, 0, sizeof( hints ) );
		if( inetflag == 1 ){
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
		hints.ai_flags    = AI_PASSIVE;
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
	struct addrinfo hints, *res0;
	struct timeval timeout;
	int errcode, width, val;
	socklen_t len;
	fd_set mask, write_mask, read_mask;
#ifdef __GDT_DEBUG__
//	printf( "client_socket: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
#endif
	do{
		(void) memset( &hints, 0, sizeof( hints ) );
		if( option->inetflag == 0 ){
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
		hints.ai_flags    = AI_PASSIVE;
		//hints.ai_flags = AI_NUMERICSERV;
		if( ( errcode = getaddrinfo( (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) , (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), &hints, &res0 ) ) != 0 ){
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
		gdt_set_sock_option( option );
		switch( option->socket_type )
		{
			case SOKET_TYPE_SERVER_TCP:
			case SOKET_TYPE_SERVER_UDP:
				if( ( option->sockid = gdt_server_socket( option, 0 ) ) <= 0 )
				{
					printf( "gdt_server_socket error: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
					break;
				}
				if( option->inetflag == 1 )
				{
					if( ( option->sockid6 = gdt_server_socket( option, 1 ) ) <= 0 )
					{
						printf( "gdt_server_socket ipv6 error: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool,option->port_num_munit ), (char *)gdt_upointer( option->memory_pool,option->host_name_munit ) );
						option->sockid6 = -1;
					}
				}
#ifdef __WINDOWS__
				switch( option->mode )
				{
					case SOCKET_MODE_SINGLE:	// single connection server mode
						gdt_single_task_server( option );
						break;
					case SOCKET_MODE_SELECT:	// select server mode
						gdt_select_server( option );
						break;
					case SOCKET_MODE_POOL:		// pool server mode
					case SOCKET_MODE_EPOOL:		// epool server mode
					case SOCKET_MODE_KQUEUE:	// kqueue server mode
					case SOCKET_MODE_PREFORK:	// prefork serve mode
						printf("not suppoerted windows server mode");
						break;
					case SOCKET_MODE_THREAD:	// thread server mode
						gdt_thread_server( option );
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
					case SOCKET_MODE_SINGLE:	// single connection server mode
						gdt_single_task_server( option );
						break;
					case SOCKET_MODE_SELECT:	// select server mode
						gdt_select_server( option );
						break;
					case SOCKET_MODE_POOL:		// pool server mode
						gdt_pool_server( option );
						break;
					case SOCKET_MODE_EPOOL:		// epool server mode
						gdt_epool_server( option );
						break;
					case SOCKET_MODE_PREFORK:	// prefork server mode
						gdt_prefork_server( option );
						break;
					case SOCKET_MODE_THREAD:	// thread server mode
						gdt_thread_server( option );
						break;
					case SOCKET_MODE_KQUEUE:	// kqueue server mode
						gdt_kqueue_server( option );
						break;
					case SOCKET_MODE_NONBLOCKING:
						gdt_nonblocking_server(option);
						break;
					default:
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
				switch( option->mode )
				{
					case SOCKET_MODE_SIMPLE_TERM:
						gdt_send_recv_client_term( option );
						break;
					case SOCKET_MODE_CLIENT_THREAD:
						gdt_thread_client( option );
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

void gdt_single_task_server( GDT_SOCKET_OPTION *option )
{
	GDT_SOCKET_ID sock;
	int pfret;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	char *pbuf;
	struct GDT_SERVER_CONNECTION_INFO childinfo;
	struct GDT_RECV_INFO* rinfo;
	struct sockaddr_storage from;
	socklen_t len, srlen;
	sock = option->sockid;
#ifdef __GDT_DEBUG__
	printf( "gdt_single_task_server: soc=%d\n", (int)option->sockid );
#endif
	gdt_make_connection_info_core(option,&childinfo,0);
	rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, childinfo.recvinfo_munit );
	pbuf = (char*)gdt_upointer( option->memory_pool, childinfo.recvbuf_munit );
	for(;;)
	{
		len = (socklen_t) sizeof( from );
		if( ( childinfo.id = accept( sock, (struct sockaddr *) &from, &len ) ) == -1 )
		{
			if( errno != EINTR ){
				perror("accept EINTR ");
				break;
			}
		}
		else{
			(void) getnameinfo( (struct sockaddr *) &from, len, childinfo.hbuf, sizeof( childinfo.hbuf ), childinfo.sbuf, sizeof( childinfo.sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
			gdt_initialize_connection_info( option, &childinfo );
			if( option->connection_start_callback != NULL ){
				option->connection_start_callback( (void*)&childinfo );
			}
			for(;;)
			{
				*pbuf = '\0';
				if( option->user_recv_function != NULL ){
					if( -1 == ( srlen = option->user_recv_function( &childinfo, childinfo.id, pbuf, buffer_size, 0 ) ) ){
						perror( "recv" );
						break;
					}
				}
				else{
					if( ( srlen = gdt_recv_with_timeout( option->recvtimeoutmode, option->t_sec, childinfo.id, pbuf, buffer_size, 0 ) ) == -1 ){
						perror( "recv" );
						break;
					}
				}
				if( srlen == 0 ){
					printf( "recv:EOF\n" );
					break;
				}
				if( option->plane_recv_callback != NULL )
				{
					childinfo.sockparam.acc	= childinfo.id;
					rinfo->tinfo			= &childinfo;
					rinfo->recvbuf_munit	= childinfo.recvbuf_munit;
					rinfo->recvlen			= srlen;
					rinfo->recvfrom			= childinfo.id;
					pfret = gdt_pre_packetfilter( option, rinfo, &childinfo.sockparam, childinfo.recvmsg_munit );
					if( pfret == -1 )
					{
						printf( "recv:EOF\n" );
						break;
					}
					if( pfret == 1 )
					{
						option->plane_recv_callback( (void*)rinfo );
						if( childinfo.sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
						{
							break;
						}
					}
				}
			}
			if( option->close_callback != NULL )
			{
				option->close_callback( (void*)&childinfo );
			}
			gdt_close_socket( &childinfo.id, NULL ); 
			gdt_free_sockparam( option, &childinfo.sockparam );
		}
	}
}

void gdt_select_server( GDT_SOCKET_OPTION *option )
{
	GDT_SOCKET_ID sock, acc;
	int pfret;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	GDT_SERVER_CONNECTION_INFO *child;
	char* pbuf;
	struct sockaddr_storage from;
	int child_no, width, i, pos;
	socklen_t len, srlen;
	struct timeval timeout;
	fd_set mask, read_mask;
	struct GDT_RECV_INFO *rinfo;
#ifdef __GDT_DEBUG__
	printf( "gdt_select_server: soc4=%d, soc6=%d\n", (int)option->sockid, (int)option->sockid6 );
#endif
	if( -1 == gdt_make_connection_info( option ) ){
		printf("gdt_make_connection_info error.");
		return;
	}
	child_no = 0;
	for(;;)
	{
		FD_ZERO( &mask );
		FD_SET( option->sockid, &mask );
		width = (int)(option->sockid + 1);
		if( option->sockid6 != -1 ){
			FD_SET( option->sockid6, &mask );
			width = (int)(option->sockid6 + 1);
		}
		for( i = 0; i < child_no; i++ ){
			child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->memory_pool,option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
			if( child->id != -1 ){
				FD_SET( child->id, &mask );
				if( child->id + 1 > width ){
					width = (int)(child->id + 1);
				}
			}
		}
		read_mask = mask;
		timeout.tv_sec = option->s_sec;
		timeout.tv_usec = option->s_usec;
		switch( select ( width, ( fd_set * ) &read_mask, NULL, NULL, &timeout ) ){
			case -1:
				perror("select");
				break;
			case 0:
				break;
			default:
				sock = -1;
				if( FD_ISSET( option->sockid, &read_mask ) )
				{
					sock = option->sockid;
				}
				else if( option->sockid6 != -1 )
				{
					if( FD_ISSET( option->sockid6, &read_mask ) )
					{
						sock = option->sockid6;
					}
				}
				if( sock != -1 )
				{
					len = (socklen_t) sizeof( from );
					if( ( acc = accept( sock, (struct sockaddr * ) &from, &len ) ) == -1 )
					{
						if( errno != EINTR ){
							perror( "accept" );
						}
					}
					else{
						pos = -1;
						for( i = 0; i < child_no; i++ )
						{
							child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->memory_pool,option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
							if( child->id == -1 )
							{
								pos = i;
								break;
							}
						}
						if( pos == -1 )
						{
							if( child_no+1 > option->maxconnection )
							{
#ifdef __GDT_DEBUG__
								printf( "child is full : count accept\n" );
#endif
								gdt_close_socket( &acc, NULL );
							}
							else{
								child_no++;
								pos = child_no -1;
							}
						}
						if( pos != -1 ){
							child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->memory_pool,option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), pos );
							(void) getnameinfo( (struct sockaddr *) &from, len, 
									child->hbuf, sizeof( child->hbuf ), child->sbuf, sizeof( child->sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
							child->id = acc;
							gdt_initialize_connection_info( option, child );
							if( option->connection_start_callback != NULL ){
								option->connection_start_callback( (void*)child );
							}
						}
					}
				}
				for( i = 0; i < child_no; i++ )
				{
					child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->memory_pool,option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
					if( child->id != -1 )
					{
						if( FD_ISSET( child->id, &read_mask ) )
						{
							pbuf = (char*)gdt_upointer( option->memory_pool, child->recvbuf_munit );
							*pbuf = '\0';
							if( option->user_recv_function != NULL ){
								if( -1 == ( srlen = option->user_recv_function( child, child->id, pbuf, buffer_size, 0 ) ) ){
									perror( "recv" );
									gdt_close_socket( &child->id, NULL );
								}
							}
							else{
								if( ( srlen = gdt_recv_with_timeout( TIMEOUT_MODE_NONE, 0, child->id, pbuf, buffer_size, 0 ) ) == -1 ){
									perror( "recv" );
									gdt_close_socket( &child->id, NULL );
								}
							}
							if( srlen == 0 ){
								gdt_close_socket( &child->id, NULL );
							}
							else if( option->plane_recv_callback != NULL )
							{
								rinfo = (struct GDT_RECV_INFO *)gdt_upointer( option->memory_pool, child->recvinfo_munit );
								child->sockparam.acc = child->id;
								rinfo->tinfo = child;
								rinfo->recvlen = srlen;
								rinfo->recvbuf_munit = child->recvbuf_munit;
								rinfo->recvfrom = child->id;
								pfret = gdt_pre_packetfilter( option, rinfo, &child->sockparam, child->recvmsg_munit );
								if( pfret == -1 ){
									gdt_close_socket( &child->id, NULL );
								}
								else if( pfret == 1 ){
									option->plane_recv_callback( (void*)rinfo );
									if( child->sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
									{
										gdt_close_socket( &child->id, NULL );
									}
								}
							}
						}
						if( child->id == -1 )
						{
							if( option->close_callback != NULL )
							{
								option->close_callback( (void*)child );
							}
							gdt_free_sockparam( option, &child->sockparam );
						}
					}
				}
				break;
		}
	}
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
			if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				perror("accept");
			}
		}
		if (acc == -1 && option->sockid6 != -1) {
			if (-1 == (acc = accept(option->sockid6, (struct sockaddr *) &from, &len))) {
				if (errno != 0 && errno != EINTR && errno != EAGAIN) {
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
						child->sockparam.type = SOCK_TYPE_NORMAL;
					}
					gdt_set_block(child->id, 0);
					gdt_initialize_connection_info(option, child);
					if (option->connection_start_callback != NULL){
						option->connection_start_callback((void*)child);
					}
					break;
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
						if (errno != 0 && errno != EAGAIN)
						{
							perror("recv");
							gdt_close_socket(&child->id, NULL);
						}
					}
				}
				else {
					if ((srlen = recv(child->id, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0)) == -1) {
						if (errno != 0 && errno != EAGAIN)
						{
							perror("recv");
							gdt_close_socket(&child->id, NULL);
						}
					}
				}
				if (srlen == -1) {
				}
				else if (srlen == 0) {
					perror("recv");
					gdt_close_socket(&child->id, NULL);
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
					//memset((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), 0, buffer_size);
				}
				if (child->id == -1)
				{
					if (option->close_callback != NULL)
					{
						option->close_callback((void*)child);
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
#ifdef __WINDOWS__
#else
			struct sockaddr_in* pfrom = (struct sockaddr_in*)&child->sockparam.from;
			printf("(%d)%s:%d\n",(int)len,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port));
#endif
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
#ifdef __GDT_DEBUG__
	printf("gdt_nonblocking_client: soc4=%d, soc6=%d\n", (int)option->sockid, (int)option->sockid6);
#endif
	size_t buffer_size = sizeof(char) * (option->recvbuffer_size);
	gdt_set_block(option->sockid, 0);
	option->connection_munit = gdt_create_munit( option->memory_pool, sizeof( GDT_SERVER_CONNECTION_INFO ), MEMORY_TYPE_DEFAULT );
	if( option->connection_munit == -1 )
	{
		return;
	}
	GDT_SERVER_CONNECTION_INFO * child = (GDT_SERVER_CONNECTION_INFO*)GDT_POINTER(option->memory_pool,option->connection_munit);
	child->id = option->sockid;
	child->recvbuf_munit = gdt_create_munit( option->memory_pool, buffer_size, MEMORY_TYPE_DEFAULT );
	child->recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
	child->recvmsg_munit = gdt_create_munit( option->memory_pool, sizeof( char) * ( option->msgbuffer_size ), MEMORY_TYPE_DEFAULT );
	child->gdt_socket_option = option;
	gdt_init_socket_param( &child->sockparam );
	child->sockparam.acc = option->sockid;
	if (option->socket_type == SOKET_TYPE_CLIENT_TCP){
		child->sockparam.type = SOCK_TYPE_NORMAL;
	}
	else {
		child->sockparam.type = SOCK_TYPE_NORMAL_UDP;
	}
	if( option->connection_start_callback != NULL )
	{
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
					if (errno != 0 && errno != EAGAIN)
					{
						perror("recv");
						gdt_close_socket(&child->id, NULL);
					}
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
				if( *((char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit)) == '\0' )
				{
					printf("gdt_recv invalid packet\n");
					return;
				}
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
			}
			if (child->id == -1)
			{
                gdt_free_sockparam(option, &child->sockparam);
				if (option->close_callback != NULL)
				{
					option->close_callback((void*)child);
				}
			}
		}
	}
	else if (option->socket_type == SOKET_TYPE_CLIENT_UDP)
	{
		child = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer(option->memory_pool, option->connection_munit, sizeof(GDT_SERVER_CONNECTION_INFO), 0);
		child->sockparam.fromlen = sizeof(child->sockparam.from);
		if ((srlen = recvfrom(option->sockid, (char*)GDT_POINTER(option->memory_pool, child->recvbuf_munit), buffer_size, 0, (struct sockaddr *)&child->sockparam.from, &child->sockparam.fromlen)) == -1)
		{
			if (errno != 0 && errno != EINTR && errno != EAGAIN) {
				perror("recvfrom");
			}
			return;
		}
		if (srlen == -1) {
		}
		else if (srlen == 0) {
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
#ifdef __WINDOWS__
#else
			struct sockaddr_in* pfrom = (struct sockaddr_in*)&child->sockparam.from;
			printf("(%d)%s:%d\n", (int)srlen, inet_ntoa(pfrom->sin_addr), ntohs(pfrom->sin_port));
#endif
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

#ifdef __WINDOWS__
#else
void gdt_pool_server( GDT_SOCKET_OPTION *option )
{
#ifdef __GDT_DEBUG__
	printf( "gdt_pool_server: soc=%d\n", option->sockid );
#endif
	GDT_SOCKET_ID sock = option->sockid;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	char hbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	struct GDT_SERVER_CONNECTION_INFO *child;
	char* pbuf;
	struct GDT_RECV_INFO *rinfo;
	struct sockaddr_storage from;
	int acc, child_no, i, j, count, pos;
	socklen_t len, srlen;
	int cpos, spos;
	int pfret;
	struct pollfd targets[ option->maxconnection + 2 ];	// maxconnection + ipv4 + ipv6
	if( -1 == gdt_make_connection_info( option ) ){
		printf("gdt_make_connection_info error.");
		return;
	}
	child = (struct GDT_SERVER_CONNECTION_INFO *)gdt_upointer( option->memory_pool, option->connection_munit );
	child_no = 0;
	for(;;)
	{
		count=0;
		targets[count].fd = option->sockid;
		targets[count].events = POLLIN;
		count++;
		if( option->sockid6 != -1 )
		{
			targets[count].fd = option->sockid6;
			targets[count].events = POLLIN;
			count++;
		}
		spos = count;
		for( i = 0; i < child_no; i++ )
		{
			if( child[i].id != -1 )
			{
				targets[count].fd = child[i].id;
				targets[count].events = POLLIN;
				count++;
			}
		}
		switch( poll( targets, count, option->s_sec * 1000 ) )
		{
			case -1:
				perror("poll");
				break;
			case 0:
				break;
			default:
				sock = -1;
				if( targets[0].revents & POLLIN )
				{
					sock = targets[0].fd;
				}
				else if( option->sockid6 != -1 && targets[1].revents & POLLIN )
				{
					sock = targets[1].fd;
				}
				if( sock != -1 )
				{
					len = (socklen_t) sizeof( from );
					if( ( acc = accept( sock, (struct sockaddr * ) &from, &len ) ) == -1 )
					{
						if( errno != EINTR ){
							perror( "accept" );
						}
					}
					else{
						(void) getnameinfo( (struct sockaddr *) &from, len, 
								hbuf, sizeof( hbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
#ifdef __GDT_DEBUG__
//						printf( "accept:%s:%s\n", hbuf, sbuf );
#endif
						pos = -1;
						for( i = 0; i < child_no; i++ )
						{
							if( child[i].id == -1 )
							{
								pos = i;
								break;
							}
						}
						if( pos == -1 ){
							if( child_no + 1 > option->maxconnection ){
#ifdef __GDT_DEBUG__
								printf( "child is full : count accept\n" );
#endif
								gdt_close_socket(&acc, NULL);
							}
							else{
								child_no++;
								pos = child_no -1;
							}
						}
						if( pos != -1 ){
							child[pos].id = acc;
							gdt_initialize_connection_info( option, &child[pos] );
							memcpy( child[pos].hbuf, hbuf, NI_MAXHOST );
							memcpy( child[pos].sbuf, sbuf, NI_MAXSERV );
							if( option->connection_start_callback != NULL )
							{
								option->connection_start_callback( (void*)&child[pos] );
							}
						}
					}
				}
				for( i = spos; i < count; i++ )
				{
					if( targets[i].revents & ( POLLIN | POLLERR ) )
					{
						cpos = -1;
						for( j = 0; j < child_no; j++ )
						{
							if( child[j].id == targets[i].fd )
							{
								cpos = j;
								break;
							}
						}
						child[cpos].sockparam.acc = targets[i].fd;
						rinfo = (struct GDT_RECV_INFO *)gdt_upointer( option->memory_pool, child[cpos].recvinfo_munit );
						pbuf = (char*)gdt_upointer( option->memory_pool, child[cpos].recvbuf_munit );
						*(pbuf) = '\0';
						//memset( pbuf, 0, buffer_size );
						if( option->user_recv_function != NULL ){
							if( -1 == ( srlen = option->user_recv_function( &child[cpos], targets[i].fd, pbuf, buffer_size, 0 ) ) ){
								perror( "recv" );
								gdt_close_socket(&child[cpos].id, NULL);
							}
						}
						else{
							if( ( srlen = gdt_recv_with_timeout( TIMEOUT_MODE_NONE, 0, targets[i].fd, pbuf, buffer_size, 0 ) ) == -1 )
							{
								perror( "recv" );
								gdt_close_socket(&child[cpos].id, NULL);
							}
						}
						if( srlen == 0 )
						{
							
						}
						else if( option->plane_recv_callback != NULL )
						{
							rinfo->tinfo = &child[cpos];
							rinfo->recvlen = srlen;
							rinfo->recvbuf_munit = child[cpos].recvbuf_munit;
							rinfo->recvfrom = child[cpos].id;
							pfret = gdt_pre_packetfilter( option, rinfo, &child[cpos].sockparam, child[cpos].recvmsg_munit );
							if( pfret == -1 )
							{
								gdt_close_socket(&child[cpos].id, NULL);
							}
							else if( pfret == 1 )
							{
								option->plane_recv_callback( (void*)rinfo );
								if( child[cpos].sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
								{
									gdt_close_socket(&child[cpos].id, NULL);
								}
							}
						}
						if( child[cpos].id == -1 )
						{
							if( option->close_callback != NULL )
							{
								option->close_callback( (void*)&child[cpos] );
							}
							gdt_free_sockparam( option, &child[cpos].sockparam );
						}
					}
				}
				break;
		}
	}
}

void gdt_epool_server( GDT_SOCKET_OPTION *option )
{
#ifdef USE_EPOOL
#ifdef __GDT_DEBUG__
	printf( "gdt_epool_server: soc=%d\n", option->sockid );
#endif
	GDT_SOCKET_ID sock = option->sockid;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	char hbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	GDT_SERVER_CONNECTION_INFO *child;
	struct sockaddr_storage from;
	int acc, count, i, j, epollfd, nfds;
	socklen_t len, srlen;
	int cpos, pos, pfret;
	char* pbuf;
	struct GDT_RECV_INFO *rinfo;
	struct epoll_event ev, events[option->maxconnection];
	if( ( epollfd = epoll_create( option->maxconnection + 1 ) ) == -1 )
	{
		perror("epoll_create");
		return;
	}
	ev.data.fd = sock;
	ev.events = EPOLLIN;
	if( epoll_ctl( epollfd, EPOLL_CTL_ADD, sock, &ev ) == -1 ){
		perror("epoll_ctl");
		(void) close( epollfd );
		return;
	}
	if( option->sockid6 != -1 )
	{
		ev.data.fd = option->sockid6;
		ev.events = EPOLLIN;
		if( epoll_ctl( epollfd, EPOLL_CTL_ADD, option->sockid6, &ev ) == -1 ){
			perror("epoll_ctl");
			(void) close( epollfd );
			return;
		}
	}
	if( -1 == gdt_make_connection_info( option ) ){
		printf("gdt_make_connection_info error.");
		return;
	}
	child = (struct GDT_SERVER_CONNECTION_INFO *)gdt_upointer( option->memory_pool, option->connection_munit );
	count = 0;
	for(;;)
	{
		switch( ( nfds = epoll_wait( epollfd, events, option->maxconnection + 1, option->s_sec * 1000 ) ) ){
			case -1:
				perror("poll");
				break;
			case 0:
				break;
			default:
				for( i = 0; i < nfds; i++ )
				{
					if( events[i].data.fd == option->sockid || ( option->sockid6 != -1 && events[i].data.fd == option->sockid6 ) )
					{
						len = (socklen_t) sizeof( from );
						if( ( acc = accept( events[i].data.fd, (struct sockaddr * ) &from, &len ) ) == -1 )
						{
							if( errno != EINTR ){
								perror( "accept" );
							}
						}
						else{
							(void) getnameinfo( (struct sockaddr *) &from, len, hbuf, sizeof( hbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
#ifdef __GDT_DEBUG__
//							printf( "accept:%s:%s\n", hbuf, sbuf );
#endif
							if( count + 1 > option->maxconnection ){
#ifdef __GDT_DEBUG__
								printf( "child is full : count accept\n" );
#endif
								gdt_close_socket(&acc, NULL);
							}
							else{
								ev.data.fd = acc;
								ev.events = EPOLLIN;
								if( epoll_ctl( epollfd, EPOLL_CTL_ADD, acc, &ev ) == -1 )
								{
									perror("epoll_ctl");
									gdt_close_socket(&acc, NULL);
									(void) close(epollfd);
									return;
								}
								pos = -1;
								for( j = 0; j < option->maxconnection; j++ )
								{
									if( child[j].id == -1 )
									{
										pos = j;
										break;
									}
								}
								if( pos >= 0 )
								{
									child[pos].id = acc;
									memcpy( child[pos].hbuf, hbuf, NI_MAXHOST );
									memcpy( child[pos].sbuf, sbuf, NI_MAXSERV );
									gdt_initialize_connection_info( option, &child[pos] );
									if( option->connection_start_callback != NULL )
									{
										option->connection_start_callback( (void*)&child[pos] );
									}
								}else{
#ifdef __GDT_DEBUG__
									printf( "child is full : count accept\n" );
#endif
									gdt_close_socket(&acc, NULL);
									(void) close(epollfd);
									return;
								}
								count++;
							}
						}
					}
					else{
						cpos = -1;
						for( j = 0; j < option->maxconnection; j++ )
						{
							if( child[j].id == events[i].data.fd )
							{
								cpos = j;
								break;
							}
						}
						child[cpos].sockparam.acc = events[i].data.fd;
						rinfo = (struct GDT_RECV_INFO *)gdt_upointer( option->memory_pool, child[cpos].recvinfo_munit );
						pbuf = (char*)gdt_upointer( option->memory_pool, child[cpos].recvbuf_munit );
						*(pbuf) = '\0';
						//memset( pbuf, 0, buffer_size );
						if( option->user_recv_function != NULL ){
							if( -1 == ( srlen = option->user_recv_function( &child[cpos], events[i].data.fd, pbuf, buffer_size, 0 ) ) ){
								perror( "recv" );
								if( epoll_ctl( epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev ) == -1 ){
									perror( "epoll_ctl");
									(void) close( events[i].data.fd );
									(void) close( epollfd );
									return;
								}
								(void) close( events[i].data.fd );
								count--;
								child[cpos].id = -1;
							}
						}
						else{
							if( ( srlen = gdt_recv_with_timeout( TIMEOUT_MODE_NONE, 0, events[i].data.fd, pbuf, buffer_size, 0 ) ) == -1 ){
								perror( "recv" );
								if( epoll_ctl( epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev ) == -1 ){
									perror( "epoll_ctl");
									(void) close( events[i].data.fd );
									(void) close( epollfd );
									return;
								}
								(void) close( events[i].data.fd );
								count--;
								child[cpos].id = -1;
							}
						}
						if( srlen == 0 ){
							if( epoll_ctl( epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev ) == -1 ){
								perror( "epoll_ctl");
								(void) close( events[i].data.fd );
								(void) close( epollfd );
								return;
							}
							(void) close( events[i].data.fd );
							count--;
							child[cpos].id = -1;
						}
						else if( option->plane_recv_callback != NULL )
						{
							rinfo->tinfo = &child[cpos];
							rinfo->recvlen = srlen;
							rinfo->recvbuf_munit = child[cpos].recvbuf_munit;
							rinfo->recvfrom = child[cpos].id;
							pfret = gdt_pre_packetfilter( option, rinfo, &child[cpos].sockparam, child[cpos].recvmsg_munit );
							if( pfret == -1 ){
								if( epoll_ctl( epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev ) == -1 ){
									perror( "epoll_ctl");
									(void) close( events[i].data.fd );
									(void) close( epollfd );
									return;
								}
								(void) close( events[i].data.fd );
								count--;
								child[cpos].id = -1;
							}
							else if( pfret == 1 ){
								option->plane_recv_callback( (void*)rinfo );
								if( child[cpos].sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
								{
									if( epoll_ctl( epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev ) == -1 ){
										perror( "epoll_ctl");
										(void) close( events[i].data.fd );
										(void) close( epollfd );
										return;
									}
									(void) close( events[i].data.fd );
									count--;
									child[cpos].id = -1;
								}
							}
						}
						if( child[cpos].id == -1 )
						{
							if( option->close_callback != NULL )
							{
								option->close_callback( (void*)&child[cpos] );
							}
							gdt_free_sockparam( option, &child[cpos].sockparam );
						}
					}
				}
				break;
		}
	}
#else
	printf( "epool disabled\n" );
#endif // ifdef USE_EPOOL
}

void gdt_kqueue_server( GDT_SOCKET_OPTION *option )
{
#ifdef USE_KQUEUE
#ifdef __GDT_DEBUG__
	printf( "gdt_kqueue_server: soc=%d\n", option->sockid );
#endif
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	char hbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	struct GDT_SERVER_CONNECTION_INFO *child;
	char* pbuf;
	struct GDT_RECV_INFO *rinfo;
	struct sockaddr_storage from;
	int acc,i, j, count, pos;
	socklen_t len, srlen;
	int cpos;
	int pfret;
	int kq;
	int retevent;
	struct kevent kev;
	struct kevent kqevents[ option->maxconnection + 2 ];	// maxconnection + ipv4 + ipv6
	if( -1 == ( kq = kqueue() ) ){
		perror("kqueue");
		return;
	}
	count = 0;
	EV_SET( &kev, option->sockid, EVFILT_READ, EV_ADD, 0, 0, 0 );
	count++;
	if( -1 == kevent( kq, &kev, 1, NULL, 0, NULL ) ){
		printf("kevent error.\n");
	}
	if( option->sockid6 != -1 )
	{
		EV_SET( &kev, option->sockid6, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL );
		if( -1 == kevent( kq, &kev, 1, NULL, 0, NULL ) ){
			printf("kevent error.\n");
		}
		count++;
	}
	if( -1 == gdt_make_connection_info( option ) ){
		printf("gdt_make_connection_info error.");
		return;
	}
	child = (struct GDT_SERVER_CONNECTION_INFO *)gdt_upointer( option->memory_pool, option->connection_munit );
	for(;;)
	{
		switch( (retevent = kevent( kq, NULL, 0, kqevents, option->maxconnection + 2, NULL )) )
		{
			case -1:
				perror("kevent");
				break;
			case 0:
				break;
			default:
				for( i = 0; i < retevent; i++ )
				{
					if( kqevents[i].ident == option->sockid || kqevents[i].ident == option->sockid6 )
					{
						if( kqevents[i].flags & EV_EOF ){
							printf("EOF:%d\n",i);
						}
						if( kqevents[i].flags & EV_ERROR ){
							printf("ERROR:%d\n",i);
						}
						else{
							len = (socklen_t) sizeof( from );
							if( ( acc = accept( kqevents[i].ident, (struct sockaddr * ) &from, &len ) ) == -1 )
							{
								if( errno != EINTR ){
									perror( "accept" );
								}
							}
							else{
								(void) getnameinfo( (struct sockaddr *) &from, len, 
										hbuf, sizeof( hbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
								//printf( "accept:%s:%s\n", hbuf, sbuf );
								if( count + 1 > option->maxconnection ){
#ifdef __GDT_DEBUG__
									printf( "child is full : count accept\n" );
#endif
									(void) close( acc );
								}
								else{
									EV_SET( &kev, acc, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL );
									if( -1 == kevent( kq, &kev, 1, NULL, 0, NULL ) ){
										printf("kevent error.\n");
										(void) close(acc);
										(void) close(kq);
										return;
									}
									pos = -1;
									for( j = 0; j < option->maxconnection; j++ )
									{
										if( child[j].id == -1 )
										{
											pos = j;
											break;
										}
									}
									if( pos >= 0 )
									{
										child[pos].id = acc;
										memcpy( child[pos].hbuf, hbuf, NI_MAXHOST );
										memcpy( child[pos].sbuf, sbuf, NI_MAXSERV );
										gdt_initialize_connection_info( option, &child[pos] );
										if( option->connection_start_callback != NULL )
										{
											option->connection_start_callback( (void*)&child[pos] );
										}
									}else{
#ifdef __GDT_DEBUG__
										printf( "child is full : count accept\n" );
#endif
										(void) close(acc);
										return;
									}
									count++;
								}
							}
						}
					}
					else{
						cpos = -1;
						if( kqevents[i].flags & EV_EOF ){
							printf("EOF:%d\n",i);
						}
						if( kqevents[i].flags & EV_ERROR ){
							printf("ERROR:%d\n",i);
						}
						for( j = 0; j < option->maxconnection; j++ )
						{
							if( child[j].id == kqevents[i].ident )
							{
								cpos = j;
								break;
							}
						}
						child[cpos].sockparam.acc = kqevents[i].ident;
						rinfo = (struct GDT_RECV_INFO *)gdt_upointer( option->memory_pool, child[cpos].recvinfo_munit );
						pbuf = (char*)gdt_upointer( option->memory_pool, child[cpos].recvbuf_munit );
						//memset( pbuf, 0, buffer_size );
						*(pbuf) = '\0';
						if( option->user_recv_function != NULL ){
							if( -1 == ( srlen = option->user_recv_function( &child[cpos], child[cpos].sockparam.acc, pbuf, buffer_size, 0 ) ) ){
								perror( "recv" );
								EV_SET(&kev, child[cpos].sockparam.acc, EVFILT_READ, EV_DELETE, 0, 0, NULL);
								if( kevent( kq, &kev, 1, NULL, 0, NULL ) == -1 ){
									(void) close( child[cpos].sockparam.acc );
									(void) close(kq);
									return;
								}
								(void) close( child[cpos].sockparam.acc );
								count--;
								child[cpos].id = -1;
							}
						}
						else{
							if( ( srlen = gdt_recv_with_timeout( TIMEOUT_MODE_NONE, 0, child[cpos].sockparam.acc, pbuf, buffer_size, 0 ) ) == -1 ){
								perror( "recv" );
								EV_SET(&kev, child[cpos].sockparam.acc, EVFILT_READ, EV_DELETE, 0, 0, NULL);
								if( kevent( kq, &kev, 1, NULL, 0, NULL ) == -1 ){
									(void) close( child[cpos].sockparam.acc );
									(void) close(kq);
									return;
								}
								(void) close( child[cpos].sockparam.acc );
								count--;
								child[cpos].id = -1;
							}
						}
						if( srlen == 0 ){
							EV_SET(&kev, child[cpos].sockparam.acc, EVFILT_READ, EV_DELETE, 0, 0, NULL);
							if( kevent( kq, &kev, 1, NULL, 0, NULL ) == -1 ){
								(void) close( child[cpos].sockparam.acc );
								(void) close(kq);
								return;
							}
							(void) close( child[cpos].sockparam.acc );
							count--;
							child[cpos].id = -1;
						}
						else if( option->plane_recv_callback != NULL )
						{
							rinfo->tinfo = &child[cpos];
							rinfo->recvlen = srlen;
							rinfo->recvbuf_munit = child[cpos].recvbuf_munit;
							rinfo->recvfrom = child[cpos].id;
							pfret = gdt_pre_packetfilter( option, rinfo, &child[cpos].sockparam, child[cpos].recvmsg_munit );
							if( pfret == -1 ){
								EV_SET(&kev, child[cpos].sockparam.acc, EVFILT_READ, EV_DELETE, 0, 0, NULL);
								if( kevent( kq, &kev, 1, NULL, 0, NULL ) == -1 ){
									(void) close( child[cpos].sockparam.acc );
									(void) close(kq);
									return;
								}
								(void) close( child[cpos].sockparam.acc );
								count--;
								child[cpos].id = -1;
							}
							else if( pfret == 1 ){
								option->plane_recv_callback( (void*)rinfo );
								if( child[cpos].sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
								{
									EV_SET(&kev, child[cpos].sockparam.acc, EVFILT_READ, EV_DELETE, 0, 0, NULL);
									if( kevent( kq, &kev, 1, NULL, 0, NULL ) == -1 ){
										(void) close( child[cpos].sockparam.acc );
										(void) close(kq);
										return;
									}
									(void) close( child[cpos].sockparam.acc );
									count--;
									child[cpos].id = -1;
								}
							}
						}
						if( child[cpos].id == -1 )
						{
							if( option->close_callback != NULL )
							{
								option->close_callback( (void*)&child[cpos] );
							}
							gdt_free_sockparam( option, &child[cpos].sockparam );
						}
					}
				}
				break;
		}
	}
#else
	printf( "kqueue disabled\n" );
#endif
}

void gdt_prefork_server( GDT_SOCKET_OPTION *option )
{
#ifdef __GDT_DEBUG__
	printf( "gdt_prefork_server: soc=%d\n", option->sockid );
#endif
	int i;
	pid_t parentpid;
	pid_t pid;
	GDT_SERVER_CONNECTION_INFO *pforkinfo;
	do{
		if( option->socket_type == SOKET_TYPE_SERVER_UDP )
		{
			if( option->maxconnection > 1 ){
				printf( "change udp maxconnection %zd -> 1\n", option->maxconnection );
				option->maxconnection = 1;
			}
		}
		if( option->mmap_memory_pool == NULL ){
			printf("gdt_prefork_server option->mmap_memory_pool is null");
			break;
		}
		option->connection_munit = gdt_create_munit( option->mmap_memory_pool,sizeof( GDT_SERVER_CONNECTION_INFO ) * ( option->maxconnection ), MEMORY_TYPE_DEFAULT );
		parentpid = getpid();
		option->lock_file_munit	= gdt_create_munit( option->memory_pool, SIZE_BYTE * ( 128 ), MEMORY_TYPE_DEFAULT );
		(void) snprintf( (char *)gdt_upointer( option->memory_pool, option->lock_file_munit ), gdt_usize( option->memory_pool, option->lock_file_munit ), "proc_lock_file_%d", option->sockid );
		if( ( option->lock_file_fd = open( (char *)gdt_upointer( option->memory_pool,option->lock_file_munit ), O_RDWR | O_CREAT, 0666 ) ) == -1 )
		{
			perror("open");
			break;
		}
		(void) unlink( (char *)gdt_upointer( option->memory_pool,option->lock_file_munit ) );
		for( i = 0; i < option->maxconnection; i++ )
		{
			if( ( pid = fork() ) == 0 )
			{
				if( option->mmap_memory_pool == NULL ){
					pforkinfo = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->memory_pool,option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
				}else{
					pforkinfo = (GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->mmap_memory_pool,option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
				}
				pforkinfo->parent_processid			= parentpid;
				pforkinfo->current_processid		= getpid();
				pforkinfo->index			= i;
				pforkinfo->recvbuf_munit	= -1;
				pforkinfo->recvinfo_munit	= -1;
				pforkinfo->recvmsg_munit	= -1;
				pforkinfo->user_data_munit		= -1;
				pforkinfo->gdt_socket_option			= option;
				gdt_init_socket_param( &pforkinfo->sockparam );
				if( option->socket_type == SOKET_TYPE_SERVER_TCP )
				{
					gdt_accept_loop_fork( pforkinfo );
				}
				else if( option->socket_type == SOKET_TYPE_SERVER_UDP )
				{
					gdt_recvfrom_loop_fork( pforkinfo );
				}
				_exit(1);
			}
			else if( pid > 0 )
			{
				// parent process
			}
			else{
				perror("fork");
			}
		}
	}while( false );
}

void gdt_accept_loop_fork( GDT_SERVER_CONNECTION_INFO *forkinfo )
{
#ifdef __ANDROID__
	printf("fork server use lockf. android not supported\n");
#else
	char hbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
	struct sockaddr_storage from;
	socklen_t len;
	int accstatus = 0;
	GDT_SOCKET_ID sock;
	int width;
	struct timeval timeout;
	fd_set mask, read_mask;
	GDT_SOCKET_OPTION *option = forkinfo->gdt_socket_option;
#ifdef __GDT_DEBUG__
//	( void ) printf( "gdt_accept_loop_fork[parent:%d][current:%d][index:%d]\n", forkinfo->parent_processid, forkinfo->current_processid, forkinfo->index );
#endif
	for(;;)
	{
		( void ) lockf( option->lock_file_fd, F_LOCK, 0 );
		accstatus = 0;
		while( accstatus == 0 )
		{
			FD_ZERO( &mask );
			FD_SET( option->sockid, &mask );
			width = option->sockid + 1;
			if( option->sockid6 != -1 ){
				FD_SET( option->sockid6, &mask );
				width = option->sockid6 + 1;
			}
			read_mask = mask;
			timeout.tv_sec = option->s_sec;
			timeout.tv_usec = 0;
			switch( select ( width, ( fd_set * ) &read_mask, NULL, NULL, &timeout ) )
			{
				case -1:
					perror("select");
					break;
				case 0:
					break;
				default:
					sock = -1;
					if( FD_ISSET( option->sockid, &read_mask ) )
					{
						sock = option->sockid;
					}
					if( option->sockid6 != -1 )
					{
						if( FD_ISSET( option->sockid6, &read_mask ) )
						{
							sock = option->sockid6;
						}
					}
					if( sock != -1 )
					{
						len = (socklen_t) sizeof( from );
						if( ( forkinfo->sockparam.acc = accept( sock, (struct sockaddr * ) &from, &len ) ) == -1 )
						{
							if( errno != EINTR ){
								perror( "accept" );
								accstatus = -1;
							}
						}
						else{
							accstatus = 1;
						}
					}
					break;
			}
		}
		
		if( forkinfo->sockparam.acc == -1 )
		{
			if( errno != EINTR ) {
				perror( "accept" );
			}
			( void ) lockf( option->lock_file_fd, F_ULOCK, 0 );
		}
		else{
			( void ) getnameinfo( (struct sockaddr *) &from, len, hbuf, sizeof( hbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
#ifdef __GDT_DEBUG__
//			( void ) printf( "accept:%s:%s\n", hbuf, sbuf );
#endif
			( void ) lockf( option->lock_file_fd, F_ULOCK, 0 );
			gdt_initialize_connection_info( option, forkinfo );
			memcpy( forkinfo->hbuf, hbuf, NI_MAXHOST );
			memcpy( forkinfo->sbuf, sbuf, NI_MAXSERV );
			if( option->connection_start_callback != NULL )
			{
				option->connection_start_callback( (void *)forkinfo );
			}
			gdt_recv_loop_fork( forkinfo );
			( void )close( forkinfo->sockparam.acc );
			forkinfo->sockparam.acc = -1;
			gdt_free_sockparam( option, &forkinfo->sockparam );
		}
	}
	gdt_free_sockparam( option, &forkinfo->sockparam );
#endif
}

void gdt_recv_loop_fork( GDT_SERVER_CONNECTION_INFO *forkinfo )
{
#ifdef __GDT_DEBUG__
//	printf( "<%d>gdt_recv_loop_fork: acc=%d\n", getpid(), forkinfo->sockparam.acc );
#endif
	char *pbuf;
	ssize_t len;
	GDT_SOCKET_OPTION *option = forkinfo->gdt_socket_option;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	int pfret;
	if( forkinfo->recvbuf_munit < 0 ){
		forkinfo->recvbuf_munit	= gdt_create_munit( option->memory_pool,buffer_size, MEMORY_TYPE_DEFAULT );
	}
	if( forkinfo->recvmsg_munit < 0 ){
		forkinfo->recvmsg_munit	= gdt_create_munit( option->memory_pool,buffer_size, MEMORY_TYPE_DEFAULT );
	}
	if( forkinfo->recvinfo_munit < 0 )
	{
		forkinfo->recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
	}
	struct GDT_RECV_INFO* rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, forkinfo->recvinfo_munit );
	pbuf = (char*)gdt_upointer( option->memory_pool,forkinfo->recvbuf_munit );
	for(;;)
	{
		//memset( pbuf, 0, buffer_size );
		*(pbuf) = '\0';
		if( option->user_recv_function != NULL ){
			if( -1 == ( len = option->user_recv_function( forkinfo, forkinfo->sockparam.acc, pbuf, buffer_size, 0 ) ) ){
				perror( "recv" );
				break;
			}
		}
		else{
			if( ( len = gdt_recv_with_timeout( option->recvtimeoutmode, option->t_sec, forkinfo->sockparam.acc, pbuf, buffer_size, 0 ) ) == -1 ){
				perror( "recv" );
				break;
			}
		}
		if( len == 0){
			break;
		}
		if( option->plane_recv_callback != NULL )
		{
			rinfo->tinfo			= forkinfo;
			rinfo->recvbuf_munit	= forkinfo->recvbuf_munit;
			rinfo->recvlen			= len;
			rinfo->recvfrom			= forkinfo->sockparam.acc;
			pfret = gdt_pre_packetfilter( option, rinfo, &forkinfo->sockparam, forkinfo->recvmsg_munit );
			if( pfret == -1 ){
				break;
			}
			else if( pfret == 1 ){
				option->plane_recv_callback( (void*)rinfo );
				if( forkinfo->sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
				{
					break;
				}
			}
		}
	}
	if( option->close_callback != NULL )
	{
		option->close_callback( (void*)forkinfo );
	}
}

void gdt_recvfrom_loop_fork( struct GDT_SERVER_CONNECTION_INFO *forkinfo )
{
	int pfret;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	char *pbuf;
	ssize_t len;
	GDT_SOCKET_OPTION *option = forkinfo->gdt_socket_option;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
#ifdef __GDT_DEBUG__
	printf( "<%d>gdt_recvfrom_loop_fork\n", getpid() );
#endif
	if( forkinfo->recvbuf_munit < 0 ){
		forkinfo->recvbuf_munit	= gdt_create_munit( option->memory_pool,buffer_size, MEMORY_TYPE_DEFAULT );
	}
	if( forkinfo->recvmsg_munit < 0 ){
		forkinfo->recvmsg_munit	= gdt_create_munit( option->memory_pool,buffer_size, MEMORY_TYPE_DEFAULT );
	}
	if( forkinfo->recvinfo_munit < 0 )
	{
		forkinfo->recvinfo_munit = gdt_create_munit( option->memory_pool,sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
	}
	struct GDT_RECV_INFO* rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, forkinfo->recvinfo_munit );
	pbuf = (char*)gdt_upointer( option->memory_pool,forkinfo->recvbuf_munit );
	forkinfo->sockparam.acc = option->sockid;
	for(;;)
	{
		for(;;)
		{
			//memset( pbuf, 0, buffer_size );
			*(pbuf) = '\0';
			forkinfo->sockparam.fromlen = sizeof(forkinfo->sockparam.from);
			if( ( len = recvfrom( option->sockid, pbuf, buffer_size, 0, (struct sockaddr *)&forkinfo->sockparam.from, &forkinfo->sockparam.fromlen ) ) == -1 )
			{
				perror( "recvfrom" );
				break;
			}
			if( option->plane_recv_callback != NULL )
			{
				(void) getnameinfo( (struct sockaddr *) &forkinfo->sockparam.from, forkinfo->sockparam.fromlen, 
							hbuf, sizeof( hbuf ), sbuf, sizeof( sbuf ), NI_NUMERICHOST | NI_NUMERICSERV );
#ifdef __GDT_DEBUG__
				//printf( "recvfrom:%s:%s,len:%zu\n", hbuf, sbuf, len );
#endif
				memcpy(forkinfo->hbuf, hbuf, NI_MAXHOST);
				memcpy(forkinfo->sbuf, sbuf, NI_MAXSERV);
				rinfo->tinfo			= forkinfo;
				rinfo->recvbuf_munit	= forkinfo->recvbuf_munit;
				rinfo->recvlen			= len;
				rinfo->recvfrom			= forkinfo->sockparam.acc;
				pfret = gdt_pre_packetfilter( option, rinfo, &forkinfo->sockparam, forkinfo->recvmsg_munit );
				if( pfret == -1 ){
					break;
				}
				else if( pfret == 1 ){
					option->plane_recv_callback( (void*)rinfo );
				}
			}
		}
		if( option->close_callback != NULL )
		{
			option->close_callback( (void*)forkinfo );
		}
	}
}
#endif

void gdt_thread_server( GDT_SOCKET_OPTION *option )
{
	int i;
#ifdef __WINDOWS__
	HANDLE thread_id;
#else
	pthread_t thread_id;
#endif
	struct GDT_SERVER_CONNECTION_INFO *tinfo;
#ifdef __GDT_DEBUG__
	printf( "gdt_thread_server: soc=%d\n", option->sockid );
#endif
	do{
		if( option->socket_type == SOKET_TYPE_SERVER_UDP )
		{
			if( option->maxconnection > 1 ){
				printf( "change udp maxconnection %zd -> 1\n", option->maxconnection );
				option->maxconnection = 1;
			}
		}
#ifdef __WINDOWS__
		option->accept_lock = CreateMutex( NULL, false , NULL );
#else
		if( pthread_mutex_init(&option->accept_lock, NULL) != 0 ){
			printf( "pthread_mutex_init : error \n" );
			break;
		}
#endif
		option->connection_munit = gdt_create_munit( option->memory_pool,sizeof( struct GDT_SERVER_CONNECTION_INFO ) * ( option->maxconnection ), MEMORY_TYPE_DEFAULT );
		for( i = 0; i < option->maxconnection; i++ )
		{
			tinfo = (struct GDT_SERVER_CONNECTION_INFO*)gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( struct GDT_SERVER_CONNECTION_INFO ), i );
#ifdef __WINDOWS__
#else
			tinfo->parentid			= pthread_self();
#endif
			tinfo->index			= i;
			tinfo->recvbuf_munit	= gdt_create_munit( option->memory_pool, sizeof( char ) * ( option->recvbuffer_size ), MEMORY_TYPE_DEFAULT );
			tinfo->recvinfo_munit	= gdt_create_munit( option->memory_pool,sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
			tinfo->recvmsg_munit	= gdt_create_munit( option->memory_pool, sizeof( char) * ( option->msgbuffer_size ), MEMORY_TYPE_DEFAULT );
			tinfo->user_data_munit		= -1;
			tinfo->gdt_socket_option			= option;
			gdt_init_socket_param( &tinfo->sockparam );
			if( option->socket_type == SOKET_TYPE_SERVER_TCP )
			{
#ifdef __WINDOWS__
				thread_id = (HANDLE)_beginthreadex( NULL, 0, gdt_accept_thread, (void*) tinfo, 0, NULL );
				if( thread_id == NULL )
#else
				if( pthread_create( &thread_id, NULL, gdt_accept_thread, (void*) tinfo ) != 0 )
#endif
				{
					perror( "pthread_create" );
				}
				else{
					tinfo->currentid = thread_id;
				}
			}
			else if( option->socket_type == SOKET_TYPE_SERVER_UDP )
			{
#ifdef __WINDOWS__
				thread_id = (HANDLE)_beginthreadex( NULL, 0, gdt_recvfrom_loop_thread, (void*) tinfo, 0, NULL );
				if( thread_id == NULL )
#else
				if( pthread_create( &thread_id, NULL, gdt_recvfrom_loop_thread, (void*) tinfo ) != 0 )
#endif
				{
					perror( "pthread_create" );
				}
				else{
					tinfo->currentid = thread_id;
				}
			}
		}
	}while( false );
}

#ifdef __WINDOWS__
unsigned __stdcall gdt_accept_thread( void* arg )
#else
void* gdt_accept_thread( void* arg )
#endif
{
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct sockaddr_storage from;
	socklen_t len;
	GDT_SERVER_CONNECTION_INFO *tinfo = ( GDT_SERVER_CONNECTION_INFO * )arg;
	GDT_SOCKET_OPTION *option = tinfo->gdt_socket_option;
	int accstatus = 0;
	GDT_SOCKET_ID sock;
	int width;
	struct timeval timeout;
	fd_set mask, read_mask;
#ifdef __WINDOWS__
#else
	uint64_t mythread_id = (uint64_t)( pthread_self() );
	pthread_detach( pthread_self() );
#endif
	memset(hbuf, 0, sizeof(hbuf));
	memset(sbuf, 0, sizeof(sbuf));
	for(;;)
	{
#ifdef __WINDOWS__
		WaitForSingleObject( option->accept_lock, INFINITE );
		option->t_lock_id = 0;
#else
		(void) pthread_mutex_lock( &option->accept_lock );
		option->t_lock_id = mythread_id;
#endif
		accstatus = 0;
		while( accstatus == 0 )
		{
			FD_ZERO( &mask );
			FD_SET( option->sockid, &mask );
			width = (int)(option->sockid + 1);
			if( option->sockid6 != -1 ){
				FD_SET( option->sockid6, &mask );
				width = (int)(option->sockid6 + 1);
			}
			read_mask = mask;
			timeout.tv_sec = option->s_sec;
			timeout.tv_usec = 0;
			switch( select ( width, ( fd_set * ) &read_mask, NULL, NULL, &timeout ) )
			{
				case -1:
					perror("select");
					break;
				case 0:
					break;
				default:
					sock = -1;
					if( FD_ISSET( option->sockid, &read_mask ) )
					{
						sock = option->sockid;
					}
					if( option->sockid6 != -1 )
					{
						if( FD_ISSET( option->sockid6, &read_mask ) )
						{
							sock = option->sockid6;
						}
					}
					if( sock != -1 )
					{
						len = (socklen_t) sizeof( from );
						if( ( tinfo->sockparam.acc = accept( sock, (struct sockaddr * ) &from, &len ) ) == -1 )
						{
							printf("errno : %d\n" ,errno );
							accstatus = -1;
						}
						else{
							accstatus = 1;
						}
					}
					break;
			}
		}
		
		if( tinfo->sockparam.acc == -1 )
		{
			if( errno != EINTR )
			{
				perror( "accept" );
			}
#ifdef __WINDOWS__
			ReleaseMutex( option->accept_lock );
#else
			(void) pthread_mutex_unlock( &option->accept_lock );
#endif
			option->t_lock_id = -1;
		}
		else{
#ifdef __WINDOWS__
			ReleaseMutex( option->accept_lock );
#else
			(void)getnameinfo((struct sockaddr*) &from, len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
#ifdef __GDT_DEBUG__
			//printf( "accept:%s:%s\n", hbuf, sbuf );
#endif
			(void) pthread_mutex_unlock( &option->accept_lock );
#endif
			option->t_lock_id = -1;
			gdt_initialize_connection_info( option, tinfo );
			memcpy( tinfo->hbuf, hbuf, NI_MAXHOST );
			memcpy( tinfo->sbuf, sbuf, NI_MAXSERV );
			if( option->connection_start_callback != NULL )
			{
				option->connection_start_callback( (void *)tinfo );
			}
			gdt_recv_loop_thread( tinfo );
			gdt_close_socket( &tinfo->sockparam.acc, NULL );
			gdt_free_sockparam( option, &tinfo->sockparam );
		}
	}
	gdt_free_sockparam( option, &tinfo->sockparam );
#ifdef __WINDOWS__
	return 0;
#else
	pthread_exit( (void *)0 );
	return ( (void *)0 );
#endif
}

void gdt_recv_loop_thread( struct GDT_SERVER_CONNECTION_INFO *tinfo )
{
	char *pbuf;
	int pfret;
	ssize_t len;
	GDT_SOCKET_OPTION *option = tinfo->gdt_socket_option;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	struct GDT_RECV_INFO* rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, tinfo->recvinfo_munit );
	for(;;)
	{
		rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, tinfo->recvinfo_munit );
		pbuf = (char*)gdt_upointer( option->memory_pool, tinfo->recvbuf_munit );
		//memset( pbuf, 0, buffer_size );
		*(pbuf) = '\0';
		if( option->user_recv_function != NULL ){
			if( -1 == ( len = option->user_recv_function( tinfo, tinfo->sockparam.acc, pbuf, buffer_size, 0 ) ) ){
				perror( "recv" );
				break;
			}
		}
		else{
			if( ( len = gdt_recv_with_timeout( option->recvtimeoutmode, option->t_sec, tinfo->sockparam.acc, pbuf, buffer_size, 0 ) ) == -1 ){
				perror( "recv" );
				break;
			}
		}
		if( len == 0 ){
			break;
		}
		if( option->plane_recv_callback != NULL )
		{
			rinfo->tinfo = tinfo;
			rinfo->recvbuf_munit = tinfo->recvbuf_munit;
			rinfo->recvlen = len;
			rinfo->recvfrom = tinfo->sockparam.acc;
			pfret = gdt_pre_packetfilter( option, rinfo, &tinfo->sockparam, tinfo->recvmsg_munit );
			if( pfret == -1 ){
				break;
			}
			else if( pfret == 1 ){
				option->plane_recv_callback( (void*)rinfo );
				if( tinfo->sockparam.c_status == PROTOCOL_STATUS_DEFAULT )
				{
					break;
				}
			}
		}
	}
	if( option->close_callback != NULL )
	{
		option->close_callback( (void*)tinfo );
	}
}

#ifdef __WINDOWS__
unsigned __stdcall gdt_recvfrom_loop_thread( void* arg )
#else
void* gdt_recvfrom_loop_thread( void* arg )
#endif
{
	int pfret;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	char *pbuf;
	ssize_t len;
	GDT_SERVER_CONNECTION_INFO *tinfo = ( GDT_SERVER_CONNECTION_INFO * )arg;
	GDT_SOCKET_OPTION *option = tinfo->gdt_socket_option;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	struct GDT_RECV_INFO* rinfo;
#ifdef __WINDOWS__
#else
	pthread_detach( pthread_self() );
#endif
	if( tinfo->recvbuf_munit < 0 ){
		tinfo->recvbuf_munit	= gdt_create_munit( option->memory_pool,buffer_size, MEMORY_TYPE_DEFAULT );
	}
	if( tinfo->recvmsg_munit < 0 ){
		tinfo->recvmsg_munit	= gdt_create_munit( option->memory_pool,buffer_size, MEMORY_TYPE_DEFAULT );
	}
	if( tinfo->recvinfo_munit < 0 )
	{
		tinfo->recvinfo_munit = gdt_create_munit( option->memory_pool,sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
	}
	rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, tinfo->recvinfo_munit );
	pbuf = (char*)gdt_upointer( option->memory_pool,tinfo->recvbuf_munit );
	tinfo->sockparam.acc = option->sockid;
	for(;;)
	{
		//memset( pbuf, 0, buffer_size );
		*(pbuf) = '\0';
		tinfo->sockparam.fromlen = sizeof(tinfo->sockparam.from);
		if( ( len = recvfrom( option->sockid, pbuf, buffer_size, 0, (struct sockaddr *)&tinfo->sockparam.from, &tinfo->sockparam.fromlen ) ) == -1 ){
			perror("recvfrom");
			break;
		}
		if( option->plane_recv_callback != NULL )
		{
			(void)getnameinfo((struct sockaddr *) &tinfo->sockparam.from, tinfo->sockparam.fromlen,
				hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
#ifdef __GDT_DEBUG__
			//printf("recvfrom:%s:%s,len:%zu\n", hbuf, sbuf, len);
#endif
			memcpy(tinfo->hbuf, hbuf, NI_MAXHOST);
			memcpy(tinfo->sbuf, sbuf, NI_MAXSERV);
			rinfo->tinfo			= tinfo;
			rinfo->recvbuf_munit	= tinfo->recvbuf_munit;
			rinfo->recvlen			= len;
			rinfo->recvfrom		= tinfo->sockparam.acc;
			pfret = gdt_pre_packetfilter( option, rinfo, &tinfo->sockparam, tinfo->recvmsg_munit );
			if( pfret == -1 ){
				break;
			}
			else if( pfret == 1 ){
				option->plane_recv_callback( (void*)rinfo );
			}
		}
	}
	if( option->close_callback != NULL )
	{
		option->close_callback( (void*)tinfo );
	}
	gdt_free_sockparam( option, &tinfo->sockparam );
#ifdef __WINDOWS__
	return 0;
#else
	pthread_exit( (void *)0 );
	return ( (void *)0 );
#endif
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

/*
 * parse recv packet
 * return int ( 0:continue, 1:call plane_recv_callback )
 */
int gdt_pre_packetfilter( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam, int32_t recvmsg_munit )
{
	int ret = 0;
	char *pbuf;
	char *msgpbuf;
	ssize_t _tmpmsglen;
	do{
		pbuf = (char*)gdt_upointer( option->memory_pool, rinfo->recvbuf_munit );
		switch( gdt_socket_phase( option, rinfo, psockparam, recvmsg_munit ) )
		{
			case SOCK_PHASE_HANDSHAKE_WEBSOCKET:
				break;
			case SOCK_PHASE_MSG_WEBSOCKET:
				if( rinfo->recvlen > 0 )
				{
					_tmpmsglen = gdt_parse_websocket_binary( option, psockparam, (uint8_t*)pbuf, rinfo->recvlen, recvmsg_munit );
					msgpbuf = (char*)gdt_upointer( option->memory_pool, recvmsg_munit );
					if( _tmpmsglen < 0 )
					{
						printf( "recv:EOF\n" );
						ret = -1; // EOF
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
				}
				break;
			case SOCK_PHASE_MSG_SOCKET:
				if( rinfo->recvlen > 0 )
				{
					if( option->protocol == PROTOCOL_SIMPLE )
					{
						_tmpmsglen = gdt_parse_socket_binary( option, psockparam, (uint8_t*)pbuf, rinfo->recvlen, recvmsg_munit );
						msgpbuf = (char*)gdt_upointer( option->memory_pool, recvmsg_munit );
						if( _tmpmsglen < 0 )
						{
							ret = -1; // EOF
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
				break;
			case SOCK_PHASE_REQUEST_HTTP:
				if( psockparam->opcode == 2 ){
					psockparam->opcode = 0;
					psockparam->tmpmsglen = 0;
					msgpbuf = (char*)gdt_upointer( option->memory_pool, recvmsg_munit );
					rinfo->recvlen = 0;
					if( gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) > 0 ){
						rinfo->recvlen = atoi( (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
					}
					msgpbuf[rinfo->recvlen] = '\0';
					rinfo->recvbuf_munit = recvmsg_munit;
					ret = 1;
				}
				break;
			default:
				ret = -1;
				break;
		}
	}while( false );
	return ret;
}

/*
 * 独自ハンドシェイク( websocketの初期接続ハンドシェイクと普通のsocketのハンドシェイクが同じ処理でできるようにハンドリング )
 * @param *option : GDT_SOCKET_OPTION構造体のポインタ
 * @param *rinfo : GDT_RECV_INFO構造体のポインタ
 * @param *psockparam : SocketParam構造体のポインタ
 * return 現在のフェーズ
 */
int32_t gdt_socket_phase( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam, int32_t recvmsg_munit )
{
	char *pbuf;
	int ret;
	int32_t phase = 0;
	pbuf = (char*)gdt_upointer( option->memory_pool,rinfo->recvbuf_munit );
	do{
		if( option->socket_type == SOKET_TYPE_SERVER_UDP )
		{
			psockparam->type		= SOCK_TYPE_NORMAL_UDP;
			psockparam->c_status	= PROTOCOL_STATUS_OTHER;
			phase					= SOCK_PHASE_MSG_SOCKET;
			break;
		}
		switch( psockparam->c_status )
		{
			case PROTOCOL_STATUS_DEFAULT:
				pbuf[rinfo->recvlen] = '\0';
				ret = gdt_parse_http( option, psockparam, pbuf, recvmsg_munit );
				if( ret == -1 )
				{
					phase = -1;
				}else if( ret == 0 ){
					psockparam->type = SOCK_TYPE_NORMAL;
					psockparam->c_status = PROTOCOL_STATUS_OTHER;
					phase = SOCK_PHASE_MSG_SOCKET;
				}
				else{
					if( ( ret = gdt_send_handshake_param( rinfo->recvfrom, option, psockparam ) ) == -1 )
					{
						psockparam->c_status = PROTOCOL_STATUS_HTTP;
						psockparam->type = SOCK_TYPE_HTTP;
						phase = SOCK_PHASE_REQUEST_HTTP;
					}
					else{
						psockparam->c_status = PROTOCOL_STATUS_WEBSOCKET_SENDHANDSHAKE;
						psockparam->type = SOCK_TYPE_WEBSOCKET;
						phase = SOCK_PHASE_HANDSHAKE_WEBSOCKET;
					}
				}
				break;
			case PROTOCOL_STATUS_WEBSOCKET_SENDHANDSHAKE:
				ret = gdt_parse_http( option, psockparam, pbuf, recvmsg_munit );
				if( ret == -1 )
				{
					phase = -1;
				}else if( ret == 0 ){
					psockparam->c_status = PROTOCOL_STATUS_WEBSOCKET_RECVHANDSHAKEOK;
					phase = SOCK_PHASE_MSG_WEBSOCKET;
				}
				else{
					if( ( ret = gdt_send_handshake_param( rinfo->recvfrom, option, psockparam ) ) == -1 ){
						gdt_free_sockparam( option, psockparam );
						perror( "gdt_send_handshake_param" );
					}
					psockparam->c_status = PROTOCOL_STATUS_WEBSOCKET_SENDHANDSHAKE;
					psockparam->type = SOCK_TYPE_WEBSOCKET;
					phase = SOCK_PHASE_HANDSHAKE_WEBSOCKET;
				}
				break;
			case PROTOCOL_STATUS_WEBSOCKET_RECVHANDSHAKEOK:
				ret = gdt_parse_http( option, psockparam, pbuf, recvmsg_munit );
				if( ret == -1 )
				{
					phase = -1;
				}else if( ret == 0 ){
					phase = SOCK_PHASE_MSG_WEBSOCKET;
				}
				else{
					if( ( ret = gdt_send_handshake_param( rinfo->recvfrom, option, psockparam ) ) == -1 ){
						gdt_free_sockparam( option, psockparam );
						perror( "gdt_send_handshake_param" );
					}
					psockparam->c_status = PROTOCOL_STATUS_WEBSOCKET_SENDHANDSHAKE;
					psockparam->type = SOCK_TYPE_WEBSOCKET;
					phase = SOCK_PHASE_HANDSHAKE_WEBSOCKET;
				}
				break;
			case PROTOCOL_STATUS_HTTP:
				gdt_parse_http_header( option, psockparam, pbuf, recvmsg_munit );
				phase = SOCK_PHASE_REQUEST_HTTP;
				break;
			case PROTOCOL_STATUS_OTHER:
				phase = SOCK_PHASE_MSG_SOCKET;
				break;
		}
	}while( false );
	psockparam->phase = phase;
	return phase;
}

/*
 * 受信したデータを取り出す( プロトコルの解析 )
 */
ssize_t gdt_parse_socket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit )
{
	int i, startpos;
	int64_t cnt = 0;
	uint64_t tmppayloadlen = 0;
	char* msg = (char*)gdt_upointer( option->memory_pool,basebuf_munit );
	ssize_t retsize = -1;
	do{
		if( psockparam->fin == 0 || ( psockparam->fin == 1 && psockparam->tmpmsglen == 0 ) ){
			if( psockparam->fin == 0 ){
				cnt = psockparam->tmpmsglen;
			}
			else{
				memset( msg, 0, gdt_usize( option->memory_pool, basebuf_munit ) );
				psockparam->payloadlen = 0;
			}
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
			psockparam->payloadlen += tmppayloadlen;
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
			startpos = 0;
			cnt = psockparam->tmpmsglen;
		}
		for( i = startpos; i < size; i++ )
		{
			msg[cnt++] = u8buf[i];
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
			msg[psockparam->payloadlen] = '\0';
			retsize = psockparam->payloadlen;
		}
		else{
			msg[psockparam->tmpmsglen] = '\0';
			retsize = psockparam->tmpmsglen;
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
		if( size < 125 ){
			headersize = 6;
			head2 = 0x00 | (uint8_t)size;
		}
		else if( size > 126 && size <= 65536 ){
			// MAX 64KByte
			headersize = 8;
			head2 = 0x00 | 126;
		}
		else{
			if( size >= SIZE_MBYTE * 128 )
			{
				printf( "[gdt_send_msg]size over: %zd byte\n", size );
				break;
			}
			headersize = 14;
			head2 = 0x00 | 127;
		}
		if( psockparam->buf_munit < 0 || gdt_usize( option->memory_pool, psockparam->buf_munit ) <= size + headersize )
		{
			if( psockparam->buf_munit >= 0 ){
				gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
			}
			if( ( psockparam->buf_munit = gdt_create_munit( option->memory_pool, sizeof( uint8_t ) * GDT_ALIGNUP( size + headersize, option->msgbuffer_size ), MEMORY_TYPE_DEFAULT ) ) == -1 )
			{
				printf( "[gdt_send_msg]size over: %zd byte\n", size );
				break;
			}
		}
		sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
		memset( sendbin, 0, gdt_usize( option->memory_pool, psockparam->buf_munit ) );
		ptr = (uint8_t*) sendbin;
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
		len = gdt_send_all( psockparam->acc, (char*)sendbin, (size_t) ( ( sizeof( char ) * size ) + headersize ), 0 );
		if( size >= SIZE_KBYTE * 16 ){
#ifdef __GDT_DEBUG__
			printf( "free big buffer : %zd\n" , size );
#endif
			gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
		}
	}while( false );
	return len;
}

ssize_t gdt_send_udpmsg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type, struct sockaddr *pfrom, socklen_t fromlen  )
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
		if( size < 125 ){
			headersize = 6;
			head2 = 0x00 | (uint8_t)size;
		}
		else if( size > 126 && size <= 65536 ){
			// MAX 64KByte
			headersize = 8;
			head2 = 0x00 | 126;
		}
		else{
			if( size >= SIZE_MBYTE * 128 )
			{
				printf( "[gdt_send_udpmsg]size over: %zd byte\n", size );
				break;
			}
			headersize = 14;
			head2 = 0x00 | 127;
		}
		if( psockparam->buf_munit < 0 || gdt_usize( option->memory_pool, psockparam->buf_munit ) <= size + headersize )
		{
			if( psockparam->buf_munit >= 0 ){
				gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
			}
			if( ( psockparam->buf_munit = gdt_create_munit( option->memory_pool, sizeof( uint8_t ) * GDT_ALIGNUP( size + headersize, option->msgbuffer_size ), MEMORY_TYPE_DEFAULT ) ) == -1 )
			{
				printf( "[gdt_send_udpmsg]size over: %zd byte\n", size );
				break;
			}
		}
		sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
		memset( sendbin, 0, gdt_usize( option->memory_pool, psockparam->buf_munit ) );
		ptr = (uint8_t*) sendbin;
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
		MEMORY_PUSH_BIT32_B( option->memory_pool, ptr, payload_type );
		cptr = (char*)ptr;
		memcpy( cptr, msg, size );
		len = gdt_sendto_all( psockparam->acc, (char*)sendbin, (size_t) ( ( sizeof( char ) * size ) + headersize ), 0, pfrom, fromlen );
		if( size >= SIZE_KBYTE * 16 ){
#ifdef __GDT_DEBUG__
			printf( "free big buffer : %zd\n" , size );
#endif
			gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
		}
	}while( false );
	return len;
}

/*
 * websocket protocol
 *	+-+-+-+-+-------+-+-------------+-------------------------------+
 *	|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *	|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *	|N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *	| |1|2|3|       |K|             |                               |
 *	+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *	|     Extended payload length continued, if payload len == 127  |
 *	+ - - - - - - - - - - - - - - - +-------------------------------+
 *	|                               |Masking-key, if MASK set to 1  |
 *	+-------------------------------+-------------------------------+
 *	| Masking-key (continued)       |          Payload Data         |
 *	+-------------------------------- - - - - - - - - - - - - - - - +
 *	:                     Payload Data continued ...                :
 *	+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *	|                     Payload Data continued ...                |
 *	+---------------------------------------------------------------+
 */
ssize_t gdt_parse_websocket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit )
{
	int i, j, startpos;
	int cnt = 0;
	uint64_t tmppayloadlen = 0;
	char* msg = (char*)gdt_upointer( option->memory_pool,basebuf_munit );
	ssize_t retsize = -1;
	do{
		if( psockparam->fin == 0 || ( psockparam->fin == 1 && psockparam->tmpmsglen == 0 ) )
		{
			if( psockparam->fin == 0 ){
				cnt = psockparam->tmpmsglen;
			}
			else{
				memset( msg, 0, gdt_usize( option->memory_pool, basebuf_munit ) );
				psockparam->payloadlen = 0;
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
			if( psockparam->mask == 0 )
			{
				//printf("mask : 0 not support.\n");
				break;
			}
			if(psockparam->ckpayloadlen < 126 ){
				psockparam->maskindex = 2;
				tmppayloadlen = psockparam->ckpayloadlen;
			}
			else if(psockparam->ckpayloadlen < 127 ){
				psockparam->maskindex = 4;
				tmppayloadlen |= u8buf[2] << 8;
				tmppayloadlen |= u8buf[3] << 0;
			}
			else{
				psockparam->maskindex = 10;
				tmppayloadlen |= (uint64_t)u8buf[2] << 56;
				tmppayloadlen |= (uint64_t)u8buf[3] << 48;
				tmppayloadlen |= (uint64_t)u8buf[4] << 40;
				tmppayloadlen |= (uint64_t)u8buf[5] << 32;
				tmppayloadlen |= (uint64_t)u8buf[6] << 24;
				tmppayloadlen |= (uint64_t)u8buf[7] << 16;
				tmppayloadlen |= (uint64_t)u8buf[8] << 8;
				tmppayloadlen |= (uint64_t)u8buf[9] << 0;
			}
			
			psockparam->payloadlen += tmppayloadlen;
			
			if( psockparam->payloadlen >= gdt_usize( option->memory_pool, basebuf_munit ) ){
				printf( "payloadlen buffersize over[%"PRIu64"][%zd]\n", psockparam->payloadlen, gdt_usize( option->memory_pool, basebuf_munit ) );
				psockparam->payloadlen = -1;
				break;
			}
			psockparam->payloadmask = 0x00000000;
			psockparam->payloadmask |= u8buf[psockparam->maskindex] << 24;
			psockparam->payloadmask |= u8buf[psockparam->maskindex+1] << 16;
			psockparam->payloadmask |= u8buf[psockparam->maskindex+2] << 8;
			psockparam->payloadmask |= u8buf[psockparam->maskindex+3] << 0;
			startpos = psockparam->maskindex+4;
		}
		else{
			startpos = 0;
			cnt = psockparam->tmpmsglen;
		}
		for( i = startpos; i < size; i++ )
		{
			if( psockparam->tmpbitsift < 0 ){
				psockparam->tmpbitsift = 24;
			}
			for( j = (psockparam->tmpbitsift/8); j >= 0; j-- )
			{
				if( i >= size )
				{
					if( psockparam->fin == 0 || cnt + (3-j) >= psockparam->payloadlen )
					{
						psockparam->appdata32bit = psockparam->appdata32bit ^ psockparam->payloadmask;
						msg[cnt++] = ( psockparam->appdata32bit & 0xff000000 ) >> 24;
						msg[cnt++] = ( psockparam->appdata32bit & 0x00ff0000 ) >> 16;
						msg[cnt++] = ( psockparam->appdata32bit & 0x0000ff00 ) >> 8;
						msg[cnt++] = ( psockparam->appdata32bit & 0x000000ff ) >> 0;
						psockparam->appdata32bit = 0x00000000;
					}
					break;
				}
				psockparam->appdata32bit |= u8buf[i] << psockparam->tmpbitsift;
				psockparam->tmpbitsift -= 8;
				if( j > 0 ){ 
					i++;
				}
				else{
					psockparam->appdata32bit = psockparam->appdata32bit ^ psockparam->payloadmask;
					msg[cnt++] = ( psockparam->appdata32bit & 0xff000000 ) >> 24;
					msg[cnt++] = ( psockparam->appdata32bit & 0x00ff0000 ) >> 16;
					msg[cnt++] = ( psockparam->appdata32bit & 0x0000ff00 ) >> 8;
					msg[cnt++] = ( psockparam->appdata32bit & 0x000000ff ) >> 0;
					psockparam->appdata32bit = 0x00000000;
				}
			}
		}
		psockparam->tmpmsglen += ( cnt - psockparam->tmpmsglen );
#ifdef __GDT_DEBUG__
//		printf( "websocket protocol : fin[%d], rsv[%d], opcode[%d], mask[%d], payload[%llu],tmplen[%zd], pktsize[%zd], [%zd]\n"
//				, psockparam->fin
//				, psockparam->rsv
//				, psockparam->opcode
//				, psockparam->mask
//				, psockparam->payloadlen
//				, psockparam->tmpmsglen
//				, size
//				, gdt_usize( option->memory_pool, basebuf_munit )
//				);
#endif
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
			msg[psockparam->payloadlen] = '\0';
			retsize = psockparam->payloadlen;
		}
		else{
			msg[psockparam->tmpmsglen] = '\0';
			retsize = psockparam->tmpmsglen;
		}
	}while( false );
	return retsize;
}

ssize_t gdt_send_websocket_msg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size )
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
		if( size < 125 ){
			headersize = 2;
			head2 = 0x00 | (uint8_t)size;
		}
		else if( size > 126 && size <= 65536 ){
			headersize = 4;
			head2 = 0x00 | 126;
		}
		else{
			if( size >= SIZE_MBYTE * 128 )
			{
				printf( "[gdt_send_websocket_msg]size over: %zd byte\n", size );
				break;
			}
			headersize = 10;
			head2 = 0x00 | 127;
		}
		if( psockparam->buf_munit < 0 || gdt_usize( option->memory_pool, psockparam->buf_munit ) <= size + headersize )
		{
			if( psockparam->buf_munit >= 0 ){
				gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
			}
			if( ( psockparam->buf_munit = gdt_create_munit( option->memory_pool, sizeof( uint8_t ) * GDT_ALIGNUP( size + headersize, option->msgbuffer_size ), MEMORY_TYPE_DEFAULT ) ) == -1 )
			{
				printf( "[gdt_send_websocket_msg]size over: %zd byte\n", size );
				break;
			}
		}
		sendbin = gdt_upointer( option->memory_pool, psockparam->buf_munit );
		memset( sendbin, 0, gdt_usize( option->memory_pool, psockparam->buf_munit ) );
		ptr = (uint8_t*) sendbin;
		*(ptr++) |= head1;
		*(ptr++) |= head2;
		if( headersize == 4 ){
			*(ptr++) = size >> 8;
			*(ptr++) = size & 0x00ff;
		}
		else if( headersize == 10 ){
			*(ptr++) = ( size & 0xff00000000000000) >> 56;
			*(ptr++) = ( size & 0x00ff000000000000) >> 48;
			*(ptr++) = ( size & 0x0000ff0000000000) >> 40;
			*(ptr++) = ( size & 0x000000ff00000000) >> 32;
			*(ptr++) = ( size & 0x00000000ff000000) >> 24;
			*(ptr++) = ( size & 0x0000000000ff0000) >> 16;
			*(ptr++) = ( size & 0x000000000000ff00) >> 8;
			*(ptr++) = ( size & 0x00000000000000ff) >> 0;
		}
		cptr = (char*)ptr;
		memcpy( cptr, msg, size );
		len = gdt_send_all( psockparam->acc, (char*)sendbin, (size_t) ( ( sizeof( char ) * size ) + headersize ), 0 );
		if( size >= SIZE_KBYTE * 16 ){
#ifdef __GDT_DEBUG__
			printf( "free socket buffer : %zd\n" , size );
#endif
			gdt_free_memory_unit( option->memory_pool, &psockparam->buf_munit );
		}
	}while( false );
	return len;
}

/*
 * websocket handshake
 * @param socket
 * @param option
 * @param psockparam
 */
int gdt_send_handshake_param(GDT_SOCKET_ID socket, GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam )
{
	uint8_t sendbuffer[1024];
	uint8_t* pbuffer;
	char* pwskey = NULL;
	char* pprotocol = NULL;
	char *responseKey = NULL;
	char shaHash[21];
	int buffersize = 0;
	uint8_t length = 0;
	ssize_t sendlen = 0;
	const char* secret = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	if( gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_KEY" ) ){
		pwskey = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_KEY" ));
	}
	if( gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_PROTOCOL" ) ){
		pprotocol = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_PROTOCOL" ));
	}
	if( pwskey == NULL || pwskey[0] == '\0' )
	{
		return -1;
	}
	memset(shaHash, 0, sizeof(shaHash));
	memset( sendbuffer, 0 ,sizeof( sendbuffer ) );
	pbuffer = sendbuffer;
	length = strlen(pwskey) + strlen( secret );
	if( psockparam->wsockkey_munit < 0 || gdt_usize( option->memory_pool, psockparam->wsockkey_munit ) <= length )
	{
		if( psockparam->wsockkey_munit >= 0 ){
			gdt_free_memory_unit( option->memory_pool, &psockparam->wsockkey_munit );
		}
		if( ( psockparam->wsockkey_munit = gdt_create_munit( option->memory_pool, sizeof( char ) * GDT_ALIGNUP( length, 64 ), MEMORY_TYPE_DEFAULT ) ) == -1 )
		{
			printf( "[responseKey]size over: %d byte\n", length );
			return gdt_error("[responseKey]size over");
		}
	}
	responseKey = (char*)gdt_upointer( option->memory_pool, psockparam->wsockkey_munit );
	memset( responseKey, 0, gdt_usize( option->memory_pool, psockparam->wsockkey_munit ) );
	memcpy(responseKey, pwskey, length);
	memcpy(&(responseKey[strlen(pwskey)]), secret, strlen(secret));
	gdt_sha1( shaHash, responseKey, length );
	shaHash[20] = '\0';
	gdt_base64_encode(responseKey, gdt_usize( option->memory_pool, psockparam->wsockkey_munit ) , shaHash, sizeof( shaHash )-1 );
	if( pprotocol == NULL || !strcmp( pprotocol, "" ) )
	{
		buffersize = gdt_sprintf((char *)pbuffer,sizeof(sendbuffer),
			"HTTP/1.1 101 Switching Protocols\r\n"
			"%s%s\r\n"
			"%s%s\r\n"
			"Sec-WebSocket-Accept: %s\r\n\r\n"
			,"Upgrade: "
			,"websocket"
			,"Connection: "
			,"Upgrade"
			,responseKey
		);
	}
	else{
		buffersize = gdt_sprintf((char *)pbuffer, sizeof(sendbuffer),
			"HTTP/1.1 101 Switching Protocols\r\n"
			"%s%s\r\n"
			"%s%s\r\n"
			"Sec-WebSocket-Accept: %s\r\n"
			"Sec-WebSocket-Protocol: %s\r\n\r\n"
			,"Upgrade: "
			,"websocket"
			,"Connection: "
			,"Upgrade"
			,responseKey
			,"chat"
		);
	}
	if( option->user_send_function != NULL ){
		GDT_SERVER_CONNECTION_INFO connection;
		connection.gdt_socket_option = option;
		option->user_send_function(&connection, psockparam->acc, (char*)pbuffer, buffersize, 0);
	}
	else{
		sendlen = gdt_send_all( socket, (char*)pbuffer, buffersize, 0 );
	}
	if( sendlen == -1 )
	{
		return gdt_error("send handshake error");
	}
	if( sendlen != buffersize )
	{
		return gdt_error("send handshake buffer size error");
	}
	return 1;
}

void gdt_init_socket_param( GDT_SOCKPARAM *psockparam )
{
	psockparam->http_header_munit	= -1;
	psockparam->wsockkey_munit		= -1;
	psockparam->c_status			= PROTOCOL_STATUS_DEFAULT;
	psockparam->acc					= -1;
	psockparam->type				= 0;
	psockparam->phase				= 0;
	psockparam->ws_msg_mode			= WS_MODE_TEXT;//WS_MODE_BINARY; // ( 1: text, 2 : binary )
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

/*
 * parse http header
 */
int gdt_parse_http( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char* target, int32_t recvmsg_munit )
{
	int method;
	char* target_pt;
	char headername[256];
	char headerparam[2048];
	method = 0;
	do{
		target_pt = target;
		//memset( headername, 0, sizeof( headername ) );
		//memset( headerparam, 0, sizeof( headerparam ) );
		target_pt = gdt_readline( headername, sizeof(headername), target_pt, ' ' );
		if( strcmp( headername, "GET" ) == 0 ){
			method = HTTP_METHOD_GET;
		}
		else if( strcmp( headername, "POST" ) == 0 ){
			method = HTTP_METHOD_POST;
		}
		else if( strcmp( headername, "HEAD" ) == 0 ){
			method = HTTP_METHOD_HEAD;
		}
		if( method == 0 ){
			break;
		}
		psockparam->opcode = 0;

		const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;
		int i;
		if( psockparam->http_header_munit == -1 )
		{
			if( 0 >= ( psockparam->http_header_munit = gdt_create_hash( option->memory_pool, 16 ) ) ){
				break;
			}
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				gdt_add_hash_emptystring( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]), atoi((char*)(http_header_strings[i][2])) );
			}
		}
		else{
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				gdt_clear_hash_string( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]) );
			}
		}
		gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "HTTP_METHOD", headername );
		target_pt = gdt_readline( headerparam, sizeof(headerparam), target_pt, ' ' );
		if( strcmp( headerparam, "/" ) != 0 )
		{
			int32_t pos = gdt_find_char( headerparam, strlen( headerparam ), '?' );
			if( pos > 0 )
			{
				char params_buf[2048];
				size_t params_buf_size = sizeof(headerparam) - (pos+1);
				//memset( params_buf, 0, sizeof(params_buf));
				memcpy( params_buf, &headerparam[pos+1], params_buf_size );
				headerparam[pos] = '\0';
				gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "GET_PARAMS", params_buf );
			}
			else{
				if( 0 < ( pos = gdt_find_char( headerparam, strlen( headerparam ), '&' ) ) ){
					headerparam[pos] = '\0';
				}
			}
		}
		char request_path[2048];
		if( GDT_SYSTEM_ERROR == gdt_escape_directory_traversal( request_path, headerparam, sizeof(request_path) ) ){
			psockparam->type = SOCK_TYPE_HTTP;
			char* msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n400 Bad Request\r\n";
			if( option->user_send_function != NULL ){
				//option->user_send_function(&childinfo, psockparam->acc, msg, strlen(msg), 0);
				printf("call user_send_function\n");
			}
			else{
				gdt_send( option, psockparam, msg, gdt_strlen( msg ), 0 );
			}
			method = -1;
			break;
		}
		gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "REQUEST", request_path );
		target_pt = gdt_readline( headername, sizeof(headername), target_pt, '\0' );
		gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "HTTP_VERSION", headername );
		gdt_parse_http_header( option, psockparam, target_pt, recvmsg_munit );
	}while( false );
	return method;
}

int gdt_parse_http_header( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char* target, int32_t recvmsg_munit )
{
	char headername[256];
	char headerparam[2048];
	char* target_pt;
	const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;
	do{
		target_pt = target;
		if( *target_pt=='\0' ){
			break;
		}
		for(;;)
		{
			if( *target_pt=='\n' || *target_pt=='\r' ){
				if( *target_pt != '\0' )
				{
					psockparam->opcode = 1;
					psockparam->tmpmsglen = 0;
				}
			}
			if( psockparam->opcode == 1 ){
				if( gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) > 0 ){
					int contentlen = atoi( (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
					if( contentlen > 0 )
					{
						char* msgbuf = ( (char*)GDT_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
						while( *target_pt=='\n' || *target_pt=='\r' ){ target_pt++; }
						while( *target_pt != '\0' ){
							*(msgbuf++) = *(target_pt++);
							psockparam->tmpmsglen++;
							if( psockparam->tmpmsglen >= contentlen ){
								*msgbuf = '\0';
								psockparam->opcode = 2;
								break;
							}
						}
					}
					else{
						char* msgbuf = ( (char*)GDT_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
						*msgbuf = '\0';
						psockparam->opcode = 2;
					}
				}
				else{
					char* msgbuf = ( (char*)GDT_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
					*msgbuf = '\0';
					psockparam->opcode = 2;
				}
				break;
			}
			target_pt = gdt_readline( headername, sizeof(headername), target_pt, ' ' );
			target_pt = gdt_readline( headerparam, sizeof(headerparam), target_pt, '\0' );
			int i;
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				if( !strcmp( headername, http_header_strings[i][0] ) )
				{
					gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]), headerparam );
				}
			}
			if( (*target_pt) == '\0' ){
				break;
			}
		}
	}while( false );
	return 0;
}

void gdt_send_recv_client_term( GDT_SOCKET_OPTION *option )
{
	struct GDT_SERVER_CONNECTION_INFO childinfo;
	struct GDT_RECV_INFO* rinfo;
	char *pbuf;
	char *ibuf;
	char *msgpbuf;
	ssize_t _tmpmsglen;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	struct timeval timeout;
	int end, width;
	ssize_t len;
	fd_set mask, ready;
#ifdef __WINDOWS__
	HANDLE	wthread_id;
#else
	uint32_t inputbuf_munit;
	inputbuf_munit = gdt_create_munit( option->memory_pool, buffer_size, MEMORY_TYPE_DEFAULT );
	ibuf = (char*)gdt_upointer( option->memory_pool, inputbuf_munit );
#endif
#ifdef __GDT_DEBUG__
	printf("gdt_send_recv_client_term %d\n" , option->sockid);
#endif
	FD_ZERO( &mask );
#if defined(__WINDOWS__) || defined(__IOS__) || defined(__ANDROID__)
#else
	FD_SET( 0, &mask ); // stdin
#endif
	FD_SET( option->sockid, &mask );
	width = (int)(option->sockid + 1);
	childinfo.id = -1;
	childinfo.recvbuf_munit = gdt_create_munit( option->memory_pool, buffer_size, MEMORY_TYPE_DEFAULT );
	childinfo.recvinfo_munit = gdt_create_munit( option->memory_pool, sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
	childinfo.recvmsg_munit = gdt_create_munit( option->memory_pool, sizeof( char) * ( option->msgbuffer_size ), MEMORY_TYPE_DEFAULT );
	childinfo.gdt_socket_option			= option;
	gdt_init_socket_param( &childinfo.sockparam );
	rinfo = (struct GDT_RECV_INFO*)gdt_upointer( option->memory_pool, childinfo.recvinfo_munit );
	pbuf = (char*)gdt_upointer( option->memory_pool, childinfo.recvbuf_munit );
	childinfo.sockparam.acc	= option->sockid;
	if (option->socket_type == SOKET_TYPE_CLIENT_TCP){
		childinfo.sockparam.type = SOCK_TYPE_NORMAL;
	}
	else {
		childinfo.sockparam.type = SOCK_TYPE_NORMAL_UDP;
	}
#ifdef __WINDOWS__
	wthread_id = (HANDLE)_beginthreadex( NULL, 0, gdt_input_thread_client, (void*) &childinfo, 0, NULL );
	if( wthread_id == NULL ){
		perror("thread");
	}
#else
//	(void)system( "stty -echo raw" );
#endif
	// buffering OFF
#ifdef __WINDOWS__
	//setvbuf(stdin, NULL, _IONBF, 0);
	//setvbuf(stdout, NULL, _IONBF, 0);
#else
	(void)setbuf(stdin, NULL);
	(void)setbuf(stdout, NULL);
#endif
	if( option->connection_start_callback != NULL )
	{
		option->connection_start_callback( (void*)&childinfo );
	}
	for(end = 0;;)
	{
		ready = mask;
		timeout.tv_sec = option->s_sec;
		timeout.tv_usec = 0;
		switch( select( width, (fd_set*) &ready, NULL, NULL, &timeout ) )
		{
			case -1:
				if( errno != EINTR )
				{
					printf("errno : %d\n", errno);
					perror("select");
					end = 1;
				}
				break;
			case 0:
				// timeout
				break;
			default:
				if( FD_ISSET( option->sockid, &ready ) )
				{
					if( option->socket_type == SOKET_TYPE_CLIENT_TCP )
					{
						if( ( len = recv( option->sockid, pbuf, buffer_size, 0 ) ) == -1 ){
							perror( "recv" );
							end = 1;
							break;
						}
						if( len == 0 ){
							printf( "recv:EOF\n" );
							end = 1;
							break;
						}
						if( option->plane_recv_callback != NULL )
						{
							rinfo->tinfo			= &childinfo;
							rinfo->recvbuf_munit	= childinfo.recvbuf_munit;
							rinfo->recvlen			= len;
							rinfo->recvfrom			= option->sockid;
							if( option->protocol == PROTOCOL_SIMPLE )
							{
								_tmpmsglen = gdt_parse_socket_binary( option, &childinfo.sockparam, (uint8_t*)pbuf, rinfo->recvlen, childinfo.recvmsg_munit );
								msgpbuf = (char*)gdt_upointer( option->memory_pool, childinfo.recvmsg_munit );
								if( _tmpmsglen < 0 )
								{
									printf( "recv:EOF\n" );
									end = 1;
									break;
								}
								else{
									if( childinfo.sockparam.tmpmsglen == 0 )
									{
										rinfo->recvlen = _tmpmsglen;
										msgpbuf[rinfo->recvlen] = '\0';
										rinfo->recvbuf_munit = childinfo.recvmsg_munit;
										option->plane_recv_callback( (void*)rinfo );
									}
									else{
										printf("sock msg continue.\n");
									}
								}
							}else{
								option->plane_recv_callback( (void*)rinfo );
							}
						}
					}
					else if( option->socket_type == SOKET_TYPE_CLIENT_UDP )
					{
#ifdef __GDT_DEBUG__
						//printf("do not recv udp packet\n");
#if defined(__WINDOWS__)
						Sleep(100);
#else
						usleep(100);
#endif
#endif
					}
				}
#if defined(__WINDOWS__) || defined(__IOS__) || defined(__ANDROID__)
#else
				if( FD_ISSET( 0, &ready ) )
				{
					char* pstr = fgets( ibuf, buffer_size, stdin );
					if( feof( stdin ) || pstr == NULL ){
						end = 1;
						break;
					}
					if (option->user_send_function != NULL) {
						if ((len = option->user_send_function(&childinfo, childinfo.sockparam.acc, ibuf, strlen(ibuf), 0)) == -1)
						{
							perror("send");
							end = 1;
							break;
						}
					}
					else {
						if ((len = gdt_send(childinfo.gdt_socket_option, &childinfo.sockparam, ibuf, strlen(ibuf), 0x01)) == -1)
						{
							perror("send");
							end = 1;
							break;
						}
					}
				}
#endif
				break;
		}
		if( end ){
			break;
		}
	}
#ifdef __WINDOWS__
#else
//	(void)system( "stty echo cooked -istrip" );
#endif
}

#ifdef __WINDOWS__
unsigned __stdcall gdt_input_thread_client( void* arg )
#else
void* gdt_input_thread_client( void* arg )
#endif
{
	GDT_SERVER_CONNECTION_INFO *tinfo = ( GDT_SERVER_CONNECTION_INFO* )arg;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	char *ibuf;
	size_t buffer_size = sizeof( char ) * ( option->recvbuffer_size );
	ssize_t len;
	uint32_t inputbuf_munit;
	inputbuf_munit = gdt_create_munit( option->memory_pool, buffer_size, MEMORY_TYPE_DEFAULT );
	ibuf = (char*)gdt_upointer( option->memory_pool, inputbuf_munit );
#ifdef __WINDOWS__
#else
	pthread_detach( pthread_self() );
#endif
	for(;;)
	{
		char* pstr = fgets( ibuf, buffer_size, stdin );
		if( feof( stdin ) || pstr == NULL ){
			break;
		}
		if (option->user_send_function != NULL) {
			if ((len = option->user_send_function(tinfo, tinfo->sockparam.acc, ibuf, strlen(ibuf), 0)) == -1)
			{
				perror("send");
				break;
			}
		}
		else {
			if ((len = gdt_send(option, &tinfo->sockparam, ibuf, strlen(ibuf), 0x01)) == -1)
			{
				perror("send");
				break;
			}
		}
	}
#ifdef __WINDOWS__
	return 0;
#else
	pthread_exit( (void *)0 );
	return ( (void *)0 );
#endif
}

/*
 * recv onry client
 */
void gdt_thread_client( GDT_SOCKET_OPTION *option )
{
#ifdef __WINDOWS__
	HANDLE thread_id;
#else
	pthread_t thread_id;
#endif
	struct GDT_SERVER_CONNECTION_INFO *tinfo;
#ifdef __GDT_DEBUG__
	printf( "gdt_client_thread: soc=%d\n", option->sockid );
#endif
	do{
#ifdef __WINDOWS__
		option->accept_lock = CreateMutex( NULL, false , NULL );
#else
		if( pthread_mutex_init(&option->accept_lock, NULL) != 0 ){
			printf( "pthread_mutex_init : error \n" );
			break;
		}
#endif
		option->connection_munit = gdt_create_munit( option->memory_pool, sizeof( struct GDT_SERVER_CONNECTION_INFO ), MEMORY_TYPE_DEFAULT );
		tinfo = ( struct GDT_SERVER_CONNECTION_INFO *)gdt_upointer( option->memory_pool, option->connection_munit );
#ifdef __WINDOWS__
		tinfo->parentid			= 0;
#else
		tinfo->parentid			= pthread_self();
#endif
		tinfo->index			= 0;
		tinfo->recvbuf_munit	= gdt_create_munit( option->memory_pool, sizeof( char ) * ( option->recvbuffer_size ), MEMORY_TYPE_DEFAULT );
		tinfo->recvinfo_munit	= gdt_create_munit( option->memory_pool,sizeof( struct GDT_RECV_INFO ), MEMORY_TYPE_DEFAULT );
		tinfo->recvmsg_munit	= gdt_create_munit( option->memory_pool, sizeof( char) * ( option->msgbuffer_size ), MEMORY_TYPE_DEFAULT );
		tinfo->user_data_munit		= -1;
		tinfo->gdt_socket_option			= option;
		gdt_init_socket_param( &tinfo->sockparam );
		tinfo->sockparam.acc = option->sockid;
		tinfo->sockparam.type = SOCK_TYPE_NORMAL;
#ifdef __WINDOWS__
		thread_id = (HANDLE)_beginthreadex( NULL, 0, gdt_recv_thread_client, (void*) tinfo, 0, NULL );
		if( thread_id == NULL )
#else
		if( pthread_create( &thread_id, NULL, gdt_recv_thread_client, (void*) tinfo ) != 0 )
#endif
		{
			perror( "pthread_create" );
		}
		else{
#ifdef __GDT_DEBUG__
			printf( "pthread_create:create:thread_id=%d\n", (int) thread_id );
#endif
			tinfo->currentid = thread_id;
		}
		printf( "pthread_create : finish\n" );
	}while( false );
}

/*
 * 受信専用クライアントスレッド
 */
#ifdef __WINDOWS__
unsigned __stdcall gdt_recv_thread_client( void* arg )
#else
void* gdt_recv_thread_client( void* arg )
#endif
{
	struct GDT_SERVER_CONNECTION_INFO *tinfo = ( struct GDT_SERVER_CONNECTION_INFO * )arg;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	int is_continue = 0;
	for(;;)
	{
		// retry
		if( is_continue )
		{
			if( ( option->sockid = gdt_client_socket( option ) ) <= 0 )
			{
				printf( "gdt_client_socket error: port=%s, host=%s\n", (char *)gdt_upointer( option->memory_pool, option->port_num_munit ), (char *)gdt_upointer( option->memory_pool, option->host_name_munit ) );
				// 失敗したら再接続。少し間を置く( 何回リトライしてもだめだったらアラート出すのもあり )
#ifdef __WINDOWS__
				Sleep( 250 ); // 250ms
#else
				usleep( 2500000 ); // 250ms
#endif

				continue;
			}
		}
		gdt_initialize_connection_info( option, tinfo );
		tinfo->sockparam.acc = option->sockid;
		tinfo->sockparam.type = SOCK_TYPE_NORMAL;
		if( option->connection_start_callback != NULL )
		{
			option->connection_start_callback( (void*)tinfo );
		}
		gdt_recv_loop_thread( tinfo );
		gdt_close_socket(&tinfo->sockparam.acc, NULL);
		tinfo->sockparam.acc = -1;
		option->sockid = -1;
		gdt_free_sockparam(option, &tinfo->sockparam );
		is_continue = 1;
#ifdef __GDT_DEBUG__
		printf("conection retry\n");
#endif
	}
	gdt_free_sockparam( option, &tinfo->sockparam );
#ifdef __WINDOWS__
	return 0;
#else
	pthread_exit( (void *)0 );
	return ( (void *)0 );
#endif
}

