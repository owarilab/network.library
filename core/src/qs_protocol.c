#include "qs_protocol.h"

uint8_t qs_get_protocol_header_size_byte(ssize_t payload_size)
{
	uint8_t size_byte = 0;
	if( payload_size <= 125 ){
		size_byte = (uint8_t)payload_size;
	}
	else if( payload_size >= 126 && payload_size <= 65536 ){
		size_byte = 126;
	}
	else{
		size_byte = 127;
	}
	return size_byte;
}

uint32_t qs_get_protocol_header_size(ssize_t payload_size)
{
	uint32_t headersize = 2;
	uint8_t header_size_byte = qs_get_protocol_header_size_byte(payload_size);
	if( header_size_byte == 126 ){
		headersize += 2;
	}
	else if( header_size_byte == 127 ){
		headersize += 8;
	}
	headersize += 4; // payload_type
	headersize += 4; // sequence num
	return headersize;
}

ssize_t qs_get_protocol_buffer_size(ssize_t payload_size)
{
	return qs_get_protocol_header_size(payload_size) + payload_size;	
}

uint32_t qs_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num)
{
	uint8_t* pbin = bin;
	int endian = qs_endian();
	uint32_t headersize = 2;
	// mode
	*(pbin++) = 0;
	uint8_t header_size_byte = qs_get_protocol_header_size_byte(payload_size);
	*(pbin++) = header_size_byte;
	// payload size
	if( header_size_byte == 126 ){
		headersize += 2;
		MEMORY_PUSH_BIT16_B2(endian,pbin,payload_size);
	}
	else if( header_size_byte == 127 ){
		headersize += 8;
		MEMORY_PUSH_BIT64_B2(endian,pbin,payload_size);
	}
	// payload type
	headersize += 4;
	MEMORY_PUSH_BIT32_B2( endian, pbin, payload_type );
	// sequence num
	headersize += 4;
	MEMORY_PUSH_BIT32_B2( endian, pbin, seq_num );
	return headersize;
}

ssize_t qs_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num)
{
	uint8_t* pbin = bin;
	uint32_t headersize = qs_make_protocol_header(bin,payload_size,payload_type,seq_num);
	pbin += headersize;
	memcpy(pbin,payload,payload_size);
	return headersize + payload_size;
}

