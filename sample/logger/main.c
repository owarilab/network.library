/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "qs_api.h"

int main(int argc, char *argv[])
{
    api_qs_init();

    QS_MEMORY_CONTEXT context;
    if(-1==api_qs_memory_alloc(&context, 1024 * 1024 * 4))
    {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

    // INFO 以上を出力するロガーを作成
    QS_LOGGER_CONTEXT logger;
    if(-1==api_qs_logger_create(&context, &logger, QS_LOG_LEVEL_INFO))
    {
        printf("api_qs_logger_create failed\n");
        return -1;
    }

    // ファイルを指定しない場合は stdout に出力される
    if(-1==api_qs_logger_open(&logger, "./logger_sample.log"))
    {
        printf("api_qs_logger_open failed\n");
        return -1;
    }

    api_qs_logger_debug(&logger, "debug is suppressed");
    api_qs_logger_info(&logger, "info count=%d name=%s", 42, "sample");
    api_qs_logger_warn(&logger, "warn: ratio=%.2f", 0.75);
    api_qs_logger_error(&logger, "error: code=%d", -1);

    // __FILE__ / __LINE__ 付き
    API_QS_LOG_ERROR(&logger, "macro error code=%d", -2);

    // 実行時にレベルを変更
    api_qs_logger_set_level(&logger, QS_LOG_LEVEL_DEBUG);
    api_qs_logger_debug(&logger, "debug is shown after set_level, level=%d", api_qs_logger_get_level(&logger));

    api_qs_logger_close(&logger);
    api_qs_logger_free(&logger);

    // 解放後は memid が -1 になり、書き込みは失敗を返す
    if(-1!=api_qs_logger_info(&logger, "must not be written"))
    {
        printf("unexpected: write after free succeeded\n");
    }

    api_qs_memory_free(&context);
    return 0;
}
