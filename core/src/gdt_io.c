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

#include "gdt_io.h"

int gdt_fget_info( char* file_name, GDT_FILE_INFO* info )
{
	int error_code = GDT_SYSTEM_OK;
	struct stat st;
	FILE* f;
	do{
		if( info == NULL ){
			printf( "[gdt_fget_info] GDT_FILE_INFO is null\n" );
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
		
#ifdef __WINDOWS__
		if (0 != fopen_s(&f, file_name, "r"))
#else
		if (!(f = fopen(file_name, "r")))
#endif
		{
			//printf( "[gdt_fget_info] fopen error = %s\n", file_name );
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
		
#ifdef __WINDOWS__
		if (stat(file_name, &st) < 0)
#else
		if (lstat(file_name, &st) < 0)
#endif
		{
			printf("[gdt_lstate]lstat error\n");
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
		info->size = st.st_size;
		fclose( f );
	}while( false );
	return error_code;
}

int gdt_fputchar( FILE* f )
{
	int error_code = 0;
	char c;
	do{
		if( f == NULL || !f )
		{
			printf( "[gdt_fputchar] FILE get error\n" );
			break;
		}
		while( ( c = fgetc( f ) ) != EOF )
		{
			if( putchar( c ) < 0 ){
				exit( 1 );
			}
		}
	}while( false );
	return error_code;
}

int gdt_fputchar_line( FILE* f, uint32_t start, uint32_t line )
{
	int error_code = 0;
	int lineCount = 0;
	int isR = 0;
	int isFirst = 1;
	int finishLine = start + line;
	char c;
	do{
		if( f == NULL || !f )
		{
			printf( "[gdt_fputchar_line] FILE get error\n" );
			break;
		}
		while( ( c = fgetc( f ) ) != EOF )
		{
			if( lineCount >= start )
			{
				if( lineCount > 0 && isFirst && c == '\n' ){
					isFirst = 0;
					continue;
				}
				if( putchar( c ) < 0 ){
					exit( 1 );
				}
				isFirst = 0;
			}
			if( c == '\r' ){
				isR = 1;
				++lineCount;
			}
			else if( c == '\n' )
			{
				if( !isR ){
					++lineCount;
				}
				isR = 0;
			}
			else{
				isR = 0;
			}
			if( lineCount > finishLine ){
				break;
			}
		}
	}while( false );
	return error_code;
}

int gdt_fout( char* file_name )
{
	int error_code = 0;
	FILE* f;
	do{
#ifdef __WINDOWS__
		if (0 != fopen_s(&f, file_name, "r"))
#else
		if (!(f = fopen(file_name, "r")))
#endif
		{
			printf( "[gdt_fout] fopen error\n" );
			break;
		}
		gdt_fputchar( f );
		fclose( f );
	}while( false );
	return error_code;
}

int gdt_fout_line( char* file_name, uint32_t start, uint32_t line  )
{
	int error_code = 0;
	FILE* f;
	do{
#ifdef __WINDOWS__
		if (0 != fopen_s(&f, file_name, "r"))
#else
		if (!(f = fopen(file_name, "r")))
#endif
		{
			printf( "[gdt_fout] fopen error\n" );
			break;
		}
		gdt_fputchar_line( f, start, line );
		fclose( f );
	}while( false );
	return error_code;
}

size_t gdt_fread( char* file_name, char* dest, size_t size )
{
	size_t rsize = 0;
	struct stat st;
	FILE* f;
	int8_t c;
	char* ps = dest;
	do{
		if( dest == NULL ){
			printf( "[gdt_fread] output pointer is null : %s\n", file_name );
			break;
		}

#ifdef __WINDOWS__
		if (0 != fopen_s(&f, file_name, "r"))
#else
		if (!(f = fopen(file_name, "r")))
#endif
		{
			printf( "[gdt_fread] fopen error = %s\n", file_name );
			break;
		}
		
#ifdef __WINDOWS__
		if (stat(file_name, &st) < 0)
#else
		if (lstat(file_name, &st) < 0)
#endif
		{
			printf("[gdt_lstate]lstat error\n");
			fclose( f );
			break;
		}

		if (st.st_size >= size) {
			printf("[gdt_fread_bin] buffer size over %zd , %zd\n", st.st_size, size);
			fclose( f );
			break;
		}
		
		while( ( c = fgetc( f ) ) != EOF )
		{
			*( ps++ ) = c;
		}
		*ps = '\0';
		rsize = st.st_size;
		fclose( f );
	}while( false );
	return rsize;
}

size_t gdt_fread_bin( char* file_name, char* dest, size_t size )
{
	size_t retsize = 0;
	struct stat st;
	FILE* f;
	size_t rsize = 0;
	do{
		if( dest == NULL ){
			printf( "[gdt_fread_bin] output pointer is null : %s\n", file_name );
			break;
		}
#ifdef __WINDOWS__
		if (0 != fopen_s(&f, file_name, "rb"))
#else
		if (!(f = fopen(file_name, "rb")))
#endif
		{
			printf( "[gdt_fread_bin] fopen error = %s\n", file_name );
			break;
		}

#ifdef __WINDOWS__
		if (stat(file_name, &st) < 0)
#else
		if (lstat(file_name, &st) < 0)
#endif
		{
			printf("[gdt_fread_bin]lstat error\n");
			fclose( f );
			break;
		}
		retsize = st.st_size;

		if( retsize > size ){
			printf( "[gdt_fread_bin] buffer size over %zd , %zd\n", retsize , size );
			retsize = size;
		}
		
		rsize = fread( dest, sizeof(char), size, f );
		if( rsize > size ){
			printf("fread error %d, %d\n", (int)size, (int)rsize);
			retsize = 0;
		}
		fclose( f );
	}while( false );
	return retsize;
}

size_t gdt_fread_bin_range( char* file_name, char* dest, size_t pos, size_t size )
{
	size_t retsize = 0;
	struct stat st;
	FILE* f;
	size_t rsize = 0;
	do{
#ifdef __WINDOWS__
		if (0 != fopen_s(&f, file_name, "rb"))
#else
		if (!(f = fopen(file_name, "rb")))
#endif
		{
			printf( "[gdt_fread_bin] fopen error = %s\n", file_name );
			break;
		}

#ifdef __WINDOWS__
		if (stat(file_name, &st) < 0)
#else
		if (lstat(file_name, &st) < 0)
#endif
		{
			printf("[gdt_fread_bin]lstat error\n");
			fclose( f );
			break;
		}
		retsize = st.st_size;

		if( (fseek(f, pos, SEEK_SET)) != 0 )
		{
			printf( "[gdt_fread_bin] fsetpos error\n" );
			fclose( f );
			break;
		}
		
		rsize = fread( dest, sizeof(char), size, f );
		if( rsize != size ){
			printf("fread error %d, %d\n", (int)size, (int)rsize);
			retsize = 0;
		}
		retsize = rsize;
		fclose( f );
	}while( false );
	return retsize;
}

int gdt_fwrite( char* file_name, char* out, size_t size )
{
	int error_code = 0;
	char* p;
	FILE *fp;
	do{
		p = out;
#ifdef __WINDOWS__
		if (0 != fopen_s(&fp, file_name, "w"))
#else
		if (!(fp = fopen(file_name, "w")))
#endif
		{
			error_code = -1;
			break;
		}
		while( *p != '\0' && ( p - out ) < size ){ fputc( *(p++), fp ); };
		fclose(fp);
	}while( false );
	return error_code;
}

int gdt_fwrite_a( char* file_name, char* out, size_t size )
{
	int error_code = 0;
	char* p;
	FILE *fp;
	do{
		p = out;
#ifdef __WINDOWS__
		if (0 != fopen_s(&fp, file_name, "a"))
#else
		if (!(fp = fopen(file_name, "a")))
#endif
		{
			printf( "gdt_fwrite_a : fopen error.\n" );
			error_code = -1;
			break;
		}
		while( *p != '\0' && ( p - out ) < size ){
			//printf("c : %d\n", *p);
			fputc( *(p++), fp );
		}
		fputc( '\n', fp );
		fclose(fp);
	}while( false );
	return error_code;
}

int gdt_fwrite_bin( char* file_name, char* out, size_t size )
{
	int error_code = 0;
	do{
		FILE *fp;
#ifdef __WINDOWS__
		if (0 != fopen_s(&fp, file_name, "wb"))
#else
		if (!(fp = fopen(file_name, "wb")))
#endif
		{
			printf( "gdt_fwrite_a : fopen error.\n" );
			error_code = -1;
			break;
		}
		error_code = ( int )fwrite(( void * )out , sizeof( char ) , size , fp );
		fclose(fp);
	}while( false );
	return error_code;
}

int gdt_fwrite_bin_a( char* file_name, char* out, size_t size )
{
	int error_code = 0;
	do{
		FILE *fp;
#ifdef __WINDOWS__
		if (0 != fopen_s(&fp, file_name, "ab"))
#else
		if (!(fp = fopen(file_name, "ab")))
#endif
		{
			printf( "gdt_fwrite_a : fopen error.\n" );
			error_code = -1;
			break;
		}
		error_code = ( int )fwrite(( void * )out , sizeof( char ) , size , fp );
		fclose(fp);
	}while( false );
	return error_code;
}

int gdt_frename( const char* old_name, const char* new_name )
{
	int error_code = 0;
	do{
		if( rename( old_name, new_name ) < 0 ){
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
	}while( false );
	return error_code;
}

int gdt_unlink( const char* file_name )
{
	int error_code = 0;
	do{
#ifdef __WINDOWS__
		if( remove( file_name ) < 0 )
#else
		if( unlink( file_name ) < 0 )
#endif
		{
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
	}while( false );
	return error_code;
}

int32_t gdt_lstate( GDT_MEMORY_POOL* _ppool, const char* path )
{
	int32_t status_munit = -1;
	struct stat* ps;
	do{
		status_munit = gdt_create_munit( _ppool, sizeof(struct stat), MEMORY_TYPE_DEFAULT );
		ps = (struct stat*)gdt_upointer( _ppool, status_munit );
		if( ps == NULL ){
			printf( "[gdt_lstate]gdt_upointer error\n" );
			status_munit = -1;
			break;
		}
#ifdef __WINDOWS__
		if( stat( path, ps ) < 0 )
#else
		if( lstat( path, ps ) < 0 )
#endif
		{
			printf( "[gdt_lstate]lstat error\n" );
			status_munit = -1;
			break;
		}
	}while( false );
	return status_munit;
}

void gdt_lstateout( GDT_MEMORY_POOL* _ppool, const char* path )
{
	struct stat* ps;
	int32_t mulstate = gdt_lstate( _ppool, path );
#ifdef __WINDOWS__
	char bufa[32];
	char bufm[32];
	char bufc[32];
#endif
	if( mulstate >= 0 ){
		ps = (struct stat*)gdt_upointer( _ppool, mulstate );
#ifdef __WINDOWS__
		printf(  "type\t(%d)\n", ps->st_mode );
#else
		printf(  "type\t%o (%s)\n", ( ps->st_mode & S_IFMT ), gdt_filetype2char( ps->st_mode ) );
		printf(  "blksize\t%lu\n", (unsigned long)ps->st_blksize );
		printf(  "blocks\t%lu\n", (unsigned long)ps->st_blocks );
#endif
		printf(  "mode\t%o\n", ( ps->st_mode & S_IFMT ) );
		printf(  "dev\t%llu\n", (unsigned long long)ps->st_dev );
		printf(  "ino\t%lu\n", (unsigned long)ps->st_ino );
		printf(  "rdev\t%llu\n", (unsigned long long)ps->st_rdev );
		printf(  "nlink\t%d\n", (int)ps->st_nlink );
		printf(  "uid\t%d\n", (int)ps->st_uid );
		printf(  "gid\t%d\n", (int)ps->st_gid );
		printf(  "size\t%lld\n", ps->st_size );
#ifdef __WINDOWS__
		ctime_s(bufa, sizeof(bufa), &ps->st_atime);
		ctime_s(bufm, sizeof(bufm), &ps->st_mtime);
		ctime_s(bufc, sizeof(bufc), &ps->st_ctime);
		printf("atime\t%s", bufa);
		printf("mtime\t%s", bufm);
		printf("ctime\t%s", bufc);
#else
		printf("atime\t%s", ctime(&ps->st_atime));
		printf("mtime\t%s", ctime(&ps->st_mtime));
		printf("ctime\t%s", ctime(&ps->st_ctime));
#endif

	}
}

#ifdef __WINDOWS__

#else
/*
 * ユーザー権限の変更( シンボリックリンクの場合はシンボリックリンク先のファイル権限を変更 )
 * @param file_name ファイル名
 * @param owner 権限を付与するユーザー( 変更したくない場合は-1 )
 * @param group 権限を付与するグループ( 変更したくない場合は-1 )
 */
int gdt_chown( char* file_name, uid_t owner, gid_t group )
{
	int retcode = 0;
	retcode = chown( file_name, owner, group );
#ifdef __GDT_DEBUG__
	if( retcode == -1 ){
		printf( "chown faild\n" );
	}
#endif
	return retcode;
}

/*
 * ユーザー権限の変更( シンボリックリンクの場合はシンボリックリンクの権限を変更 )
 * @param file_name ファイル名
 * @param owner 権限を付与するユーザー( 変更したくない場合は-1 )
 * @param group 権限を付与するグループ( 変更したくない場合は-1 )
 */
int gdt_lchown( char* file_name, uid_t owner, gid_t group )
{
	int retcode = 0;
	if( (retcode = lchown( file_name, owner, group )) == -1 ){
		printf( "lchown error : %d\n", retcode );
	}
	return retcode;
}

/*
 * ファイルの最終アクセス、最終更新時間を変更する
 * @param file_name ファイル名
 * @param buf 更新時間( NULLの場合は現在時間 )
 */
int gdt_utime( char *file_name, struct utimbuf *buf )
{
	int retcode = 0;
	if( ( retcode = utime( file_name, buf ) ) == -1 ){
		printf( "utime error : %d\n", retcode );
	}
	return retcode;
}

// mode ( ex : "777" )
int gdt_chmod_char( char *file_name, char* mode )
{
	int retcode = 0;
	int i_mode;
	i_mode = strtol( mode, NULL, 8 );
	retcode = chmod( file_name, i_mode );
#ifdef __GDT_DEBUG__
	if( retcode == -1 ){
		printf( "chmod faild\n" );
	}
#endif
	return retcode;
}

int gdt_ls( char* path )
{
	int error_code = 0;
	do{
		DIR * d;
		struct dirent *ent;
		if( !( d = opendir( path ) ) ){
			printf( "[gdt_ls] opendir error\n" );
			break;
		}
		do{
			ent = readdir( d );
			if( ent )
			{
				if( ent->d_name[0] == '.' ){
					continue;
				}
				printf( "%s\n", ent->d_name );
			}
		}while( ent );
		closedir( d );
	}while( false );
	return error_code;
}

/*
 * mode ( ex:0777 )
 */
int gdt_mkdir( char* path, mode_t mode )
{
	int error_code = 0;
	do{
		if( mkdir( path, mode ) < 0 ){
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
	}while( false );
	return error_code;
}

int gdt_rmdir( char* path )
{
	int error_code = 0;
	do{
		if( rmdir( path ) < 0 ){
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
	}while( false );
	return error_code;
}

int gdt_link( const char* src, const char* dest )
{
	int error_code = 0;
	do{
		if( link( src, dest ) < 0 ){
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
	}while( false );
	return error_code;
}

int gdt_symlink( const char* src, const char* dest )
{
	int error_code = 0;
	do{
		if( symlink( src, dest ) < 0 ){
			error_code = GDT_SYSTEM_ERROR;
			break;
		}
	}while( false );
	return error_code;
}

int32_t gdt_readlink( GDT_MEMORY_POOL* _ppool, const char* path )
{
	int32_t muPathBuffer = -1;
	do{
		muPathBuffer = gdt_create_munit( _ppool, MAXPATHLEN, MEMORY_TYPE_DEFAULT );
		char* pbuf = (char*)gdt_upointer( _ppool, muPathBuffer );
		if( pbuf == NULL ){
			printf( "[gdt_readlink]gdt_upointer error\n" );
			muPathBuffer = -1;
			break;
		}
		if( readlink( path, pbuf, gdt_usize( _ppool, muPathBuffer ) ) < 0 )
		{
			printf( "[gdt_readlink]readlink error\n" );
			muPathBuffer = -1;
			break;
		}
	}while( false );
	return muPathBuffer;
}

char* gdt_filetype2char( mode_t mode )
{
	if( S_ISREG(mode) ) return "file";
	if( S_ISDIR(mode) ) return "directory";
	if( S_ISLNK(mode) ) return "symlink";
	if( S_ISSOCK(mode) ) return "socket";
	if( S_ISCHR(mode) ) return "chardev";
	if( S_ISBLK(mode) ) return "blockdev";
	if( S_ISFIFO(mode) ) return "fifo";
	return "unknown";
}
#endif