int qs_http_protocol_filter(QS_RECV_INFO* rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	if( psockparam->phase == QS_HTTP_SOCK_PHASE_RECV_CONTINUE )
	{
		int ret = qs_http_parser( rinfo );
		if( ret == -1 ){
			qs_disconnect( psockparam );
			return -1;
		}else if( ret == 0 ){
			qs_disconnect( psockparam );
			return -1;
		}
		else{
			psockparam->phase = QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER;
			psockparam->c_status = PROTOCOL_STATUS_HTTP;
			psockparam->type = SOCK_TYPE_NORMAL_TCP;
		}
	}
	else if( psockparam->phase == QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER ){
		qs_http_parse_header( rinfo );
	}
	if( psockparam->opcode == 2 ){
		psockparam->opcode = 0;
		psockparam->phase = QS_HTTP_SOCK_PHASE_MSG_HTTP;
		psockparam->tmpmsglen = 0;
		rinfo->recvlen = 0;
		if( -1 != qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
			rinfo->recvlen = atoi( (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
		}
		rinfo->recvbuf_munit = tinfo->recvmsg_munit;
		return psockparam->phase;
	}
	else{
		return psockparam->phase; // continue
	}
	qs_disconnect( psockparam );
	return -1;
}

int qs_http_parser( QS_RECV_INFO *rinfo )
{
	int method;
	char* target_pt;
	char headername[256];
	char headerparam[2048];
	method = 0;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	char *target = (char*)qs_upointer(option->memory_pool, rinfo->recvbuf_munit);
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	do{
		if(rinfo->recvlen>=qs_usize(option->memory_pool, rinfo->recvbuf_munit)){
			//printf("invalid size : %s\n", (char*)qs_upointer(option->memory_pool, rinfo->recvbuf_munit));
			method = -1;
			break;
		}
		target[rinfo->recvlen] = '\0';
		target_pt = target;
		target_pt = qs_read_line_delimiter( headername, sizeof(headername), target_pt, ' ' );
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
			//printf("invalid method : %s\n", (char*)qs_upointer(option->memory_pool, rinfo->recvbuf_munit));
			method = -1;
			break;
		}
		psockparam->opcode = 0;
		const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;
		int i;
		if( psockparam->http_header_munit == -1 )
		{
			if( -1 == ( psockparam->http_header_munit = qs_create_hash( option->memory_pool, 16 ) ) ){
				break;
			}
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				qs_add_hash_emptystring( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]), atoi((char*)(http_header_strings[i][2])) );
			}
		}
		else{
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				qs_clear_hash_string( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]) );
			}
		}
		qs_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "HTTP_METHOD", headername );
		target_pt = qs_read_line_delimiter( headerparam, sizeof(headerparam), target_pt, ' ' );
		if( strcmp( headerparam, "/" ) != 0 )
		{
			int32_t pos = qs_find_char( headerparam, strlen( headerparam ), '?' );
			if( pos > 0 )
			{
				char params_buf[2048];
				size_t params_buf_size = sizeof(headerparam) - (pos+1);
				//memset( params_buf, 0, sizeof(params_buf));
				memcpy( params_buf, &headerparam[pos+1], params_buf_size );
				headerparam[pos] = '\0';
				qs_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "GET_PARAMS", params_buf );
			}
			else{
				if( 0 < ( pos = qs_find_char( headerparam, strlen( headerparam ), '&' ) ) ){
					headerparam[pos] = '\0';
				}
			}
		}
		char request_path[2048];
		if( QS_SYSTEM_ERROR == qs_escape_directory_traversal( request_path, headerparam, sizeof(request_path) ) ){
			char* msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n400 Bad Request\r\n";
			if( option->user_send_function != NULL ){
				//option->user_send_function(&childinfo, psockparam->acc, msg, strlen(msg), 0);
				//printf("call user_send_function\n");
			}
			else{
				if( -1 == qs_send( option, psockparam, msg, qs_strlen( msg ), 0 ) ){
					//printf("http send error\n");
				}
			}
			method = -1;
			break;
		}
		qs_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "REQUEST", request_path );
		target_pt = qs_read_line_delimiter( headername, sizeof(headername), target_pt, '\0' );
		qs_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "HTTP_VERSION", headername );
		qs_http_parse_header( rinfo );
	}while( false );
	return method;
}
int qs_http_parse_header( QS_RECV_INFO *rinfo )
{
	char headername[256];
	char headerparam[2048];
	char* target_pt;
	const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;

	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	char *target = (char*)qs_upointer(option->memory_pool, rinfo->recvbuf_munit);
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	int32_t recvmsg_munit = tinfo->recvmsg_munit;

	do{
		target_pt = target;
		for(;;){
			if(psockparam->opcode==3){
				if( -1 != qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
					int contentlen = atoi( (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
					size_t write_size = 0;
					size_t current = 0;
					while(current<rinfo->recvlen){
						if(*(target_pt+current) == '-'){
							char tmpkey[256];
							memcpy(tmpkey,(target_pt+current),qs_strlen((char*)psockparam->header));
							tmpkey[qs_strlen((char*)psockparam->header)] = '\0';
							printf("is key ? : [%s][%s]\n",(char*)psockparam->header,tmpkey);
							if(!strcmp(tmpkey,(char*)psockparam->header)){
								write_size = current - 2 -2;
								break;
							}
						}
						write_size++;
						current++;
					}
					//size_t write_size = rinfo->recvlen;
					//printf("save to file : %d\n",(int)psockparam->header_size);
					//if((psockparam->tmpmsglen+rinfo->recvlen)>(contentlen-psockparam->header_size)){
					//	write_size = rinfo->recvlen - (psockparam->tmpmsglen+rinfo->recvlen-(contentlen-psockparam->header_size));
					//}
					printf("write_size : %d\n",(int)write_size);
					if(write_size>0){
						qs_fwrite_bin_a( "./out.data", target, write_size );
					}
					psockparam->tmpmsglen +=rinfo->recvlen;
					if( psockparam->tmpmsglen >= contentlen ){
						psockparam->opcode = 2;
						printf("upload data size full\n");
					}
				}
				else{
					// error
				}
				break;
			}
			if( *target_pt=='\n' || *target_pt=='\r' ){
				psockparam->opcode = 1;
				psockparam->tmpmsglen = 0;
			}
			if( psockparam->opcode == 1 ){

				if(false)
				{
					if(qs_get_hash(option->memory_pool, psockparam->http_header_munit, "CONTENT_TYPE") != -1){
						char* contentType = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash(option->memory_pool, psockparam->http_header_munit, "CONTENT_TYPE"));
						char* pt = contentType;
						char param[256];
						char formkey[256];
						pt = qs_read_line_delimiter( param, sizeof(param), pt, ';' );
						if(!strcmp(param,"multipart/form-data")){
							pt = qs_read_line_delimiter( param, sizeof(param), pt, '=' );
							printf("is form-data [%s] = [%s]\n",param,pt);
							qs_unlink( "./out.data" );
							char* ppt = target_pt;
							ppt = qs_read_line_delimiter( param, sizeof(param), ppt, ' ' );
							// first --key
							// data
							// last --key--
							//printf("[%s] = [%s]\n",param,pt);
							memcpy(formkey,param+2,sizeof(param)-2); // remove --
							printf("form key : %s\n",formkey);
							memcpy(psockparam->header,formkey,sizeof(psockparam->header));
							if(!strcmp(pt,formkey)){
								psockparam->header_size = qs_strlen(param) + 4 + 2; // last -- and \n * 4
								char key[256];
								char type[256];
								ppt = qs_read_line_delimiter( key, sizeof(key), ppt, ':' );
								ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ' ' );
								ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ';' );
								printf("find key : %s, key : %s, type : %s\n",param,key,type);
								ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ' ' );
								ppt = qs_read_line_delimiter( key, sizeof(key), ppt, '=' );
								ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ';' );
								printf("param name : key : %s value : %s\n",key,type);
								// upload file name
								if(*(ppt-1)==';'){
									ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ' ' );
									ppt = qs_read_line_delimiter( key, sizeof(key), ppt, '=' );
									ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ';' );
									printf("file name : key : %s value : %s\n",key,type);
									ppt = qs_read_line_delimiter( key, sizeof(key), ppt, ':' );
									ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ' ' );
									ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ';' );
									printf("content key : %s value : %s\n",key,type);
									ppt = qs_read_line_delimiter( type, sizeof(type), ppt, ' ' );
								}
								else{
									ppt+=2; // \n*2
								}
								psockparam->opcode = 3;

								if( -1 == qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
									// error
									break;
								}
								int contentlen = atoi( (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
								size_t write_size = 0;
								size_t current = (ppt-target_pt);
								while(current<rinfo->recvlen){
									if(*(target_pt+current) == '-'){
										char tmpkey[256];
										memcpy(tmpkey,(target_pt+current),qs_strlen(formkey));
										tmpkey[qs_strlen(formkey)] = '\0';
										printf("is key ? : [%s]\n",tmpkey);
										if(!strcmp(tmpkey,formkey)){
											write_size = current - (ppt-target_pt) - 2 -2;
											printf("first write_size : %d\n",(int)write_size);
											break;
										}
									}
									current++;
								}
								//write_size = rinfo->recvlen - (ppt-target_pt);
								//if(psockparam->tmpmsglen+rinfo->recvlen>contentlen){
								//	write_size = rinfo->recvlen - (psockparam->tmpmsglen+rinfo->recvlen-contentlen);
								//}
								if(write_size>0){
									qs_fwrite_bin_a( "./out.data", ppt, write_size );
								}
								psockparam->tmpmsglen +=rinfo->recvlen;
								if( psockparam->tmpmsglen >= contentlen ){
									psockparam->opcode = 2;
									printf("upload dataaa size full\n");
									//printf("form-data size : %d, %d\n",(int)contentlen,(int)psockparam->tmpmsglen);
								}
								break;
							}
						}
					}
				}

				if( -1 != qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
					int contentlen = atoi( (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
					if( contentlen > 0 )
					{
						char* msgbuf = ( (char*)QS_GET_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
						ssize_t tmprcvlen = rinfo->recvlen;
						int newline = 0;
						while( *target_pt=='\n' || *target_pt=='\r' ){ target_pt++; tmprcvlen--; newline=1; }
						if( newline == 1 && *target_pt == '\0' ){

						}
						else{
							memcpy( msgbuf, target_pt, tmprcvlen );
							msgbuf+=tmprcvlen;
							psockparam->tmpmsglen += tmprcvlen;
							if( psockparam->tmpmsglen >= contentlen ){
								*msgbuf = '\0';
								psockparam->opcode = 2;
								break;
							}
						}
						//while( *target_pt != '\0' ){
						//	*(msgbuf++) = *(target_pt++);
						//	psockparam->tmpmsglen++;
						//	if( psockparam->tmpmsglen >= contentlen ){
						//		*msgbuf = '\0';
						//		psockparam->opcode = 2;
						//		break;
						//	}
						//}
					}
					else{
						char* msgbuf = ( (char*)QS_GET_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
						*msgbuf = '\0';
						psockparam->opcode = 2;
					}
				}
				else{
					char* msgbuf = ( (char*)QS_GET_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
					*msgbuf = '\0';
					psockparam->opcode = 2;
				}
				break;
			}
			target_pt = qs_read_line_delimiter( headername, sizeof(headername), target_pt, ' ' );
			target_pt = qs_read_line_delimiter( headerparam, sizeof(headerparam), target_pt, '\0' );
			int i;
			int is_push = 0;
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				if( !strcmp( headername, http_header_strings[i][0] ) )
				{
					qs_replace_hash_string( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]), headerparam );
					is_push = 1;
				}
			}
			if( is_push == 0 ){
				//printf("Not Support %s: %s\n",headername,headerparam);
			}
			if( (*target_pt) == '\0' ){
				break;
			}
		}
	}while( false );
	return 0;
}

