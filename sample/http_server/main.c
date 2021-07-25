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

	//dummy http response
	{
		//gdt_send(option, &tinfo->sockparam, HTTP_OK, gdt_strlen(HTTP_OK), 0);
		//gdt_disconnect(&tinfo->sockparam);
		//return ((void *)NULL);
	}

	GDT_MEMORY_POOL * temporary_memory = ( GDT_MEMORY_POOL* )GDT_POINTER( option->memory_pool, memid_temprorary_pool );
	gdt_memory_clean( temporary_memory );

	printf("headers\n");
	gdt_hash_dump(option->memory_pool, tinfo->sockparam.http_header_munit,0);

	char *method = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "HTTP_METHOD" ));
	char *request = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "REQUEST" ));
	char *get_params = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "GET_PARAMS" ));
	char *content_type = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "CONTENT_TYPE" ));
	char *cache_control = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "CACHE_CONTROL" ));
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

	// request path
	char request_path[MAXPATHLEN];
	gdt_http_document_path(request_path,MAXPATHLEN,"./www","index.html",request);

	// extention
	char extension[32];
	gdt_get_extension( extension, sizeof(extension), request_path );

	printf("request path : %s , extension : %s\n",request_path,extension);

	int32_t http_status_code = 500;
	do{
		GDT_FILE_INFO info;
		if( GDT_SYSTEM_ERROR == gdt_fget_info( request_path, &info ) ){
			http_status_code = 404;
			break;
		}

		//response
		int32_t memid_response_buffer = gdt_create_memory_block(temporary_memory,info.size+SIZE_KBYTE*4);
		if( -1 == memid_response_buffer ){
			break;
		}
		char* response_buffer = (char*)GDT_POINTER(temporary_memory,memid_response_buffer);
		size_t response_buffer_size = gdt_usize(temporary_memory,memid_response_buffer);
		memset(response_buffer, 0, response_buffer_size); // memory over ( windows only )
		size_t response_len = 0;
		int is_binary = 0;

		if( !strcmp(extension,"html"))
		{
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer,response_buffer_size,http_status_code,"text/html",info.size);
			response_len = gdt_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &info);
		}
		else if( !strcmp(extension,"css"))
		{
			if( !strcmp(cache_control,"max-age=0") ){
				char *modified_since = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "IF_MODIFIED_SINCE" ));
				if( strcmp("",modified_since)){
					http_status_code = 304;
					break;
				}
			}
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer,response_buffer_size,http_status_code,"text/css",info.size);
			response_len = gdt_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &info);
		}
		else if( !strcmp(extension,"js"))
		{
			if( !strcmp(cache_control,"max-age=0") ){
				char *modified_since = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "IF_MODIFIED_SINCE" ));
				if( strcmp("",modified_since)){
					http_status_code = 304;
					break;
				}
			}
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer,response_buffer_size,http_status_code,"text/javascript",info.size);
			response_len = gdt_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &info);
		}
		else if (!strcmp(extension, "json"))
		{
			if (!strcmp(cache_control, "max-age=0")) {
				char *modified_since = (char*)GDT_POINTER(option->memory_pool, gdt_get_hash(option->memory_pool, tinfo->sockparam.http_header_munit, "IF_MODIFIED_SINCE"));
				if (strcmp("", modified_since)) {
					http_status_code = 304;
					break;
				}
			}
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer, response_buffer_size, http_status_code, "application/json", info.size);
			response_len = gdt_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &info);
		}
		else if( !strcmp(extension,"ico"))
		{
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer,response_buffer_size,http_status_code,"image/x-icon",info.size);
			is_binary = 1;
		}
		else if (!strcmp(extension, "unityweb"))
		{
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer, response_buffer_size, 200, "application/octet-stream", info.size);
			is_binary = 1;
		}
		else if (!strcmp(extension, "png"))
		{
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer, response_buffer_size, 200, "image/png", info.size);
			is_binary = 1;
		}
		else if (!strcmp(extension, "mp3"))
		{
			http_status_code = 200;
			response_len = gdt_http_add_response_common(response_buffer, response_buffer_size, 200, "audio/mp3", info.size);
			is_binary = 1;
		}
		else{
			break;
		}
		if( !strcmp("HEAD",method) ){
			response_len = gdt_strlink( response_buffer, response_len, "\r\n", 2, response_buffer_size );
		}
		else{
			response_len = gdt_strlink( response_buffer, response_len, "\r\n", 2, response_buffer_size );
			char* pstart = response_buffer+response_len;
			size_t plen = response_buffer_size-response_len;
			if(is_binary==0){
				size_t readlen = gdt_fread_bin( request_path, pstart, plen );
				response_len+=readlen;
			}
			else{
				size_t readlen = gdt_fread_bin( request_path, pstart, plen );
				response_len+=readlen;
			}
		}
		response_buffer[response_len] = '\0';
		gdt_send( option, &tinfo->sockparam, response_buffer, response_len, 0 );
	}while(false);

	if( http_status_code != 200 ){
		if( http_status_code == 304 ){
			gdt_send( option, &tinfo->sockparam, HTTP_NOT_MODIFIED, gdt_strlen( HTTP_NOT_MODIFIED ), 0 );
		}
		else if( http_status_code == 404 ){
			gdt_send( option, &tinfo->sockparam, HTTP_NOT_FOUND, gdt_strlen( HTTP_NOT_FOUND ), 0 );
		}
		else{
			gdt_send( option, &tinfo->sockparam, HTTP_INTERNAL_SERVER_ERROR, gdt_strlen( HTTP_INTERNAL_SERVER_ERROR ), 0 );
		}
	}

	gdt_disconnect(&tinfo->sockparam);
	return ((void *)NULL);
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}
