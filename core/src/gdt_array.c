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

#include "gdt_array.h"

int32_t gdt_create_array( GDT_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t tmpmunit;
	int i;
	if( -1 == ( tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY ), MEMORY_TYPE_DEFAULT ) ) ){
		return -1;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, tmpmunit );
	parray->max_size = allocsize;
	if( buffer_size != 0 && buffer_size < NUMERIC_BUFFER_SIZE ){
		buffer_size = NUMERIC_BUFFER_SIZE;
	}
	parray->buffer_size = buffer_size;
	parray->len = 0;
	if( -1 == ( parray->munit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY_ELEMENT ) * allocsize, MEMORY_TYPE_DEFAULT ) ) ){
		gdt_free_memory_unit( _ppool, &tmpmunit );
		return -1;
	}
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	for( i = 0; i < allocsize; i++ ){
		(elm+i)->id = 0;
		(elm+i)->munit = -1;//gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT );
		if( buffer_size > 0 ){
			if( -1 == ( (elm+i)->buf_munit = gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT ) ) ){
				return -1;
			}
		}
	}
	return tmpmunit;
}

int32_t gdt_create_array_buffer( GDT_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t tmpmunit;
	int i;
	if( -1 == ( tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY ), MEMORY_TYPE_DEFAULT ) ) ){
		return -1;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, tmpmunit );
	parray->max_size = allocsize;
	if( buffer_size != 0 && buffer_size < NUMERIC_BUFFER_SIZE ){
		buffer_size = NUMERIC_BUFFER_SIZE;
	}
	parray->buffer_size = buffer_size;
	parray->len = 0;
	if( -1 == ( parray->munit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY_ELEMENT ) * allocsize, MEMORY_TYPE_DEFAULT ) ) ){
		gdt_free_memory_unit( _ppool, &tmpmunit );
		return -1;
	}
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	for( i = 0; i < allocsize; i++ ){
		(elm+i)->id = ELEMENT_LITERAL_STR;
		if( -1 == ( (elm+i)->munit = gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT ) ) ){
			return -1;
		}
		parray->len++;
		char* pbuf = (char*)GDT_POINTER(_ppool,(elm+i)->munit);
		*pbuf = '\0';
	}
	return tmpmunit;
}

void gdt_reset_array( GDT_MEMORY_POOL* _ppool, int32_t munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int i;
	if( -1 == ( munit = gdt_resize_array( _ppool, munit ) ) ){
		return;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
	parray->max_size = parray->max_size;
	parray->len = 0;
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	for( i = 0; i < parray->len; i++ ){
		(elm+i)->id = 0;
		//(*(char*)GDT_POINTER(_ppool,(elm+i)->munit)) = '\0';
	}
}

int32_t gdt_resize_array( GDT_MEMORY_POOL* _ppool, int32_t munit )
{
	if( -1 == munit ){
		if( -1 == ( munit = gdt_create_array( _ppool, 8, NUMERIC_BUFFER_SIZE ) ) ){
			printf("gdt_resize_array parray->len >= parray->max_size\n");
			return munit;
		}
	}
	else{
		GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
		if( parray->len >= parray->max_size )
		{
			int i;
			size_t allocsize = parray->max_size;
			GDT_ARRAY_ELEMENT* elm;
			int32_t tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY_ELEMENT ) * ( parray->max_size + allocsize ), MEMORY_TYPE_DEFAULT );
			if( -1 == tmpmunit ){
				printf("gdt_resize_array error. %d\n",(int)(parray->max_size + allocsize));
				return munit;
			}
			memcpy( 
				  ( GDT_ARRAY_ELEMENT* )GDT_POINTER( _ppool, tmpmunit )
				, ( GDT_ARRAY_ELEMENT* )GDT_POINTER( _ppool, parray->munit )
				, sizeof( GDT_ARRAY_ELEMENT )*parray->max_size
			);
			gdt_free_memory_unit( _ppool, &parray->munit );
			parray->munit = tmpmunit;
			elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
			for( i = parray->max_size; i < parray->max_size + allocsize; i++ ){
				(elm+i)->id = 0;
				(elm+i)->munit = -1;
				if( parray->buffer_size > 0 ){
					if( -1 == ( (elm+i)->buf_munit = gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT ) ) ){
						printf("gdt_resize_array init error.\n");
						munit = -1;
						return munit;
					}
				}
				//(elm+1)->tmp_munit = -1;//gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT );
			}
			parray->max_size += allocsize;
		}
	}
	return munit;
}

int32_t gdt_next_push_munit( GDT_MEMORY_POOL* _ppool, int32_t munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t tmpmunit = -1;
	if( -1 == ( munit = gdt_resize_array( _ppool, munit ) ) ){
		return tmpmunit;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		printf("[debug] parray->len >= parray->max_size\n");
		return tmpmunit;
	}
	tmpmunit = elm[parray->len].munit;
	return tmpmunit;
}

