#include "qs_datastore.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

static int failures;

static void check(const char* label, int condition)
{
    printf("[%s] %s\n", condition ? "PASS" : "FAIL", label);
    if (!condition) failures++;
}

static int child_check(const char* base_dir, const char* operation)
{
    QS_DB_CONTEXT context = {0};
    if (strcmp(operation, "drop") == 0) {
        return qs_db_drop_database(base_dir, "database") == -1 ? 0 : 1;
    }
    int result = qs_db_open_database(&context, base_dir, "database");
    if (result == 0 && qs_db_close_database(&context) != 0) return 1;
    return result == -1 ? 0 : 1;
}

static int run_child(const char* base_dir, const char* operation)
{
    char executable[MAX_PATH];
    char command[2048];
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    DWORD exit_code = 1;
    if (GetModuleFileNameA(NULL, executable, sizeof(executable)) == 0) return -1;
    if (snprintf(command, sizeof(command), "\"%s\" --child \"%s\" %s",
                 executable, base_dir, operation) < 0) return -1;
    startup.cb = sizeof(startup);
    if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) return -1;
    if (WaitForSingleObject(process.hProcess, 60000) == WAIT_OBJECT_0) {
        GetExitCodeProcess(process.hProcess, &exit_code);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return (int)exit_code;
}

int main(int argc, char** argv)
{
    if (argc == 4 && strcmp(argv[1], "--child") == 0) return child_check(argv[2], argv[3]);

    char temp_path[MAX_PATH];
    char base_dir[MAX_PATH];
    char value[128];
    QS_DB_CONTEXT context = {0};
    QS_DB_CONTEXT duplicate = {0};
    QS_DB_STORE store = {0};
    if (GetTempPathA(sizeof(temp_path), temp_path) == 0 ||
        GetTempFileNameA(temp_path, "qsd", 0, base_dir) == 0 ||
        !DeleteFileA(base_dir) || !CreateDirectoryA(base_dir, NULL)) return 1;

    check("create database", qs_db_create_database(base_dir, "database") == 0);
    check("open database", qs_db_open_database(&context, base_dir, "database") == 0);
    check("duplicate open rejected", qs_db_open_database(&duplicate, base_dir, "database") == -1);
    check("other process open rejected", run_child(base_dir, "open") == 0);
    check("other process drop rejected", run_child(base_dir, "drop") == 0);
    check("create table", qs_db_create_table(&context, "records", QS_DB_CAPACITY_B8MB) == 0);
    check("insert record", qs_db_record_insert(&context, "records", "item", "{\"value\":1}") == 0);
    check("open table", qs_db_open_table(&context, "records", QS_DB_CAPACITY_B8MB, &store) == 0);
    check("close deferred", qs_db_close_database(&context) == 1);
    qs_db_close_table(&store);
    check("close database", qs_db_close_database(&context) == 0);
    check("reopen database", qs_db_open_database(&context, base_dir, "database") == 0);
    check("record retained", qs_db_record_get(&context, "records", "item", value, sizeof(value)) == 0 &&
          strcmp(value, "{\"value\":1}") == 0);
    check("drop table", qs_db_drop_table(&context, "records") == 0);
    check("close reopened database", qs_db_close_database(&context) == 0);
    check("drop database", qs_db_drop_database(base_dir, "database") == 0);
    check("remove temporary directory", RemoveDirectoryA(base_dir) != 0);
    printf("fail: %d\n", failures);
    return failures == 0 ? 0 : 1;
}