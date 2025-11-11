/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_socket.h"
int on_connect(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info );
int on_close(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info);

int main( int argc, char *argv[], char *envp[] )
{
	QS_SOCKET_OPTION* tcp_client = qs_create_tcp_client("localhost", "52001");
	set_on_connect_event(tcp_client,on_connect);
	set_on_payload_recv_event(tcp_client,on_recv);
	set_on_close_event(tcp_client,on_close);
	qs_socket(tcp_client);
	
	QS_SOCKET_OPTION*  udp_client = qs_create_udp_client("localhost", "52001");
	set_on_payload_recv_event(udp_client, on_udp_client_payload_recv);
	qs_socket(udp_client);
	
	while(1){
		qs_client_update(tcp_client);
		qs_client_update(udp_client);
		qs_sleep(100000);
		qs_client_send_message(0x01,"test1",4,tcp_client);
		qs_sleep(100000);
		qs_client_send_message(0x02,"test2",4,udp_client);
	}
	qs_free_socket(tcp_client);
	qs_free(tcp_client->memory_pool);
	qs_free_socket(udp_client);
	qs_free(udp_client->memory_pool);
	return 0;
}

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_connect\n");
	return 0;
}

int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info )
{
	printf("on_recv(payload_type:%d,payload_len:%d)%s\n",payload_type,(int)payload_len,(char*)payload);
	return 0;
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_close\n");
	return 0;
}

int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info)
{
	char *pbuf = (char*)payload;
	pbuf[payload_len] = '\0';
	printf("recv udp(payload_type:%d,payload_len:%d) %s\n", (int)payload_type, (int)payload_len, pbuf);
	return 0;
}
