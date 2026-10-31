/*
 * Copyright (c) Katsuya Owari
 *
 * io_test
 *   qs_io (ファイルI/O) の低レベルAPIをテストする。
 *   /tmp 配下の一時ファイル/ディレクトリを使用する。
 *
 * 対象API:
 *   qs_finit / qs_fopen / qs_fwrite2 / qs_fclose / qs_fget_info
 *   qs_fread / qs_fread_range / qs_fread_bin / qs_fread_bin_range
 *   qs_fwrite / qs_fwrite_a / qs_fwrite_bin / qs_fwrite_bin_a
 *   qs_frename / qs_unlink / qs_mkdir / qs_rmdir / qs_lstate
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "qs_io.h"

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

#define TMP_DIR "/tmp/qs_io_test"
#define TMP_FILE (TMP_DIR "/test.txt")
#define TMP_FILE2 (TMP_DIR "/test2.txt")
#define TMP_BIN (TMP_DIR "/test.bin")
#define TMP_RENAMED (TMP_DIR "/renamed.txt")
#define TMP_SUBDIR (TMP_DIR "/subdir")

static void cleanup(void)
{
    unlink(TMP_FILE);
    unlink(TMP_FILE2);
    unlink(TMP_BIN);
    unlink(TMP_RENAMED);
    rmdir(TMP_SUBDIR);
    rmdir(TMP_DIR);
}

int main(void)
{
    cleanup();
    mkdir(TMP_DIR, 0755);

    /* --- qs_finit --- */
    {
        QS_FILE_INFO info;
        int rc = qs_finit(&info);
        check("finit returns OK", rc == QS_SYSTEM_OK);
        check("finit size is 0", info.size == 0);
        check("finit f is NULL", info.f == NULL);
    }

    /* --- qs_fwrite / qs_fread round trip --- */
    {
        const char* content = "hello io test\nline2\n";
        int rc = qs_fwrite((char*)TMP_FILE, (char*)content, strlen(content));
        check("fwrite returns 0", rc == 0);

        char buf[256] = {0};
        size_t rsize = qs_fread((char*)TMP_FILE, buf, sizeof(buf));
        check("fread returns file size", rsize == strlen(content));
        check_str("fread content matches", buf, content);
    }

    /* --- qs_fget_info --- */
    {
        QS_FILE_INFO info;
        qs_finit(&info);
        int rc = qs_fget_info((char*)TMP_FILE, &info);
        check("fget_info returns OK", rc == QS_SYSTEM_OK);
        check("fget_info size matches", info.size == strlen("hello io test\nline2\n"));
    }

    /* --- qs_fread_range --- */
    {
        char buf[64] = {0};
        /* 注: qs_fread_range は rsize < size-1 で停止するため、size-1 文字分を読み込む。
           pos=6, size=6 -> "io te" (5文字) */
        size_t rsize = qs_fread_range((char*)TMP_FILE, buf, 6, 6);
        check("fread_range returns size-1", rsize == 5);
        check_str("fread_range content", buf, "io te");
    }

    /* --- qs_fwrite_a (append) --- */
    {
        int rc = qs_fwrite_a((char*)TMP_FILE, (char*)"appended", 8);
        check("fwrite_a returns 0", rc == 0);
        /* 元の内容 + "appended\n" */
        char buf[256] = {0};
        qs_fread((char*)TMP_FILE, buf, sizeof(buf));
        check_str("fwrite_a appended with newline", buf, "hello io test\nline2\nappended\n");
    }

    /* --- qs_fopen / qs_fwrite2 / qs_fclose --- */
    {
        QS_FILE_INFO info;
        qs_finit(&info);
        int rc = qs_fopen((char*)TMP_FILE2, "w", &info);
        check("fopen w returns OK", rc == QS_SYSTEM_OK);
        check("fopen f is not NULL", info.f != NULL);

        rc = qs_fwrite2(&info, (char*)"via fwrite2", 11);
        check("fwrite2 returns OK", rc == QS_SYSTEM_OK);

        rc = qs_fclose(&info);
        check("fclose returns OK", rc == QS_SYSTEM_OK);
        check("fclose f is NULL", info.f == NULL);

        char buf[64] = {0};
        qs_fread((char*)TMP_FILE2, buf, sizeof(buf));
        check_str("fwrite2 content", buf, "via fwrite2");
    }

    /* --- qs_fopen non-existent file (r mode) --- */
    {
        QS_FILE_INFO info;
        qs_finit(&info);
        int rc = qs_fopen((char*)(TMP_DIR "/no_such_file.txt"), "r", &info);
        check("fopen non-existent returns ERROR", rc == QS_SYSTEM_ERROR);
    }

    /* --- qs_fwrite_bin / qs_fread_bin --- */
    {
        uint8_t data[] = {0x00, 0x01, 0x02, 0xFF, 0x10, 0x00, 0xAB};
        int rc = qs_fwrite_bin((char*)TMP_BIN, (char*)data, sizeof(data));
        check("fwrite_bin returns size", rc == (int)sizeof(data));

        uint8_t buf[64] = {0};
        size_t rsize = qs_fread_bin((char*)TMP_BIN, (char*)buf, sizeof(buf));
        check("fread_bin returns size", rsize == sizeof(data));
        check("fread_bin content matches", memcmp(buf, data, sizeof(data)) == 0);
    }

    /* --- qs_fread_bin_range --- */
    {
        uint8_t buf[16] = {0};
        /* pos=2, size=3 -> {0x02, 0xFF, 0x10} */
        size_t rsize = qs_fread_bin_range((char*)TMP_BIN, (char*)buf, 2, 3);
        check("fread_bin_range returns 3", rsize == 3);
        check("fread_bin_range content", buf[0]==0x02 && buf[1]==0xFF && buf[2]==0x10);
    }

    /* --- qs_fwrite_bin_a (binary append) --- */
    {
        uint8_t more[] = {0xCD, 0xEF};
        int rc = qs_fwrite_bin_a((char*)TMP_BIN, (char*)more, sizeof(more));
        check("fwrite_bin_a returns size", rc == (int)sizeof(more));

        uint8_t buf[64] = {0};
        size_t rsize = qs_fread_bin((char*)TMP_BIN, (char*)buf, sizeof(buf));
        check("fread_bin after append size", rsize == 9);
        check("fread_bin after append tail", buf[7]==0xCD && buf[8]==0xEF);
    }

    /* --- qs_frename --- */
    {
        int rc = qs_frename(TMP_FILE2, TMP_RENAMED);
        check("frename returns 0", rc == 0);
        QS_FILE_INFO info;
        qs_finit(&info);
        check("renamed file exists", qs_fget_info((char*)TMP_RENAMED, &info) == QS_SYSTEM_OK);
        check("old file gone", qs_fget_info((char*)TMP_FILE2, &info) == QS_SYSTEM_ERROR);
    }

    /* --- qs_mkdir / qs_rmdir --- */
    {
        int rc = qs_mkdir((char*)TMP_SUBDIR, 0755);
        check("mkdir returns 0", rc == 0);
        QS_FILE_INFO info;
        qs_finit(&info);
        check("subdir exists", qs_fget_info((char*)TMP_SUBDIR, &info) == QS_SYSTEM_OK);

        rc = qs_rmdir((char*)TMP_SUBDIR);
        check("rmdir returns 0", rc == 0);
        check("subdir gone", qs_fget_info((char*)TMP_SUBDIR, &info) == QS_SYSTEM_ERROR);
    }

    /* --- qs_lstate --- */
    {
        QS_MEMORY_POOL* memory = NULL;
        if (qs_initialize_memory_f64(&memory, 1024 * 1024) > 0) {
            int32_t munit = qs_lstate(memory, TMP_FILE);
            check("lstate returns valid munit", munit >= 0);
            if (munit >= 0) {
                struct stat* ps = (struct stat*)qs_upointer(memory, munit);
                check("lstate size matches", ps->st_size == strlen("hello io test\nline2\nappended\n"));
            }
            qs_free(memory);
        }
    }

    /* --- qs_unlink --- */
    {
        int rc = qs_unlink(TMP_FILE);
        check("unlink returns 0", rc == 0);
        QS_FILE_INFO info;
        qs_finit(&info);
        check("unlinked file gone", qs_fget_info((char*)TMP_FILE, &info) == QS_SYSTEM_ERROR);
    }

    cleanup();

    printf("\n=== io_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
