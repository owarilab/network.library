/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_socket.h"

int on_connect(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_recv(uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info);
int on_close(QS_SERVER_CONNECTION_INFO* connection);

int main( int argc, char *argv[], char *envp[] )
{
	QS_SOCKET_OPTION* tcp_client = qs_create_tcp_client_plain("localhost", "52001");
	set_on_connect_event(tcp_client,on_connect);
	set_on_plain_recv_event(tcp_client, on_recv );
	set_on_close_event(tcp_client,on_close);
	qs_socket(tcp_client);
	//qs_wait_client_socket(tcp_client);
	
	int timer = time(0);
	while(1){
		qs_client_update(tcp_client);
		qs_sleep(1000);
		if(time(0) - timer > 2){
			printf("timeout\n");
			qs_client_send("test",4,tcp_client);
			timer = time(0);
		}
	}
	qs_free_socket(tcp_client);
	qs_free(tcp_client->memory_pool);
	return 0;
}

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_connect\n");
	return 0;
}

int32_t on_recv(uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info)
{
	printf("on_recv\n");
	char* msg = (char*)payload;
	msg+=6; // skip header bytes
	printf("msg len:%d , msg:%s\n",(int)payload_len,msg);
	return 0;
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_close\n");
	return 0;
}
