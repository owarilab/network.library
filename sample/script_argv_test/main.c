/*
 * Copyright (c) Katsuya Owari
 *
 * script_argv_test
 *   api_qs_script_set_argv_* / api_qs_script_get_parameter をテストする。
 *   スクリプト内で argv 変数にアクセスし、結果を読み戻す。
 *
 * 対象API:
 *   api_qs_script_read_file / api_qs_script_set_argv_string
 *   api_qs_script_set_argv_integer / api_qs_script_set_argv_object
 *   api_qs_script_run / api_qs_script_get_parameter
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "qs_api.h"

static int pass_count = 0;
static int fail_count = 0;

static void check(const char* name, int ok)
{
    if (ok) {
        pass_count++;
        printf("[PASS] %s\n", name);
    } else {
        fail_count++;
        printf("[FAIL] %s\n", name);
    }
}

static void check_str(const char* name, const char* actual, const char* expected)
{
    int ok = (actual != NULL) && (expected != NULL) && (strcmp(actual, expected) == 0);
    if (!ok) {
        printf("       expected: \"%s\"\n       actual:   \"%s\"\n",
               expected ? expected : "(null)", actual ? actual : "(null)");
    }
    check(name, ok);
}

int main(void)
{
    api_qs_init();
    QS_MEMORY_CONTEXT memory_context;
    api_qs_memory_alloc(&memory_context, 1024 * 1024 * 16);

    QS_SERVER_SCRIPT_CONTEXT script_context;
    int rc = api_qs_script_read_file(&memory_context, &script_context, "test.qscript");
    check("script_read_file returns 0", rc == 0);

    /* --- set argv string --- */
    rc = api_qs_script_set_argv_string(&script_context, "name", "testuser");
    check("set_argv_string returns 0", rc == 0);

    rc = api_qs_script_set_argv_string(&script_context, "greeting", "hello world");
    check("set_argv_string greeting returns 0", rc == 0);

    /* --- set argv integer --- */
    rc = api_qs_script_set_argv_integer(&script_context, "count", 42);
    check("set_argv_integer returns 0", rc == 0);

    /* --- set argv object --- */
    {
        QS_JSON_ELEMENT_OBJECT config;
        api_qs_object_create(&memory_context, &config);
        api_qs_object_push_integer(&config, "port", 8080);
        api_qs_object_push_string(&config, "host", "localhost");

        rc = api_qs_script_set_argv_object(&script_context, "config", &config);
        check("set_argv_object returns 0", rc == 0);
    }

    /* --- run script --- */
    rc = api_qs_script_run(&script_context);
    check("script_run returns 0", rc == 0);

    /* --- get parameter (result set by script) --- */
    char* result = api_qs_script_get_parameter(&script_context, "result");
    check_str("get_parameter result", result, "done:testuser:42");

    /* --- get parameter (argv string) --- */
    char* name = api_qs_script_get_parameter(&script_context, "name");
    check_str("get_parameter name", name, "testuser");

    /* --- get parameter (argv integer) --- */
    char* count = api_qs_script_get_parameter(&script_context, "count");
    check("get_parameter count not NULL", count != NULL);
    if (count != NULL) {
        check("get_parameter count is 42", atoi(count) == 42);
    }

    /* --- get parameter (missing) --- */
    char* missing = api_qs_script_get_parameter(&script_context, "no_such_param");
    check("get_parameter missing returns NULL", missing == NULL);

    api_qs_memory_free(&memory_context);

    printf("\n=== script_argv_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