size_t qs_http_add_response_common(char* dest, size_t dest_size, int http_response_code, char* content_type, size_t content_length)
{
	char content_len_string[32];
	char time_string[256];
	char* http_server = "Server: qs-http-server\r\n";
	char* http_connection = "Connection: close\r\n";
	char* http_content = "Content-Type: ";
	char* http_content_len = "Content-Length: ";
	char* http_date = "Date: ";
	size_t len = 0;
	//len = qs_strlink( dest, len, http_version, qs_strlen(http_version), dest_size );
	switch(http_response_code){
		case 200:
			len = qs_strlink( dest, len, HTTP_OK, qs_strlen(HTTP_OK), dest_size );
			len = qs_strlink( dest, len, http_server, qs_strlen(http_server), dest_size );
			len = qs_strlink( dest, len, http_connection, qs_strlen(http_connection), dest_size );
			len = qs_strlink( dest, len, http_content, qs_strlen(http_content), dest_size );
			len = qs_strlink( dest, len, content_type, qs_strlen(content_type), dest_size );
			len = qs_strlink( dest, len, "\r\n", 2, dest_size );
			qs_itoa( content_length, content_len_string, sizeof(content_len_string) );
			len = qs_strlink( dest, len, http_content_len, qs_strlen(http_content_len), dest_size );
			len = qs_strlink( dest, len, content_len_string, qs_strlen(content_len_string), dest_size );
			len = qs_strlink( dest, len, "\r\n", 2, dest_size );
			qs_utc_time( time_string, sizeof(time_string) );
			len = qs_strlink( dest, len, http_date, qs_strlen(http_date), dest_size );
			len = qs_strlink( dest, len, time_string, qs_strlen(time_string), dest_size );
			len = qs_strlink( dest, len, "\r\n", 2, dest_size );
			break;
		case 304:
			len = qs_strlink( dest, len, HTTP_NOT_MODIFIED, qs_strlen(HTTP_NOT_MODIFIED), dest_size );
			break;
		case 400:
			len = qs_strlink( dest, len, HTTP_BAD_REQUEST_ERROR, qs_strlen(HTTP_BAD_REQUEST_ERROR), dest_size );
			break;
		case 500:
			len = qs_strlink( dest, len, HTTP_INTERNAL_SERVER_ERROR, qs_strlen(HTTP_INTERNAL_SERVER_ERROR), dest_size );
			break;
	}
	return len;
}

