#include "gdt_protocol.h"

uint8_t gdt_get_protocol_header_size_byte(ssize_t payload_size)
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

uint32_t gdt_get_protocol_header_size(ssize_t payload_size)
{
	uint32_t headersize = 2;
	uint8_t header_size_byte = gdt_get_protocol_header_size_byte(payload_size);
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

ssize_t gdt_get_protocol_buffer_size(ssize_t payload_size)
{
	return gdt_get_protocol_header_size(payload_size) + payload_size;	
}

uint32_t gdt_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num)
{
	uint8_t* pbin = bin;
	int endian = gdt_endian();
	uint32_t headersize = 2;
	// mode
	*(pbin++) = 0;
	uint8_t header_size_byte = gdt_get_protocol_header_size_byte(payload_size);
	*(pbin++) = header_size_byte;
	// payload size
	if( header_size_byte == 126 ){
		printf("16bit\n");
		headersize += 2;
		MEMORY_PUSH_BIT16_B2(endian,pbin,payload_size);
	}
	else if( header_size_byte == 127 ){
		printf("64bit\n");
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

ssize_t gdt_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num)
{
	uint8_t* pbin = bin;
	uint32_t headersize = gdt_make_protocol_header(bin,payload_size,payload_type,seq_num);
	pbin += headersize;
	memcpy(pbin,payload,payload_size);
	return headersize + payload_size;
}

int gdt_http_protocol_filter(GDT_RECV_INFO* rinfo)
{
	GDT_SERVER_CONNECTION_INFO * tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	GDT_SOCKPARAM* psockparam = &tinfo->sockparam;
	if( psockparam->phase == 0 )
	{
		int ret = gdt_http_parser( rinfo );
		if( ret == -1 ){
			gdt_disconnect( psockparam );
			return -1;
		}else if( ret == 0 ){
			gdt_disconnect( psockparam );
			return -1;
		}
		else{
			psockparam->phase = 1;
			psockparam->c_status = PROTOCOL_STATUS_HTTP;
			psockparam->type = SOCK_TYPE_NORMAL_TCP;
		}
	}
	else if( psockparam->phase == 1 ){
		gdt_http_parse_header( rinfo );
	}
	if( psockparam->opcode == 2 ){
		psockparam->opcode = 0;
		psockparam->phase = 0;
		psockparam->tmpmsglen = 0;
		rinfo->recvlen = 0;
		if( -1 != gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
			rinfo->recvlen = atoi( (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
		}
		rinfo->recvbuf_munit = tinfo->recvmsg_munit;
		return 1;
	}
	else{
		return 0; // continue
	}
	gdt_disconnect( psockparam );
	return -1;
}

int gdt_http_parser( GDT_RECV_INFO *rinfo )
{
	int method;
	char* target_pt;
	char headername[256];
	char headerparam[2048];
	method = 0;

	GDT_SERVER_CONNECTION_INFO * tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	char *target = (char*)gdt_upointer(option->memory_pool, rinfo->recvbuf_munit);
	target[rinfo->recvlen] = '\0';
	GDT_SOCKPARAM* psockparam = &tinfo->sockparam;
	do{
		target_pt = target;
		target_pt = gdt_read_line_delimiter( headername, sizeof(headername), target_pt, ' ' );
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
			printf("invalid method : %s\n", (char*)gdt_upointer(option->memory_pool, rinfo->recvbuf_munit));
			method = -1;
			break;
		}
		psockparam->opcode = 0;
		const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;
		int i;
		if( psockparam->http_header_munit == -1 )
		{
			if( -1 == ( psockparam->http_header_munit = gdt_create_hash( option->memory_pool, 16 ) ) ){
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
		target_pt = gdt_read_line_delimiter( headerparam, sizeof(headerparam), target_pt, ' ' );
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
			char* msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n400 Bad Request\r\n";
			if( option->user_send_function != NULL ){
				//option->user_send_function(&childinfo, psockparam->acc, msg, strlen(msg), 0);
				printf("call user_send_function\n");
			}
			else{
				if( -1 == gdt_send( option, psockparam, msg, gdt_strlen( msg ), 0 ) ){
					printf("http send error\n");
				}
			}
			method = -1;
			break;
		}
		gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "REQUEST", request_path );
		target_pt = gdt_read_line_delimiter( headername, sizeof(headername), target_pt, '\0' );
		gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, "HTTP_VERSION", headername );
		gdt_http_parse_header( rinfo );
	}while( false );
	return method;
}
int gdt_http_parse_header( GDT_RECV_INFO *rinfo )
{
	char headername[256];
	char headerparam[2048];
	char* target_pt;
	const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;

	GDT_SERVER_CONNECTION_INFO * tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	char *target = (char*)gdt_upointer(option->memory_pool, rinfo->recvbuf_munit);
	GDT_SOCKPARAM* psockparam = &tinfo->sockparam;
	int32_t recvmsg_munit = tinfo->recvmsg_munit;

	do{
		target_pt = target;
		for(;;){
			if(psockparam->opcode==3){
				if( -1 != gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
					int contentlen = atoi( (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
					size_t write_size = 0;
					size_t current = 0;
					while(current<rinfo->recvlen){
						if(*(target_pt+current) == '-'){
							char tmpkey[256];
							memcpy(tmpkey,(target_pt+current),gdt_strlen((char*)psockparam->header));
							tmpkey[gdt_strlen((char*)psockparam->header)] = '\0';
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
						gdt_fwrite_bin_a( "./out.data", target, write_size );
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
				if(gdt_get_hash(option->memory_pool, psockparam->http_header_munit, "CONTENT_TYPE") != -1){
					char* contentType = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash(option->memory_pool, psockparam->http_header_munit, "CONTENT_TYPE"));
					char* pt = contentType;
					char param[256];
					char formkey[256];
					pt = gdt_read_line_delimiter( param, sizeof(param), pt, ';' );
					if(!strcmp(param,"multipart/form-data")){
						pt = gdt_read_line_delimiter( param, sizeof(param), pt, '=' );
						printf("is form-data [%s] = [%s]\n",param,pt);
						gdt_unlink( "./out.data" );
						char* ppt = target_pt;
						ppt = gdt_read_line_delimiter( param, sizeof(param), ppt, ' ' );
						// first --key
						// data
						// last --key--
						//printf("[%s] = [%s]\n",param,pt);
						memcpy(formkey,param+2,sizeof(param)-2); // remove --
						printf("form key : %s\n",formkey);
						memcpy(psockparam->header,formkey,sizeof(psockparam->header));
						if(!strcmp(pt,formkey)){
							psockparam->header_size = gdt_strlen(param) + 4 + 2; // last -- and \n * 4
							char key[256];
							char type[256];
							ppt = gdt_read_line_delimiter( key, sizeof(key), ppt, ':' );
							ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ' ' );
							ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ';' );
							printf("find key : %s, key : %s, type : %s\n",param,key,type);
							ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ' ' );
							ppt = gdt_read_line_delimiter( key, sizeof(key), ppt, '=' );
							ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ';' );
							printf("param name : key : %s value : %s\n",key,type);
							// upload file name
							if(*(ppt-1)==';'){
								ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ' ' );
								ppt = gdt_read_line_delimiter( key, sizeof(key), ppt, '=' );
								ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ';' );
								printf("file name : key : %s value : %s\n",key,type);
								ppt = gdt_read_line_delimiter( key, sizeof(key), ppt, ':' );
								ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ' ' );
								ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ';' );
								printf("content key : %s value : %s\n",key,type);
								ppt = gdt_read_line_delimiter( type, sizeof(type), ppt, ' ' );
							}
							else{
								ppt+=2; // \n*2
							}
							psockparam->opcode = 3;

							if( -1 == gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
								// error
								break;
							}
							int contentlen = atoi( (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
							size_t write_size = 0;
							size_t current = (ppt-target_pt);
							while(current<rinfo->recvlen){
								if(*(target_pt+current) == '-'){
									char tmpkey[256];
									memcpy(tmpkey,(target_pt+current),gdt_strlen(formkey));
									tmpkey[gdt_strlen(formkey)] = '\0';
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
								gdt_fwrite_bin_a( "./out.data", ppt, write_size );
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
				if( -1 != gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" ) ){
					int contentlen = atoi( (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, psockparam->http_header_munit, "CONTENT_LENGTH" )) );
					if( contentlen > 0 )
					{
						char* msgbuf = ( (char*)GDT_POINTER(option->memory_pool,recvmsg_munit) ) + psockparam->tmpmsglen;
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
						//						 while( *target_pt != '\0' ){
						//							 *(msgbuf++) = *(target_pt++);
						//							 psockparam->tmpmsglen++;
						//							 if( psockparam->tmpmsglen >= contentlen ){
						//								 *msgbuf = '\0';
						//								 psockparam->opcode = 2;
						//								 break;
						//							 }
						//						 }
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
			target_pt = gdt_read_line_delimiter( headername, sizeof(headername), target_pt, ' ' );
			target_pt = gdt_read_line_delimiter( headerparam, sizeof(headerparam), target_pt, '\0' );
			int i;
			int is_push = 0;
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				if( !strcmp( headername, http_header_strings[i][0] ) )
				{
					gdt_replace_hash_string( option->memory_pool, psockparam->http_header_munit, (char*)(http_header_strings[i][1]), headerparam );
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


size_t gdt_http_add_response_common(char* dest, size_t dest_size, int http_response_code, char* content_type, size_t content_length)
{
	char content_len_string[32];
	char time_string[256];
	char* http_server = "Server: gdt-httpd\r\n";
	char* http_connection = "Connection: close\r\n";
	char* http_content = "Content-Type: ";
	char* http_content_len = "Content-Length: ";
	char* http_date = "Date: ";
	size_t len = 0;
	//len = gdt_strlink( dest, len, http_version, gdt_strlen(http_version), dest_size );
	switch(http_response_code){
		case 200:
			len = gdt_strlink( dest, len, HTTP_OK, gdt_strlen(HTTP_OK), dest_size );
			len = gdt_strlink( dest, len, http_server, gdt_strlen(http_server), dest_size );
			len = gdt_strlink( dest, len, http_connection, gdt_strlen(http_connection), dest_size );
			len = gdt_strlink( dest, len, http_content, gdt_strlen(http_content), dest_size );
			len = gdt_strlink( dest, len, content_type, gdt_strlen(content_type), dest_size );
			len = gdt_strlink( dest, len, "\r\n", 2, dest_size );
			gdt_itoa( content_length, content_len_string, sizeof(content_len_string) );
			len = gdt_strlink( dest, len, http_content_len, gdt_strlen(http_content_len), dest_size );
			len = gdt_strlink( dest, len, content_len_string, gdt_strlen(content_len_string), dest_size );
			len = gdt_strlink( dest, len, "\r\n", 2, dest_size );
			gdt_utc_time( time_string, sizeof(time_string) );
			len = gdt_strlink( dest, len, http_date, gdt_strlen(http_date), dest_size );
			len = gdt_strlink( dest, len, time_string, gdt_strlen(time_string), dest_size );
			len = gdt_strlink( dest, len, "\r\n", 2, dest_size );
			break;
		case 304:
			len = gdt_strlink( dest, len, HTTP_NOT_MODIFIED, gdt_strlen(HTTP_NOT_MODIFIED), dest_size );
			break;
		case 400:
			len = gdt_strlink( dest, len, HTTP_BAD_REQUEST_ERROR, gdt_strlen(HTTP_BAD_REQUEST_ERROR), dest_size );
			break;
		case 500:
			len = gdt_strlink( dest, len, HTTP_INTERNAL_SERVER_ERROR, gdt_strlen(HTTP_INTERNAL_SERVER_ERROR), dest_size );
			break;
	}
	return len;
}

size_t gdt_http_add_cache_control(char* dest, size_t dest_size, size_t start, int max_age, GDT_FILE_INFO* info)
{
	size_t len = start;
	char* http_cache_control = "Cache-Control: ";
	char* http_cache_control_max_age = "max-age=";
	char age_buffer[20];
	gdt_itoa( max_age, age_buffer, sizeof(age_buffer) );
	len = gdt_strlink( dest, len, http_cache_control, gdt_strlen(http_cache_control), dest_size );
	len = gdt_strlink( dest, len, http_cache_control_max_age, gdt_strlen(http_cache_control_max_age), dest_size );
	len = gdt_strlink( dest, len, age_buffer, gdt_strlen(age_buffer), dest_size );
	len = gdt_strlink( dest, len, "\r\n", 2, dest_size );

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
	len = gdt_strlink( dest, len, http_last_modified, gdt_strlen(http_last_modified), dest_size );
	len = gdt_strlink( dest, len, time_buffer, gdt_strlen(time_buffer), dest_size );
	len = gdt_strlink( dest, len, "\r\n", 2, dest_size );
	return len;
}

size_t gdt_http_document_path(char* dest, size_t dest_size,char* document_root, char* default_file, char* path)
{
	size_t path_len = 0;
	path_len = gdt_strlink( dest, path_len, document_root, gdt_strlen(document_root), dest_size );
	path_len = gdt_strlink( dest, path_len, path, gdt_strlen(path), dest_size );
	if(!strcmp(path,"/")){
		path_len = gdt_strlink( dest, path_len, default_file, gdt_strlen(default_file), dest_size );
	}
	dest[path_len] = '\0';
	return path_len;
}
