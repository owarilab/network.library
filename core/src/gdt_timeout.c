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

#include "gdt_timeout.h"

ssize_t gdt_recv_with_timeout( uint8_t mode, int32_t timeoutsec, GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag )
{
	buffer_size = buffer_size-1;
	if( timeoutsec <= 0 ){
		return recv( soc, buf, buffer_size, flag );
	}
	switch( mode )
	{
#ifdef __WINDOWS__
		case TIMEOUT_MODE_SELECT:
			return ( gdt_recv_timeout_by_select( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		case TIMEOUT_MODE_NONBLOCKING:
		case TIMEOUT_MODE_POOL:
		case TIMEOUT_MODE_EPOOL:
		case TIMEOUT_MODE_IOCTL:
		case TIMEOUT_MODE_SOCKET_OPTION:
			printf("not suported timeout mode");
		default:
			return recv( soc, buf, buffer_size, flag );
			break;
#else
		case TIMEOUT_MODE_NONBLOCKING:
			return ( gdt_recv_timeout_by_nonblocking( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		case TIMEOUT_MODE_SELECT:
			return ( gdt_recv_timeout_by_select( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		case TIMEOUT_MODE_POOL:
			return ( gdt_recv_timeout_by_poll( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		case TIMEOUT_MODE_EPOOL:
			return ( gdt_recv_timeout_by_epoll( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		case TIMEOUT_MODE_IOCTL:
			return ( gdt_recv_timeout_by_ioctl( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		case TIMEOUT_MODE_SOCKET_OPTION:
			return ( gdt_recv_timeout_by_setsockopt( soc, buf, buffer_size, flag, timeoutsec ) );
			break;
		default:
			return recv( soc, buf, buffer_size, flag );
			break;
#endif
	}
	return (-1);
}

ssize_t gdt_recv_timeout_by_select(GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag, int32_t timeoutsec )
{
	struct timeval timeout;
	fd_set mask;
	int width;
	uint8_t end;
	ssize_t len = 0;
	ssize_t rv = 0;
	FD_ZERO( &mask );
	FD_SET( soc, &mask );
	width = (int)(soc+1);
	timeout.tv_sec = timeoutsec;
	timeout.tv_usec = 0;
	do{
		end = 0;
		switch( select( width, &mask, NULL, NULL, &timeout ) )
		{
			case -1:
				if( errno != EINTR )
				{
					perror( "select" );
					rv = -1;
					end = 1;
				}
				break;
			case 0:
				rv = -1;
				end = 1;
				break;
			default:
				if( FD_ISSET( soc, &mask ) )
				{
					if( ( len = recv( soc, buf, buffer_size, flag ) ) == -1 )
					{
						perror( "recv" );
						rv = -1;
						end = 1;
					}
					else{
						rv = len;
						end = 1;
					}
				}
				break;
		}
	}while( end == 0 );
	return (rv);
}

#ifdef __WINDOWS__
#else
ssize_t gdt_recv_timeout_by_nonblocking(GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag, int32_t timeoutsec )
{
	uint8_t end;
	ssize_t len = 0;
	ssize_t rv = 0;
	time_t start_time;
	gdt_set_block( soc, 0 );
	start_time = time( NULL );
	do{
		end = 0;
		if( time( NULL ) - start_time > timeoutsec )
		{
			rv = -1;
			end = 1;
		}
		if( ( len = recv( soc, buf, buffer_size, flag ) ) == -1 )
		{
			if( errno == EAGAIN )
			{
				printf( "." );
				(void) usleep( 100000 );
			}
			else{
				perror( "recv" );
				rv = -1;
				end = 1;
			}
		}
		else{
			rv = len;
			end = 1;
		}
	}while( end == 0 );
	gdt_set_block( soc, 1 );
	return (rv);
}

ssize_t gdt_recv_timeout_by_poll(GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag, int32_t timeoutsec )
{
	struct pollfd targets[1];
	int nready;
	uint8_t end;
	ssize_t len = 0;
	ssize_t rv = 0;
	targets[0].fd = soc;
	targets[0].events = POLLIN;
	do{
		end = 0;
		switch( ( nready = poll( targets, 1, timeoutsec * 1000 ) ) )
		{
			case -1:
				if( errno != EINTR )
				{
					perror( "poll" );
					rv = -1;
					end = 1;
					
				}
				break;
			case 0:
				rv = -1;
				end = 1;
				break;
			default:
				if( targets[0].revents & ( POLLIN | POLLERR ) )
				{
					if( ( len = recv( soc, buf, buffer_size, flag ) ) == -1 )
					{
						perror( "recv" );
						rv = -1;
						end = 1;
					}
					else{
						rv = len;
						end = 1;
					}
				}
				break;
		}
	}while( end == 0 );
	return (rv);
}

ssize_t gdt_recv_timeout_by_epoll(GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag, int32_t timeoutsec )
{
#ifdef USE_EPOOL
	struct epoll_event ev, event;
	int nfds, epollfd;
	uint8_t end;
	ssize_t len, rv;
	if( ( epollfd = epoll_create( 1 ) ) == -1 )
	{
		perror( "epoll_create" );
		return (-1);
	}
	ev.data.fd = soc;
	ev.events = EPOLLIN;
	if( epoll_ctl( epollfd, EPOLL_CTL_ADD, soc, &ev ) == -1 )
	{
		perror( "epoll_ctl" );
		(void) close( epollfd );
		return (-1);
	}
	do{
		end = 0;
		switch( ( nfds = epoll_wait( epollfd, &event, 1, timeoutsec * 1000 ) ) )
		{
			case -1:
				if( errno != EINTR )
				{
					perror( "epoll" );
					rv = -1;
					end = 1;
					
				}
				break;
			case 0:
				rv = -1;
				end = 1;
				break;
			default:
				if( event.events & ( EPOLLIN | EPOLLERR ) )
				{
					if( ( len = recv( soc, buf, buffer_size, flag ) ) == -1 )
					{
						perror( "recv" );
						rv = -1;
						end = 1;
					}
					else{
						rv = len;
						end = 1;
					}
				}
				break;
		}
	}while( end == 0 );
	if( epoll_ctl( epollfd, EPOLL_CTL_DEL, soc, &ev ) == -1 )
	{
		perror( "epoll_ctl" );
	}
	(void) close( epollfd );
	return (rv);
#else
	printf( "epool disabled\n" );
	return -1;
#endif //#ifdef USE_EPOOL
}

ssize_t gdt_recv_timeout_by_ioctl(GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag, int32_t timeoutsec )
{
	uint8_t end;
	ssize_t len = 0;
	ssize_t rv = 0;
	ssize_t nread = 0;
	time_t start_time;
	start_time = time( NULL );
	do{
		if( time( NULL ) - start_time > timeoutsec )
		{
			rv = -1;
			end = 1;
		}
		else{
			if( ioctl( soc, FIONREAD, &nread ) == -1 )
			{
				perror( "ioctl" );
				rv = -1;
				end = 1;
			}
			if( nread <= 0 )
			{
				printf( "." );
				(void) usleep( 100000 );
			}
			else{
				if( ( len = recv( soc, buf, buffer_size, flag ) ) == -1 )
				{
					printf( "errno : %d\n", errno );
					perror( "recv" );
					rv = -1;
					end = 1;
				}
				else{
					rv = len;
					end = 1;
				}
			}
		}
	}while( end == 0 );
	return ( rv );
}

ssize_t gdt_recv_timeout_by_setsockopt(GDT_SOCKET_ID soc, char *buf, size_t buffer_size, int flag, int32_t timeoutsec )
{
	struct timeval tv;
	uint8_t end;
	ssize_t len;
	tv.tv_sec = timeoutsec;
	tv.tv_usec = 0;
	if( setsockopt( soc, SOL_SOCKET, SO_RCVTIMEO, ( char * )&tv, sizeof( tv ) ) == -1 )
	{
		perror( "setsockopt" );
		return (-1);
	}
	do{
		if( ( len = recv( soc, buf, buffer_size, flag ) ) == -1 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			if( errno == EAGAIN || errno == EWOULDBLOCK )
			{
				//printf( "timeout\n" );
			}
			else{
				perror( "recv" );
			}
			len = -1;
			end = 1;
		}
		else{
			end = 1;
		}
	}while( end == 0 );
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	if( setsockopt( soc, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof( tv ) ) == -1 )
	{
		perror( "setsockopt" );
	}
	return (len);
}
#endif