size_t qs_http_add_cache_control(char* dest, size_t dest_size, size_t start, int max_age, QS_FILE_INFO* info)
{
	size_t len = start;
	char* http_cache_control = "Cache-Control: ";
	char* http_cache_control_max_age = "max-age=";
	char age_buffer[20];
	qs_itoa( max_age, age_buffer, sizeof(age_buffer) );
	len = qs_strlink( dest, len, http_cache_control, qs_strlen(http_cache_control), dest_size );
	len = qs_strlink( dest, len, http_cache_control_max_age, qs_strlen(http_cache_control_max_age), dest_size );
	len = qs_strlink( dest, len, age_buffer, qs_strlen(age_buffer), dest_size );
	len = qs_strlink( dest, len, "\r\n", 2, dest_size );

	char* http_last_modified = "Last-Modified: ";
	char time_buffer[64];


#ifdef __WINDOWS__
	__time64_t long_time = info->update_usec;
	struct tm gm_time;
	gmtime_s(&gm_time, &long_time);
	asctime_s(time_buffer, sizeof(time_buffer), &gm_time);
	time_buffer[strlen(time_buffer) - 1] = '\0';
#else
	time_t t = info->update_usec;
	struct tm *gm_time = gmtime(&t);
	snprintf(time_buffer, sizeof(time_buffer), "%s", asctime(gm_time));
	time_buffer[strlen(time_buffer) - 1] = '\0';
#endif
	len = qs_strlink( dest, len, http_last_modified, qs_strlen(http_last_modified), dest_size );
	len = qs_strlink( dest, len, time_buffer, qs_strlen(time_buffer), dest_size );
	len = qs_strlink( dest, len, "\r\n", 2, dest_size );
	return len;
}

size_t qs_http_document_path(char* dest, size_t dest_size,char* document_root, char* default_file, char* path)
{
	size_t path_len = 0;
	path_len = qs_strlink( dest, path_len, document_root, qs_strlen(document_root), dest_size );
	path_len = qs_strlink( dest, path_len, path, qs_strlen(path), dest_size );
	if(!strcmp(path,"/")){
		path_len = qs_strlink( dest, path_len, default_file, qs_strlen(default_file), dest_size );
	}
	dest[path_len] = '\0';
	return path_len;
}

