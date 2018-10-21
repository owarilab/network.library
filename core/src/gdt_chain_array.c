#include "gdt_chain_array.h"

int32_t gdt_create_chain_array(GDT_MEMORY_POOL* memory,size_t chain_size, size_t data_size)
{
	chain_size = chain_size+1;
	int32_t header_size = (sizeof(int32_t)*8);
	int32_t block_size = data_size+(sizeof(int32_t)*2);
	int32_t chain_id = gdt_create_munit(memory,header_size+(block_size*chain_size),MEMORY_TYPE_DEFAULT);
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	memset(chain_root,0,gdt_usize(memory,chain_id));
	int32_t* pv = (int32_t*)chain_root;
	*pv = 0;                // start
	*(pv+1) = 0;            // tail
	*(pv+2) = chain_size;   // chain max size
	*(pv+3) = data_size;    // chain data size
	*(pv+4) = block_size;   // chain block size (data_size+(int32_t*2))
	*(pv+5) = header_size;  // header
	*(pv+6) = chain_size-1; // end of chain
	int i = 0;
	for(i=0;i<chain_size;i++){
		pv = (int32_t*)(chain_root+header_size+(block_size*i)+data_size);
		if(i==0){
			*(pv) = -1;
		}else{
			*(pv) = i-1;
		}
		if(i==chain_size-1){
			*(pv+1) = -1;
		}else{
			*(pv+1) = i+1;
		}
	}
	return chain_id;
}

void* gdt_add_chain(GDT_MEMORY_POOL* memory,int32_t chain_id)
{
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t *ptail = (((int32_t*)chain_root)+1);
	int32_t data_size = *(((int32_t*)chain_root)+3);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	int32_t header_size = *(((int32_t*)chain_root)+5);
	int32_t* info = (int32_t*)(chain_root+header_size+((*ptail)*block_size)+data_size);
	void* ret = (chain_root+header_size+((*ptail)*block_size));
	int32_t next = *(info+1);
	if(next==-1){
		return NULL;
	}
	*ptail = next;
	return ret;
}

int32_t gdt_add_chain_i(GDT_MEMORY_POOL* memory,int32_t chain_id)
{
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t *ptail = (((int32_t*)chain_root)+1);
	int32_t data_size = *(((int32_t*)chain_root)+3);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	int32_t header_size = *(((int32_t*)chain_root)+5);
	int32_t* info = (int32_t*)(chain_root+header_size+((*ptail)*block_size)+data_size);
	int32_t ret = *ptail;
	int32_t next = *(info+1);
	if(next==-1){
		return -1;
	}
	*ptail = next;
	return ret;
}

void* gdt_get_chain(GDT_MEMORY_POOL* memory,int32_t chain_id,void* current)
{
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t start = *(((int32_t*)chain_root));
	int32_t tail = *(((int32_t*)chain_root)+1);
	int32_t data_size = *(((int32_t*)chain_root)+3);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	int32_t header_size = *(((int32_t*)chain_root)+5);
	if(NULL==current){
		if(start==tail){
			return NULL;
		}
		return (void*)(chain_root+header_size+(start*block_size));
	}
	uint8_t* chain = (uint8_t*)current;
	int32_t* info = (int32_t*)(chain+data_size);
	int32_t next = *(info+1);
	if(next==tail){
		return NULL;
	}
	if(next==-1){
		return NULL;
	}
	return (void*)(chain_root+header_size+(next*block_size));
}

void* gdt_get_chain_i(GDT_MEMORY_POOL* memory,int32_t chain_id,int32_t offset)
{
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	int32_t header_size = *(((int32_t*)chain_root)+5);
	return (void*)(chain_root+header_size+(offset*block_size));
}

int32_t gdt_remove_chain(GDT_MEMORY_POOL* memory,int32_t chain_id,void* chain)
{
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	//int32_t tail = *(((int32_t*)chain_root)+1);
	int32_t data_size = *(((int32_t*)chain_root)+3);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	int32_t header_size = *(((int32_t*)chain_root)+5);
	int32_t end = *(((int32_t*)chain_root)+6);
	int32_t* info = (int32_t*)(((uint8_t*)chain)+data_size);
	int32_t prev = *(info);
	int32_t next = *(info+1);
	int32_t* next_info = (int32_t*)(chain_root+header_size+(next*block_size)+data_size);
	if(*(next_info+1)==-1){
		*(((int32_t*)chain_root)+1) = *(next_info);
		return 1;
	}
	int my_index = *(next_info);
	if(prev==-1){
		*(((int32_t*)chain_root)) = next;
		*(next_info) = -1;
	}else{
		int32_t* prev_info = (int32_t*)(chain_root+header_size+(prev*block_size)+data_size);
		*(prev_info+1) = next;
		*(next_info) = prev;
	}
	int32_t* end_info = (int32_t*)(chain_root+header_size+(end*block_size)+data_size);
	*(end_info+1) = my_index;
	*(info) = end;
	*(info+1) = -1;
	*(((int32_t*)chain_root)+6) = my_index;
	return 1;
}

void gdt_dump_chain_array(GDT_MEMORY_POOL* memory,int32_t chain_id)
{
	int32_t print_length=0;
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t start = *(((int32_t*)chain_root));
	int32_t tail = *(((int32_t*)chain_root)+1);
	int32_t chain_size = *(((int32_t*)chain_root)+2);
	int32_t data_size = *(((int32_t*)chain_root)+3);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	//int32_t header_size = *(((int32_t*)chain_root)+5);
	int32_t end = *(((int32_t*)chain_root)+6);
	void* current=NULL;
	while( NULL != (current=gdt_get_chain(memory,chain_id,current)) ){
		uint8_t* chain = (uint8_t*)current;
		int32_t* info = (int32_t*)(chain+data_size);
		printf("prev=%d, next=%d\n",*(info),*(info+1));
		print_length++;
	}
	printf("start=%d,tail=%d,size=%d,data_size=%d,block=%d,end=%d\n",start,tail,chain_size,data_size,block_size,end);
	printf("total : %d\n",print_length);
}

int32_t gdt_get_chain_start(GDT_MEMORY_POOL* memory,int32_t chain_id)
{
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t start = *(((int32_t*)chain_root));
	return start;
}

int32_t gdt_get_chain_length(GDT_MEMORY_POOL* memory,int32_t chain_id)
{
	int32_t print_length=0;
	uint8_t* chain_root = (uint8_t*)GDT_POINTER(memory,chain_id);
	int32_t start = *(((int32_t*)chain_root));
	int32_t tail = *(((int32_t*)chain_root)+1);
	int32_t chain_size = *(((int32_t*)chain_root)+2);
	int32_t data_size = *(((int32_t*)chain_root)+3);
	int32_t block_size = *(((int32_t*)chain_root)+4);
	//int32_t header_size = *(((int32_t*)chain_root)+5);
	int32_t end = *(((int32_t*)chain_root)+6);
	void* current=NULL;
	while( NULL != (current=gdt_get_chain(memory,chain_id,current)) ){
		print_length++;
	}
	return print_length;
}
