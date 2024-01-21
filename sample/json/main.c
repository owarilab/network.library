// qs_api sample
// Path: sample/api/main.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "qs_api.h"

int main()
{
    // memory context
    QS_MEMORY_CONTEXT memory_context;

    // initialize
    api_qs_init();

    // memory alloc
    api_qs_memory_alloc(&memory_context, 1024 * 1024 * 128);

    // root object
    QS_JSON_ELEMENT_OBJECT root_object;
    api_qs_object_create(&memory_context, &root_object);

    // root array
    QS_JSON_ELEMENT_ARRAY root_array;
    api_qs_array_create(&memory_context, &root_array);

    // create object loop 10

    for (int i = 0; i < 10; i++)
    {
        QS_JSON_ELEMENT_OBJECT object;
        api_qs_object_create(&memory_context, &object);
        api_qs_object_push_integer(&object, "id", i);
        api_qs_object_push_string(&object, "test2", "test2");
        api_qs_object_push_string(&object, "test3", "test3");
        api_qs_object_push_string(&object, "test4", "test4");
        api_qs_object_push_string(&object, "test5", "test5");
        api_qs_object_push_string(&object, "test6", "test6");
        api_qs_object_push_string(&object, "test7", "test7");
        api_qs_object_push_string(&object, "test8", "test8");
        api_qs_object_push_string(&object, "test9", "test9");
        api_qs_object_push_string(&object, "test10", "test10");

        for(int j = 0; j < 5; j++)
        {
            QS_JSON_ELEMENT_ARRAY array;
            api_qs_array_create(&memory_context, &array);
            api_qs_array_push_string(&array, "v1");
            api_qs_array_push_string(&array, "v2");

            // push array
            char name[10];
            sprintf(name, "array%d", j);
            api_qs_object_push_array(&object, name, &array);
        }

        // push object
        api_qs_array_push_object(&root_array, &object);
    }

    // create array loop 10
    for(int j = 0; j < 10; j++)
    {
        QS_JSON_ELEMENT_ARRAY array;
        api_qs_array_create(&memory_context, &array);
        api_qs_array_push_string(&array, "test2");
        api_qs_array_push_string(&array, "test3");
        api_qs_array_push_string(&array, "test4");
        api_qs_array_push_string(&array, "test5");
        api_qs_array_push_string(&array, "test6");
        api_qs_array_push_string(&array, "test7");
        api_qs_array_push_string(&array, "test8");
        api_qs_array_push_string(&array, "test9");
        api_qs_array_push_string(&array, "test10");

        // push array
        char name[10];
        sprintf(name, "array%d", j);
        api_qs_object_push_array(&root_object, name, &array);
    }

    // json encode
    char* json = api_qs_json_encode_array(&root_array, 1024 * 1024 * 1);
    printf("%s\n", json);

    // json encode object
    char* json2 = api_qs_json_encode_object(&root_object, 1024 * 1024 * 1);
    printf("%s\n", json2);

    {
        clock_t start, end;
        double cpu_time_used;
        int i;

        start = clock();

        int loop = 30000;
        for(i = 0; i < loop; i++) {
            // keys
            QS_JSON_ELEMENT_ARRAY keys;
            if(-1 == api_qs_object_get_keys(&root_object, &keys))
            {
                printf("error\n");
                return -1;
            }

            // show key value
            for(int i = 0; i < api_qs_array_get_length(&keys); i++)
            {
                char* key = api_qs_array_get_string(&keys, i);
                QS_JSON_ELEMENT_ARRAY array;
                api_qs_object_get_array(&root_object, key, &array);
                for(int j = 0; j < api_qs_array_get_length(&array); j++)
                {
                    char* value = api_qs_array_get_string(&array, j);
                    //printf("k : %s , value : %s\n", key, value);
                }
            }
        }
        end = clock();

        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

        printf("The code was executed %d times in %f seconds\n", loop, cpu_time_used);
    }

    // keys
    QS_JSON_ELEMENT_ARRAY keys;
    if(-1 == api_qs_object_get_keys(&root_object, &keys))
    {
        printf("error\n");
        return -1;
    }

    // show key value
    for(int i = 0; i < api_qs_array_get_length(&keys); i++)
    {
        char* key = api_qs_array_get_string(&keys, i);
        QS_JSON_ELEMENT_ARRAY array;
        api_qs_object_get_array(&root_object, key, &array);
        for(int j = 0; j < api_qs_array_get_length(&array); j++)
        {
            char* value = api_qs_array_get_string(&array, j);
            //printf("k : %s , value : %s\n", key, value);
        }
    }

    // finalize
    api_qs_memory_free(&memory_context);

    return 0;
}