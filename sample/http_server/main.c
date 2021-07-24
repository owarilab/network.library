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
#include "gdt_protocol.h"
#include "gdt_variable.h"

// api sample
// curl -X POST -H "Content-Type: application/json" -d '{"id":"id_12345678", "password":"test"}' http://localhost:8080/api/v1/login
// curl -X POST -d 'id=id_12345678&password=test' http://localhost:8080/api/v1/login

int32_t memid_temprorary_pool = -1;

int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
void* on_recv( void* args );
int on_close(GDT_SERVER_CONNECTION_INFO* connection);

int main( int argc, char *argv[], char *envp[] )
{
	gdt_set_defaultsignal();
	GDT_SOCKET_OPTION* option = gdt_create_tcp_server_plane(NULL, "8080");
	memid_temprorary_pool = gdt_create_mini_memory( option->memory_pool, SIZE_KBYTE * 256 );
	set_on_connect_event(option,on_connect);
	set_on_packet_recv_event(option,on_recv);
	set_on_close_event(option,on_close);
	gdt_socket(option);
	while(1){
		gdt_server_update(option);
		usleep(100);
	}
	gdt_free_socket(option);
	gdt_free(option->memory_pool);
	return 0;
}

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

void* on_recv( void* args )
{
	GDT_RECV_INFO *rinfo = (GDT_RECV_INFO *)args;
	GDT_SERVER_CONNECTION_INFO * tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	GDT_SOCKPARAM* psockparam = &tinfo->sockparam;
	switch( gdt_http_protocol_filter(rinfo) )
	{
		case -1:
			gdt_disconnect( psockparam );
			return ( (void *) NULL );
		case 0:
			return ( (void *) NULL );
		case 1:
			break;
	}

	GDT_MEMORY_POOL * temporary_memory = ( GDT_MEMORY_POOL* )GDT_POINTER( option->memory_pool, memid_temprorary_pool );
	gdt_memory_clean( temporary_memory );

	printf("headers\n");
	gdt_hash_dump(option->memory_pool, tinfo->sockparam.http_header_munit,0);

	char *method = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "HTTP_METHOD" ));
	char *request = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "REQUEST" ));
	char *get_params = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "GET_PARAMS" ));
	char *content_type = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "CONTENT_TYPE" ));
	printf("method : %s , request : %s\n",method,request);

	// get parameter
	int32_t memid_get_parameter_hash = -1;
	do{
		char* pparam = get_params;
		char param_name[2048];
		char param_value[2048];
		char param_urldecode[2048];
		if( -1 == ( memid_get_parameter_hash = gdt_create_hash( temporary_memory, 32 ) ) ){
			break;
		}
		for(;*pparam != '\0';){
			pparam = gdt_read_line_delimiter( param_name, sizeof(param_name), pparam, '=' );
			pparam = gdt_read_line_delimiter( param_value, sizeof(param_value), pparam, '&' );
			gdt_urldecode( param_urldecode, sizeof( param_urldecode ), param_value );
			gdt_add_hash_value( temporary_memory, memid_get_parameter_hash, param_name, param_urldecode,ELEMENT_LITERAL_STR );
		}
		printf("get params\n");
		gdt_hash_dump(temporary_memory, memid_get_parameter_hash,0);
	}while(false);

	// post body
	if( !strcmp("POST",method) ){
		char *body = (char*)gdt_upointer(option->memory_pool, rinfo->recvbuf_munit);
		printf("body : %s\n",body);
		int32_t memid_post_parameter_hash = -1;
		// json
		if( !strcmp("application/json",content_type)){
			do{
				GDT_NODE* hashroot = gdt_get_json_root(temporary_memory, gdt_json_decode_h(temporary_memory, body, 8, 128));
				if (NULL == hashroot || hashroot->id != ELEMENT_HASH) {
					break;
				}
				memid_post_parameter_hash = hashroot->element_munit;
				printf("json post params\n");
				gdt_hash_dump(temporary_memory, memid_post_parameter_hash,0);
			}while(false);
		}else{
			do{
				char* pparam = body;
				char param_name[2048];
				char param_value[2048];
				char param_urldecode[2048];
				if( -1 == ( memid_post_parameter_hash = gdt_create_hash( temporary_memory, 32 ) ) ){
					break;
				}
				for(;*pparam != '\0';){
					pparam = gdt_read_line_delimiter( param_name, sizeof(param_name), pparam, '=' );
					pparam = gdt_read_line_delimiter( param_value, sizeof(param_value), pparam, '&' );
					gdt_urldecode( param_urldecode, sizeof( param_urldecode ), param_value );
					gdt_add_hash_value( temporary_memory, memid_post_parameter_hash, param_name, param_urldecode,ELEMENT_LITERAL_STR );
				}
				printf("post params\n");
				gdt_hash_dump(temporary_memory, memid_post_parameter_hash,0);
			}while(false);
		}
	}

	// http response
	gdt_send(option, &tinfo->sockparam, HTTP_OK, gdt_strlen(HTTP_OK), 0);
	gdt_disconnect(&tinfo->sockparam);
	return ((void *)NULL);
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}