int32_t gdt_array_push( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, int id, int32_t munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t freemunit = -1;
	if( -1 == ( (*pmunit) = gdt_resize_array( _ppool, (*pmunit) ) ) ){
		printf("gdt_resize_array error \n");
		return freemunit;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, (*pmunit) );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		printf("[debug] parray->len >= parray->max_size\n");
		return freemunit;
	}
	elm[parray->len].id = id;
	if( munit != elm[parray->len].munit && elm[parray->len].munit > 0 )
	{
		freemunit = elm[parray->len].munit;
	}
	elm[parray->len].munit= munit;
	parray->len++;
	return freemunit;
}

int32_t gdt_array_push_integer( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, int32_t value )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	(*pmunit) = gdt_resize_array( _ppool, (*pmunit) );
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, (*pmunit) );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return GDT_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_NUM;
	elm[parray->len].munit = elm[parray->len].buf_munit;
	gdt_itoa( value, (char*)GDT_POINTER(_ppool,elm[parray->len].munit), gdt_usize(_ppool,elm[parray->len].munit) );
	(*(int32_t*)(GDT_POINTER(_ppool,elm[parray->len].munit)+gdt_usize(_ppool,elm[parray->len].munit)-sizeof(int32_t))) = value;
	parray->len++;
	return GDT_SYSTEM_OK;
}

int32_t gdt_array_push_string( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, const char* value )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	(*pmunit) = gdt_resize_array( _ppool, (*pmunit) );
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, (*pmunit) );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return GDT_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_STR;
	if( elm[parray->len].munit == -1 ){
		if( -1 == ( elm[parray->len].munit = gdt_create_munit( _ppool, strlen(value)+1, MEMORY_TYPE_DEFAULT ) ) ){
			return GDT_SYSTEM_ERROR;
		}
	}
	else{
		if( gdt_usize(_ppool,elm[parray->len].munit) <= strlen(value)+1 ){
			if( -1 == ( elm[parray->len].munit = gdt_create_munit( _ppool, strlen(value)+1, MEMORY_TYPE_DEFAULT ) ) ){
				return GDT_SYSTEM_ERROR;
			}
		}
	}
	memcpy( (char*)GDT_POINTER(_ppool,elm[parray->len].munit),value,strlen(value));
	*((char*)GDT_POINTER(_ppool,elm[parray->len].munit)+(strlen(value))) = '\0';
	parray->len++;
	return GDT_SYSTEM_OK;
}

int32_t gdt_array_push_empty_string( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, size_t size )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	(*pmunit) = gdt_resize_array( _ppool, (*pmunit) );
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, (*pmunit) );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return GDT_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_STR;
	if( elm[parray->len].munit == -1 )
	{
		if( -1 == ( elm[parray->len].munit = gdt_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT ) ) ){
			return GDT_SYSTEM_ERROR;
		}
	}
	else{
		if( gdt_usize(_ppool,elm[parray->len].munit) <= size ){
			if( -1 == ( elm[parray->len].munit = gdt_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT ) ) ){
				return GDT_SYSTEM_ERROR;
			}
		}
	}
	*((char*)GDT_POINTER(_ppool,elm[parray->len].munit)) = '\0';
	parray->len++;
	return GDT_SYSTEM_OK;
}

GDT_ARRAY_ELEMENT* gdt_array_pop( GDT_MEMORY_POOL* _ppool, int32_t arraymunit )
{
	GDT_ARRAY_ELEMENT* retelm = NULL;
	if( -1 == arraymunit ){
		return retelm;
	}
	GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, arraymunit );
	if( parray == NULL || parray->len <= 0 ){
		return retelm;
	}
	retelm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit )+( parray->len-1 );
	parray->len--;
	return retelm;
}

GDT_ARRAY_ELEMENT* gdt_array_get( GDT_MEMORY_POOL* _ppool, int32_t arraymunit, int index )
{
	GDT_ARRAY_ELEMENT* retelm = NULL;
	if( arraymunit == -1 ){
		return retelm;
	}
	GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, arraymunit );
	if( parray == NULL || parray->len <= index ){
		return retelm;
	}
	retelm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit )+( index );
	return retelm;
}

GDT_ARRAY_ELEMENT* gdt_array_foreach( GDT_MEMORY_POOL* _ppool, int32_t array_munit, size_t* psize )
{
	GDT_ARRAY_ELEMENT* retelm = NULL;
	if( array_munit == -1 ){
		return retelm;
	}
	GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, array_munit );
	if( parray == NULL || parray->len <= (*psize) ){
		return retelm;
	}
	retelm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit )+( *psize );
	*psize = *psize + 1;
	return retelm;
}