int32_t qs_http_parse_request_parameter(QS_MEMORY_POOL * memory,char *get_params, size_t buffer_size)
{
	int32_t memid_get_parameter_hash = -1;
	do{
		char* pparam = get_params;
		char param_name[1024];
		int32_t memid_value = qs_create_memory_block(memory,buffer_size);
		if( -1 == memid_value ){
			break;
		}
		int32_t memid_decode = qs_create_memory_block(memory,buffer_size);
		if( -1 == memid_decode ){
			break;
		}
		char *param_value = (char*)QS_GET_POINTER(memory,memid_value);
		char *param_urldecode = (char*)QS_GET_POINTER(memory,memid_decode);
		if( -1 == ( memid_get_parameter_hash = qs_create_hash( memory, 32 ) ) ){
			break;
		}
		for(;*pparam != '\0';){
			pparam = qs_read_line_delimiter( param_name, qs_usize(memory,memid_value), pparam, '=' );
			pparam = qs_read_line_delimiter( param_value, qs_usize(memory,memid_decode), pparam, '&' );
			qs_urldecode( param_urldecode, qs_usize(memory,memid_decode), param_value );
			qs_add_hash_value( memory, memid_get_parameter_hash, param_name, param_urldecode,ELEMENT_LITERAL_STR );
		}
	}while(false);
	return memid_get_parameter_hash;
}

