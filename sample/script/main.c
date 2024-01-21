// qs_csv sample

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/time.h>

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_api.h"

int main(int argc, char *argv[])
{
    api_qs_init();

    QS_MEMORY_CONTEXT context;
    if(-1==api_qs_memory_alloc(&context, 1024 * 1024 * 16))
    {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

	struct timeval start_timeval, end_timeval;
	double sec_timeofday;
	gettimeofday( &start_timeval, NULL );

    for(int i=0;i<1;i++)
    {
        api_qs_memory_clean(&context);
        QS_SERVER_SCRIPT_CONTEXT script_context;
        if(-1==api_qs_script_read_file(&context, &script_context, "sample.qscript"))
        {
            printf("api_qs_script_read_file failed\n");
            return -1;
        }
        if(-1==api_qs_script_run(&script_context))
        {
            printf("api_qs_script_run failed\n");
            return -1;
        }
    }

	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	printf(  "## exec time = %lf[s]\n", sec_timeofday );

    api_qs_memory_free(&context);

    return 0;
}
