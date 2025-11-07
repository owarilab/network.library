/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_csv.h"

int main(int argc, char *argv[])
{
    QS_MEMORY_POOL* memory = NULL;
	size_t size;
	if( (size = qs_initialize_memory_f64( &memory, 1024 * 1024 * 8) ) <= 0 ){
		return -1;
	}
    
    int32_t memid_csv = qs_csv_file_load(memory,"sample.csv");
    if(-1==memid_csv){
        return -1;
    }

    printf("qs_csv_build_csv:\n");
    char* csv = qs_csv_build_csv(memory,memid_csv,1024*1024*2);
    if(NULL==csv){
        return -1;
    }
    printf("%s\n",csv);
    
    printf("\n");
    printf("qs_csv_parse:\n");
    int32_t memid_csv2 = qs_csv_parse(memory,csv);
    if(-1==memid_csv2){
        return -1;
    }

    // add line
    int32_t memid_array = -1;
    if(-1==qs_array_push_string(memory,&memid_array,"1")){
        return -1;
    }
    if(-1==qs_array_push_string(memory,&memid_array,"2")){
        return -1;
    }
    if(-1==qs_array_push_string(memory,&memid_array,"3")){
        return -1;
    }
    qs_csv_add_line(memory,memid_csv2,memid_array);

    // add row
    qs_csv_add_row(memory,memid_csv2,0,"add row 1");
    qs_csv_add_row(memory,memid_csv2,1,"add row 2");
    qs_csv_add_row(memory,memid_csv2,2,"add row 3");
    qs_csv_add_row(memory,memid_csv2,10,"add row 4");
    qs_csv_add_row(memory,memid_csv2,7,"add row 5");
    qs_csv_add_row(memory,memid_csv2,7,"add row 6");
    
    int32_t i;
    for(i=0;i<qs_csv_get_line_length(memory,memid_csv2);i++){
        int32_t j;
        for(j=0;j<qs_csv_get_row_length(memory,memid_csv2,i);j++){
            printf("%s,",qs_csv_get_row(memory,memid_csv2,i,j));
        }
        printf("\n");
    }
    
    qs_free(memory);
    return 0;
}