int32_t http_request_common(QS_RECV_INFO *rinfo, QS_HTTP_REQUEST_COMMON* http_request, QS_MEMORY_POOL* temporary_memory)
{
	http_request->http_status_code = 500;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;

	int32_t memid_headers = tinfo->sockparam.http_header_munit;
	//printf("headers\n");
	//qs_hash_dump(option->memory_pool, memid_headers,0);

	http_request->temporary_memory = temporary_memory;
	http_request->method = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "HTTP_METHOD" ));
	http_request->request = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "REQUEST" ));
	http_request->get_params = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "GET_PARAMS" ));
	http_request->content_type = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "CONTENT_TYPE" ));
	http_request->cache_control = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "CACHE_CONTROL" ));
	http_request->http_version = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "HTTP_VERSION" ));
	http_request->user_agent = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "USER_AGENT" ));
	//printf("method : %s , request : %s\n",http_request->method,http_request->request);

	http_request->memid_get_parameter_hash = -1;
	do{
		// get parameter
		if(-1==(http_request->memid_get_parameter_hash = qs_http_parse_request_parameter(temporary_memory,http_request->get_params, SIZE_KBYTE*64))){
			break;
		}
		//printf("get params\n");
		//qs_hash_dump(temporary_memory, http_request->memid_get_parameter_hash,0);
	}while(false);

	do{
		int32_t memid_request_path = qs_create_memory_block(temporary_memory,MAXPATHLEN);
		if( -1 == memid_request_path ){
			break;
		}
		int32_t memid_extension = qs_create_memory_block(temporary_memory,32);
		if( -1 == memid_extension ){
			break;
		}
		http_request->request_path = (char*)QS_GET_POINTER(temporary_memory,memid_request_path);
		memset(http_request->request_path,0,MAXPATHLEN);
		qs_http_document_path(http_request->request_path,MAXPATHLEN,"./www","index.html",http_request->request);

		http_request->extension = (char*)QS_GET_POINTER(temporary_memory,memid_extension);
		memset(http_request->extension,0,32);
		qs_get_extension( http_request->extension, 32, http_request->request_path );
	}while(false);

	http_request->memid_post_parameter_hash = -1;
	if( !strcmp("POST",http_request->method) ){
		// parse post parameters
		char *body = (char*)qs_upointer(option->memory_pool, rinfo->recvbuf_munit);
		do{
			if( !strcmp("application/json",http_request->content_type)){
				QS_NODE* hashroot = qs_get_json_root(temporary_memory, qs_json_decode_h(temporary_memory, body, 8, 128));
				if (NULL == hashroot || hashroot->id != ELEMENT_HASH) {
					break;
				}
				http_request->memid_post_parameter_hash = hashroot->element_munit;
			}else{
				if(-1==(http_request->memid_post_parameter_hash = qs_http_parse_request_parameter(temporary_memory,body, SIZE_KBYTE*512))){
					break;
				}
			}
		}while(false);
		//printf("body : %s\n",body);
		//printf("post params\n");
		//qs_hash_dump(temporary_memory, http_request->memid_post_parameter_hash,0);
	}

	do{
		if( QS_SYSTEM_ERROR == qs_fget_info( http_request->request_path, &http_request->file_info ) ){
			http_request->http_status_code = 404;
			break;
		}
		//response
		size_t header_size = SIZE_KBYTE * 2;
		int32_t response_buffer_munit = qs_create_memory_block(temporary_memory,http_request->file_info.size+header_size);
		if( -1 == response_buffer_munit ){
			break;
		}
		char* response_buffer = (char*)QS_GET_POINTER(temporary_memory,response_buffer_munit);
		size_t response_buffer_size = qs_usize(temporary_memory,response_buffer_munit);
		//memset(response_buffer, 0, response_buffer_size); // memory over ( windows only )
		size_t response_len = 0;
		int is_binary = 0;

		if( !strcmp(http_request->extension,"html"))
		{
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer,response_buffer_size,http_request->http_status_code,"text/html",http_request->file_info.size);
			response_len = qs_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &http_request->file_info);
		}
		else if( !strcmp(http_request->extension,"css"))
		{
			if( !strcmp(http_request->cache_control,"max-age=0") ){
				char *modified_since = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "IF_MODIFIED_SINCE" ));
				if( strcmp("",modified_since)){
					http_request->http_status_code = 304;
					break;
				}
			}
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer,response_buffer_size,http_request->http_status_code,"text/css",http_request->file_info.size);
			response_len = qs_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &http_request->file_info);
		}
		else if( !strcmp(http_request->extension,"js"))
		{
			if( !strcmp(http_request->cache_control,"max-age=0") ){
				char *modified_since = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, memid_headers, "IF_MODIFIED_SINCE" ));
				if( strcmp("",modified_since)){
					http_request->http_status_code = 304;
					break;
				}
			}
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer,response_buffer_size,http_request->http_status_code,"text/javascript",http_request->file_info.size);
			response_len = qs_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &http_request->file_info);
		}
		else if (!strcmp(http_request->extension, "json"))
		{
			if (!strcmp(http_request->cache_control, "max-age=0")) {
				char *modified_since = (char*)QS_GET_POINTER(option->memory_pool, qs_get_hash(option->memory_pool, memid_headers, "IF_MODIFIED_SINCE"));
				if (strcmp("", modified_since)) {
					http_request->http_status_code = 304;
					break;
				}
			}
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer, response_buffer_size, http_request->http_status_code, "application/json", http_request->file_info.size);
			response_len = qs_http_add_cache_control(response_buffer, response_buffer_size, response_len, 30, &http_request->file_info);
		}
		else if( !strcmp(http_request->extension,"ico"))
		{
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer,response_buffer_size,http_request->http_status_code,"image/x-icon",http_request->file_info.size);
			is_binary = 1;
		}
		else if (!strcmp(http_request->extension, "unityweb"))
		{
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer, response_buffer_size, http_request->http_status_code, "application/octet-stream", http_request->file_info.size);
			is_binary = 1;
		}
		else if (!strcmp(http_request->extension, "png"))
		{
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer, response_buffer_size, http_request->http_status_code, "image/png", http_request->file_info.size);
			is_binary = 1;
		}
		else if (!strcmp(http_request->extension, "jpg"))
		{
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer, response_buffer_size, http_request->http_status_code, "image/jpeg", http_request->file_info.size);
			is_binary = 1;
		}
		else if (!strcmp(http_request->extension, "mp3"))
		{
			http_request->http_status_code = 200;
			response_len = qs_http_add_response_common(response_buffer, response_buffer_size, http_request->http_status_code, "audio/mp3", http_request->file_info.size);
			is_binary = 1;
		}
		else{
			http_request->http_status_code = 404;
			break;
		}
		response_len = qs_strlink( response_buffer, response_len, "\r\n", 2, response_buffer_size );
		if( !(!strcmp("HEAD",http_request->method)) )
		{
			char* pstart = response_buffer+response_len;
			size_t plen = response_buffer_size-response_len;
			if(is_binary==0){
				size_t readlen = qs_fread_bin( http_request->request_path, pstart, plen );
				response_len+=readlen;
			}
			else{
				size_t readlen = qs_fread_bin( http_request->request_path, pstart, plen );
				response_len+=readlen;
			}
		}
		response_buffer[response_len] = '\0';
		qs_send( option, &tinfo->sockparam, response_buffer, response_len, 0 );
	}while(false);
	return http_request->http_status_code;
}