int32_t gdt_array_length( GDT_MEMORY_POOL* _ppool, int32_t arraymunit )
{
	if( arraymunit == -1 ){
		return -1;
	}
	GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, arraymunit );
	if( parray == NULL ){
		return -1;
	}
	return parray->len;
}

int32_t gdt_opendir( GDT_MEMORY_POOL* _ppool, const char* path )
{
	char tmppath[MAXPATHLEN];
	char* pathstart = tmppath;
	memcpy( pathstart, path, strlen(path) );
	pathstart += strlen(path);
	*pathstart = '\0';
	struct stat st;
#ifdef __WINDOWS__
	WIN32_FIND_DATA fd;
	HANDLE handle;
	char tmpstr[MAXPATHLEN];
	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, path, strlen(path));
	*(tmpstr + strlen(path)) = '*';
	if (INVALID_HANDLE_VALUE == (handle = FindFirstFileEx(tmpstr, FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0)))
	{
		printf("error : FindFirstFileEx %s\n",tmpstr);
		return -1;
	}
#else
	DIR *dir;
	struct dirent *dent;
	if (NULL == (dir = opendir(path)))
	{
		printf("error : opendir\n");
		return -1;
	}
	dent = readdir(dir);
#endif
	int32_t path_array_munit = gdt_create_array( _ppool, 128, 0 );
	do{
#ifdef __WINDOWS__
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (fd.cFileName[0] == '.') {
				if (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.') {
					continue;
				}
			}
		}
		memcpy(pathstart, fd.cFileName, strlen(fd.cFileName));
		*(pathstart + strlen(fd.cFileName)) = '\0';
		if( stat( tmppath, &st ) < 0 )
#else
		if (dent->d_name[0] == '.') {
			if (dent->d_name[1] == '\0' || dent->d_name[1] == '.') {
				continue;
			}
		}
		memcpy(pathstart, dent->d_name, strlen(dent->d_name));
		*(pathstart + strlen(dent->d_name)) = '\0';
		if( lstat( tmppath, &st ) < 0 )
#endif
		{
#ifdef __WINDOWS__
			FindClose(handle);
#else
			closedir(dir);
#endif
			printf("error : lstat %s\n",tmppath);
			return -1;
		}
#ifdef __WINDOWS__
		// directory
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			*(pathstart + strlen(fd.cFileName)) = '/';
			*(pathstart + strlen(fd.cFileName) + 1) = '\0';
#else
		// directory
		if (S_ISDIR(st.st_mode)) {
			*(pathstart + strlen(dent->d_name)) = '/';
			*(pathstart + strlen(dent->d_name) + 1) = '\0';
#endif
			int32_t tmp_array = gdt_opendir( _ppool, tmppath );
			if( -1 != tmp_array )
			{
				GDT_ARRAY* parray;
				GDT_ARRAY_ELEMENT* elm;
				int i;
				parray = (GDT_ARRAY*)GDT_POINTER( _ppool, tmp_array );
				elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
				for( i = 0; i < parray->len; i++ ){
					gdt_array_push(_ppool,&path_array_munit,ELEMENT_LITERAL_STR,(elm+i)->munit);
				}
			}
		}
#ifdef __WINDOWS__
		else{
#else
		// file
		else if (S_ISREG(st.st_mode)) {
#endif
			gdt_array_push_string(_ppool, &path_array_munit, tmppath);
		}
#ifdef __WINDOWS__
		} while (FindNextFile(handle, &fd));
		FindClose(handle);
#else
		} while ((dent = readdir(dir)) != NULL);
		closedir(dir);
#endif
	return path_array_munit;
}

void gdt_array_dump( GDT_MEMORY_POOL* _ppool, int32_t munit, int index )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int i,k;
	if( -1 == munit ){
		return;
	}
	//if(1){for( k=0;k<index;k++ ){ printf("  ");}}
	printf("[\n");
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	for( i = 0; i < parray->len; i++ )
	{
		if( (elm+i)->id == ELEMENT_LITERAL_NUM ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  "); }
			printf("%s",(char*)GDT_POINTER(_ppool,(elm+i)->munit));
		}
		if( (elm+i)->id == ELEMENT_LITERAL_STR ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  "); }
			printf("\"%s\"",(char*)GDT_POINTER(_ppool,(elm+i)->munit));
		}
		else if( (elm+i)->id == ELEMENT_ARRAY ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  ");}
			gdt_array_dump( _ppool, (elm+i)->munit, index+1 );
		}
		else if( (elm+i)->id==ELEMENT_HASH){
			if( i > 0 ){ printf(",\n"); }
			gdt_dump_hash( _ppool, (elm+i)->munit, index+1 );
		}
	}
	printf("\n");
	for( k=0;k<index;k++ ){ printf("  "); }
	printf("]");
}