int32_t http_json_response_common(QS_SERVER_CONNECTION_INFO * connection, QS_SOCKET_OPTION* option,QS_MEMORY_POOL* temporary_memory,int32_t memid_response_hash, size_t json_buffer_size)
{
	int32_t http_status_code = 200;
	int32_t memid_response_body = qs_json_encode_hash(temporary_memory, memid_response_hash, json_buffer_size);
	if (-1 == memid_response_body) {
		return 500;
	}
	char* json = (char*)QS_GET_POINTER(temporary_memory, memid_response_body);
	size_t body_len = qs_strlen(json);
	int32_t response_buffer_munit = qs_create_memory_block(temporary_memory, body_len + SIZE_KBYTE);
	char* response_buffer = (char*)QS_GET_POINTER(temporary_memory, response_buffer_munit);
	size_t response_buffer_size = qs_usize(temporary_memory, response_buffer_munit);
	memset(response_buffer, 0, response_buffer_size); // memory over ( windows only )
	size_t response_len = 0;
	response_len = qs_http_add_response_common(response_buffer, response_buffer_size, http_status_code, "application/json", body_len);
	response_len = qs_strlink(response_buffer, response_len, "\r\n", 2, response_buffer_size);
	response_len = qs_strlink(response_buffer, response_len, json, body_len, response_buffer_size);
	response_buffer[response_len] = '\0';
	qs_send(option, &connection->sockparam, response_buffer, response_len, 0);
	return http_status_code;
}

int qs_http_protocol_filter_with_websocket(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	if( psockparam->phase <= QS_HTTP_SOCK_PHASE_MSG_HTTP )
	{
		if( option->socket_type == SOCKET_TYPE_SERVER_UDP || option->socket_type == SOCKET_TYPE_CLIENT_UDP ){
			return -1;
		}
		switch( qs_http_protocol_filter(rinfo) )
		{
			case -1:
				qs_disconnect( psockparam );
				return -1;
			case QS_HTTP_SOCK_PHASE_RECV_CONTINUE:
				return QS_HTTP_SOCK_PHASE_RECV_CONTINUE;
			case QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER:
				return QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER;
			case QS_HTTP_SOCK_PHASE_MSG_HTTP:
				break;
		}
		int ws_hand_shake = qs_send_handshake_param( rinfo->recvfrom, option, psockparam );
		if( -1 == ws_hand_shake ){
			qs_disconnect( psockparam );
			return -1;
		}
		if( 0 == ws_hand_shake ){ // is not websocket
			return QS_HTTP_SOCK_PHASE_MSG_HTTP;
		}
		psockparam->phase = QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET;
		return QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET;
	}
	psockparam->phase = QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET;
	return QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET;
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
ssize_t qs_parse_websocket_binary( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit )
{
	int i, j, startpos;
	uint64_t cnt = 0;
	uint64_t tmppayloadlen = 0;
	uint8_t* msg = (uint8_t*)qs_upointer( option->memory_pool,basebuf_munit );
	ssize_t retsize = -1;
	do{
		if( psockparam->fin == 0 || ( psockparam->fin == 1 && psockparam->tmpmsglen == 0 ) )
		{
			if( psockparam->fin == 0 ){
				cnt = psockparam->tmpmsglen;
			}
			else{
				memset( msg, 0, qs_usize( option->memory_pool, basebuf_munit ) );
				psockparam->payloadlen = 0;
			}
			psockparam->fin			= u8buf[0] >> 7;
			psockparam->rsv			= ( u8buf[0] & 0x70 ) >> 4;
			psockparam->opcode			= ( u8buf[0] & 0x0f );
			psockparam->mask			= u8buf[1] >> 7;
			psockparam->ckpayloadlen	= ( u8buf[1] & 0x7f );
			if( psockparam->mask == 0 ){
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

			if( psockparam->payloadlen >= qs_usize( option->memory_pool, basebuf_munit ) ){
				printf( "payloadlen buffersize over[%"PRIu64"][%zd]\n", psockparam->payloadlen, qs_usize( option->memory_pool, basebuf_munit ) );
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
#ifdef __QS_DEBUG__
		//		printf( "websocket protocol : fin[%d], rsv[%d], opcode[%d], mask[%d], payload[%llu],tmplen[%zd], pktsize[%zd], [%zd]\n"
		//				, psockparam->fin
		//				, psockparam->rsv
		//				, psockparam->opcode
		//				, psockparam->mask
		//				, psockparam->payloadlen
		//				, psockparam->tmpmsglen
		//				, size
		//				, qs_usize( option->memory_pool, basebuf_munit )
		//				);
#endif
		if( psockparam->opcode == 8 ){
			//printf("opecode : 8(close)\n");
			//uint16_t close_code = msg[0] << 8 | msg[1];
			//printf("close_code : %d\n",close_code);
			break;
		}
		if( psockparam->fin && psockparam->tmpmsglen >= psockparam->payloadlen )
		{
			psockparam->fin				= 1;
			psockparam->rsv				= 0;
			psockparam->opcode				= 0;
			psockparam->mask				= 0;
			psockparam->ckpayloadlen		= 0;
			psockparam->maskindex			= 0;
			psockparam->payloadmask		= 0;
			psockparam->tmpmsglen			= 0;
			psockparam->tmpbitsift			= -1;
			psockparam->appdata32bit		= 0x00000000;
			msg[psockparam->payloadlen]	= '\0';
			retsize = psockparam->payloadlen;
		}
		else{
			msg[psockparam->tmpmsglen]		= '\0';
			retsize						= psockparam->tmpmsglen;
		}
	}while( false );
	return retsize;
}
 
ssize_t qs_make_websocket_msg( void* message_buffer, size_t message_buffer_size,int is_binary, const char* msg, ssize_t size )
{
	void *sendbin = message_buffer;
	uint8_t head1;
	uint8_t head2;
	uint8_t *ptr;
	char* cptr;
	uint32_t headersize = 0;
	ssize_t len = 0;
	do{
		head1 = 0x80 | ( (is_binary) ? WS_MODE_BINARY : WS_MODE_TEXT ); // 0x81:text mode, 0x82:binary mode
		if( size <= 125 ){
			headersize = 2;
			head2 = 0x00 | (uint8_t)size;
		}
		else if( size >= 126 && size <= 65536 ){
			headersize = 4;
			head2 = 0x00 | 126;
		}
		else{
			headersize = 10;
			head2 = 0x00 | 127;
		}
		len = size + headersize;
		if( message_buffer_size < len ){
			printf( "[qs_send_websocket_msg]size over: %zd byte\n", len );
			break;
		}
		memset( sendbin, 0, message_buffer_size );
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
	}while( false );
	return len;
}

int qs_send_handshake_param(QS_SOCKET_ID socket, QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam )
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
	const char* ws_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	if( qs_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_KEY" ) ){
		pwskey = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_KEY" ));
	}
	if( qs_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_PROTOCOL" ) ){
		pprotocol = (char*)QS_GET_POINTER(option->memory_pool,qs_get_hash( option->memory_pool, psockparam->http_header_munit, "SEC_WEBSOCKET_PROTOCOL" ));
	}
	if( pwskey == NULL || pwskey[0] == '\0' )
	{
		return 0;
	}
	memset(shaHash, 0, sizeof(shaHash));
	memset( sendbuffer, 0 ,sizeof( sendbuffer ) );
	pbuffer = sendbuffer;
	length = strlen(pwskey) + strlen( ws_guid );
	if( psockparam->wsockkey_munit < 0 || qs_usize( option->memory_pool, psockparam->wsockkey_munit ) <= length )
	{
		if( psockparam->wsockkey_munit >= 0 ){
			qs_free_memory_unit( option->memory_pool, &psockparam->wsockkey_munit );
		}
		if( ( psockparam->wsockkey_munit = qs_create_munit( option->memory_pool, sizeof( char ) * QS_ALIGNUP( length, 64 ), MEMORY_TYPE_DEFAULT ) ) == -1 )
		{
			printf( "[responseKey]size over: %d byte\n", length );
			return qs_error("[responseKey]size over");
		}
	}
	responseKey = (char*)qs_upointer( option->memory_pool, psockparam->wsockkey_munit );
	memset( responseKey, 0, qs_usize( option->memory_pool, psockparam->wsockkey_munit ) );
	memcpy(responseKey, pwskey, length);
	memcpy(&(responseKey[strlen(pwskey)]), ws_guid, strlen(ws_guid));
	qs_sha1( shaHash, responseKey, length );
	shaHash[20] = '\0';
	qs_base64_encode(responseKey, qs_usize( option->memory_pool, psockparam->wsockkey_munit ) , shaHash, sizeof( shaHash )-1 );
	if( pprotocol == NULL || !strcmp( pprotocol, "" ) )
	{
		buffersize = qs_sprintf((char *)pbuffer,sizeof(sendbuffer),
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
		buffersize = qs_sprintf((char *)pbuffer, sizeof(sendbuffer),
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
				,pprotocol
				);
	}
	if( option->user_send_function != NULL ){
		QS_SERVER_CONNECTION_INFO connection;
		connection.qs_socket_option = option;
		option->user_send_function(&connection, psockparam->acc, (char*)pbuffer, buffersize, 0);
	}
	else{
		sendlen = qs_send_all( socket, (char*)pbuffer, buffersize, 0 );
	}
	if( sendlen == -1 )
	{
		return qs_error("send handshake error");
	}
	if( sendlen != buffersize )
	{
		return qs_error("send handshake buffer size error");
	}
	return 1;
}
