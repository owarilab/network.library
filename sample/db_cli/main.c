/*
 * Copyright (c) Katsuya Owari
 *
 * qs_db_cli
 *   qs_datastore（低レベル qs_db_* API）を使う mysql-cli 風のコマンドラインツール。
 *
 * 使い方:
 *   ./qs_db_cli [base_dir]                 対話モード（REPL）
 *   ./qs_db_cli [base_dir] -e "<command>"  ワンショットモード（複数 -e 可）
 *
 * コマンド:
 *   create database <name>
 *   drop database <name>
 *   show databases
 *   use <name>
 *   create table <name> [capacity <1|8|16|32|64|128|256|512|1024>]
 *   drop table <name>
 *   show tables
 *   schema define <table> <json>
 *   schema get <table>
 *   schema validate <table> <json>
 *   insert <table> <id> <json>
 *   get <table> <id>
 *   update <table> <id> <json>
 *   delete <table> <id>
 *   list <table> [filter_json] [index]
 *   index add <table> <field> <value> <id>
 *   index remove <table> <field> <value> <id>
 *   index rebuild <table> <fields_json>
 *   next id <table> <prefix>
 *   help
 *   exit / quit
 *
 * Build: make
 * Run:   ./qs_db_cli
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "qs_datastore.h"

#define CLI_MAX_LINE      65536
#define CLI_MAX_ARGS      16
#define CLI_DEFAULT_BASE  "./db_data"
#define CLI_PROMPT        "qsdb> "

/* 実行モード */
#define MODE_INTERACTIVE  0
#define MODE_ONESHOT      1

typedef struct CLI_STATE
{
    char base_dir[4096];
    QS_DB_CONTEXT ctx;
    int mode;
} CLI_STATE;

/* コマンド実装（後続タスクで順に実装する） */
static int cmd_table_or_database(CLI_STATE* st, int argc, char* argv[]);
static int cmd_show(CLI_STATE* st, int argc, char* argv[]);
static int cmd_use(CLI_STATE* st, int argc, char* argv[]);
static int cmd_schema(CLI_STATE* st, int argc, char* argv[]);
static int cmd_record(CLI_STATE* st, int argc, char* argv[]);
static int cmd_list(CLI_STATE* st, int argc, char* argv[]);
static int cmd_index(CLI_STATE* st, int argc, char* argv[]);
static int cmd_next(CLI_STATE* st, int argc, char* argv[]);

static void print_help(void)
{
    printf("Commands:\n");
    printf("  create database <name>\n");
    printf("  drop database <name>\n");
    printf("  show databases\n");
    printf("  use <name>\n");
    printf("  create table <name> [capacity <1|8|16|32|64|128|256|512|1024>]\n");
    printf("  drop table <name>\n");
    printf("  show tables\n");
    printf("  schema define <table> <json>\n");
    printf("  schema get <table>\n");
    printf("  schema validate <table> <json>\n");
    printf("  insert <table> <id> <json>\n");
    printf("  get <table> <id>\n");
    printf("  update <table> <id> <json>\n");
    printf("  delete <table> <id>\n");
    printf("  list <table> [filter_json] [index]\n");
    printf("  index add <table> <field> <value> <id>\n");
    printf("  index remove <table> <field> <value> <id>\n");
    printf("  index rebuild <table> <fields_json>\n");
    printf("  next id <table> <prefix>\n");
    printf("  help\n");
    printf("  exit / quit\n");
}

/* base_dir を mkdir -p 風に再帰的に作成する。存在すれば 0。 */
static int ensure_dir(const char* path)
{
    char tmp[4096];
    size_t len;
    size_t i;
    struct stat sb;

    if (0 == stat(path, &sb)) {
        return (S_ISDIR(sb.st_mode)) ? 0 : -1;
    }
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }
    for (i = 1; i < len; i++) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            if (0 != mkdir(tmp, 0755) && errno != EEXIST) {
                return -1;
            }
            tmp[i] = '/';
        }
    }
    if (0 != mkdir(tmp, 0755) && errno != EEXIST) {
        return -1;
    }
    return (0 == stat(path, &sb) && S_ISDIR(sb.st_mode)) ? 0 : -1;
}

/*
 * 1 行を空白区切りで argv に分割。引数数を返す。行頭空白・連続空白は無視。
 * JSON 引数（{ または [ で始まるトークン）は括弧の深さと文字列を考慮して
 * 内部の空白を保持したまま 1 トークンとして扱う（REPL ではクォートできないため）。
 */
static int split_line(char* line, char* argv[], int max_args)
{
    int count = 0;
    char* p = line;
    for (;;) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0' || count >= max_args) {
            break;
        }
        argv[count++] = p;
        if (*p == '{' || *p == '[') {
            int depth = 0;
            int in_str = 0;
            while (*p != '\0') {
                if (in_str) {
                    if (*p == '\\' && p[1] != '\0') {
                        p++; /* エスケープ文字をスキップ */
                    } else if (*p == '"') {
                        in_str = 0;
                    }
                } else {
                    if (*p == '"') {
                        in_str = 1;
                    } else if (*p == '{' || *p == '[') {
                        depth++;
                    } else if (*p == '}' || *p == ']') {
                        depth--;
                        if (depth == 0) {
                            p++;
                            break;
                        }
                    }
                }
                p++;
            }
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        } else {
            while (*p != '\0' && !isspace((unsigned char)*p)) {
                p++;
            }
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }
    }
    return count;
}

static int confirm_destructive(CLI_STATE* st, const char* message)
{
    if (MODE_ONESHOT == st->mode) {
        return 1; /* ワンショットモードでは確認を省略する */
    }
    printf("%s [y/N]: ", message);
    fflush(stdout);
    char line[128];
    if (NULL == fgets(line, sizeof(line), stdin)) {
        return 0;
    }
    return (line[0] == 'y' || line[0] == 'Y');
}

/* コマンド 1 行を実行する。0=成功, -1=エラー（対話モードでは続行、ワンショットでは失敗扱い） */
static int execute_command(CLI_STATE* st, const char* line)
{
    char buf[CLI_MAX_LINE];
    char* argv[CLI_MAX_ARGS];
    int argc;

    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    /* 行末の改行・空白を除去 */
    {
        size_t len = strlen(buf);
        while (len > 0 && isspace((unsigned char)buf[len - 1])) {
            buf[--len] = '\0';
        }
    }
    if (0 == strlen(buf)) {
        return 0;
    }

    argc = split_line(buf, argv, CLI_MAX_ARGS);
    if (argc <= 0) {
        return 0;
    }

    if (0 == strcmp(argv[0], "help")) {
        print_help();
        return 0;
    }
    if (0 == strcmp(argv[0], "exit") || 0 == strcmp(argv[0], "quit")) {
        return -100; /* 終了シグナル */
    }
    if (0 == strcmp(argv[0], "create") && argc >= 3 &&
        (0 == strcmp(argv[1], "database") || 0 == strcmp(argv[1], "table"))) {
        return cmd_table_or_database(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "drop") && argc >= 3 &&
        (0 == strcmp(argv[1], "database") || 0 == strcmp(argv[1], "table"))) {
        return cmd_table_or_database(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "show") && argc >= 2) {
        return cmd_show(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "use") && argc >= 2) {
        return cmd_use(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "schema") && argc >= 2) {
        return cmd_schema(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "insert") || 0 == strcmp(argv[0], "get") ||
        0 == strcmp(argv[0], "update") || 0 == strcmp(argv[0], "delete")) {
        return cmd_record(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "list")) {
        return cmd_list(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "index")) {
        return cmd_index(st, argc, argv);
    }
    if (0 == strcmp(argv[0], "next") && argc >= 2 && 0 == strcmp(argv[1], "id")) {
        return cmd_next(st, argc, argv);
    }

    fprintf(stderr, "ERROR: unknown command: %s (try 'help')\n", argv[0]);
    return -1;
}

/* --- コマンド実装 --- */

/* スクラッチメモリ確保（1 コマンド = 1 pool の寿命） */
static QS_MEMORY_POOL* scratch_alloc(void)
{
    QS_MEMORY_POOL* p = NULL;
    if (0 == qs_initialize_memory_f64(&p, SIZE_MBYTE * 4)) {
        return NULL;
    }
    return p;
}

static void scratch_free(QS_MEMORY_POOL* p)
{
    if (p != NULL) {
        qs_free(p);
    }
}

/* create/drop database|table */
static int cmd_table_or_database(CLI_STATE* st, int argc, char* argv[])
{
    const char* kind = argv[1]; /* database | table */
    const char* name = argv[2];

    if (0 == strcmp(argv[0], "create")) {
        if (0 == strcmp(kind, "database")) {
            if (argc != 3) {
                fprintf(stderr, "ERROR: usage: create database <name>\n");
                return -1;
            }
            if (0 == qs_db_create_database(st->base_dir, name)) {
                printf("OK\n");
                return 0;
            }
            fprintf(stderr, "ERROR: create database %s failed\n", name);
            return -1;
        }
        /* create table */
        if (!st->ctx.is_open) {
            fprintf(stderr, "ERROR: no database in use (use <name>)\n");
            return -1;
        }
        if (argc < 3 || argc > 5) {
            fprintf(stderr, "ERROR: usage: create table <name> [capacity <1|8|16|32|64|128|256|512|1024>]\n");
            return -1;
        }
        int capacity = QS_DB_CAPACITY_B1MB;
        if (argc == 5) {
            if (0 != strcmp(argv[3], "capacity")) {
                fprintf(stderr, "ERROR: expected 'capacity'\n");
                return -1;
            }
            const char* v = argv[4];
            if (0 == strcmp(v, "1")) capacity = QS_DB_CAPACITY_B1MB;
            else if (0 == strcmp(v, "8")) capacity = QS_DB_CAPACITY_B8MB;
            else if (0 == strcmp(v, "16")) capacity = QS_DB_CAPACITY_B16MB;
            else if (0 == strcmp(v, "32")) capacity = QS_DB_CAPACITY_B32MB;
            else if (0 == strcmp(v, "64")) capacity = QS_DB_CAPACITY_B64MB;
            else if (0 == strcmp(v, "128")) capacity = QS_DB_CAPACITY_B128MB;
            else if (0 == strcmp(v, "256")) capacity = QS_DB_CAPACITY_B256MB;
            else if (0 == strcmp(v, "512")) capacity = QS_DB_CAPACITY_B512MB;
            else if (0 == strcmp(v, "1024")) capacity = QS_DB_CAPACITY_B1024MB;
            else {
                fprintf(stderr, "ERROR: invalid capacity: %s\n", v);
                return -1;
            }
        }
        if (0 == qs_db_create_table(&st->ctx, name, capacity)) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: create table %s failed\n", name);
        return -1;
    }

    /* drop */
    if (0 == strcmp(kind, "database")) {
        if (argc != 3) {
            fprintf(stderr, "ERROR: usage: drop database <name>\n");
            return -1;
        }
        if (st->ctx.is_open && 0 == strcmp(st->ctx.database, name)) {
            fprintf(stderr, "ERROR: cannot drop database %s while in use\n", name);
            return -1;
        }
        if (!confirm_destructive(st, "Drop database?")) {
            printf("aborted\n");
            return 0;
        }
        if (0 == qs_db_drop_database(st->base_dir, name)) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: drop database %s failed\n", name);
        return -1;
    }
    /* drop table */
    if (!st->ctx.is_open) {
        fprintf(stderr, "ERROR: no database in use (use <name>)\n");
        return -1;
    }
    if (argc != 3) {
        fprintf(stderr, "ERROR: usage: drop table <name>\n");
        return -1;
    }
    if (!confirm_destructive(st, "Drop table?")) {
        printf("aborted\n");
        return 0;
    }
    if (0 == qs_db_drop_table(&st->ctx, name)) {
        printf("OK\n");
        return 0;
    }
    fprintf(stderr, "ERROR: drop table %s failed\n", name);
    return -1;
}

/* show databases | show tables */
static int cmd_show(CLI_STATE* st, int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "ERROR: usage: show databases | show tables\n");
        return -1;
    }
    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (NULL == scratch) {
        fprintf(stderr, "ERROR: scratch memory allocation failed\n");
        return -1;
    }
    int32_t arr;
    if (0 == strcmp(argv[1], "databases")) {
        arr = qs_db_show_database(scratch, st->base_dir);
    } else if (0 == strcmp(argv[1], "tables")) {
        if (!st->ctx.is_open) {
            fprintf(stderr, "ERROR: no database in use (use <name>)\n");
            scratch_free(scratch);
            return -1;
        }
        arr = qs_db_show_table(scratch, &st->ctx);
    } else {
        fprintf(stderr, "ERROR: usage: show databases | show tables\n");
        scratch_free(scratch);
        return -1;
    }
    if (-1 == arr) {
        fprintf(stderr, "ERROR: show %s failed\n", argv[1]);
        scratch_free(scratch);
        return -1;
    }
    int32_t len = qs_array_length(scratch, arr);
    int32_t i;
    for (i = 0; i < len; i++) {
        QS_ARRAY_ELEMENT* e = qs_array_get(scratch, arr, i);
        if (NULL == e || e->id != ELEMENT_LITERAL_STR) {
            continue;
        }
        printf("%s\n", (char*)QS_GET_POINTER(scratch, e->memid_array_element_data));
    }
    scratch_free(scratch);
    return 0;
}

/* use <name> */
static int cmd_use(CLI_STATE* st, int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "ERROR: usage: use <name>\n");
        return -1;
    }
    if (st->ctx.is_open) {
        qs_db_close_database(&st->ctx);
    }
    if (0 == qs_db_open_database(&st->ctx, st->base_dir, argv[1])) {
        printf("OK (database: %s)\n", argv[1]);
        return 0;
    }
    fprintf(stderr, "ERROR: open database %s failed\n", argv[1]);
    return -1;
}

/* schema define|get|validate */
static int cmd_schema(CLI_STATE* st, int argc, char* argv[])
{
    if (!st->ctx.is_open) {
        fprintf(stderr, "ERROR: no database in use (use <name>)\n");
        return -1;
    }
    if (0 == strcmp(argv[1], "define")) {
        if (argc != 4) {
            fprintf(stderr, "ERROR: usage: schema define <table> <json>\n");
            return -1;
        }
        if (0 == qs_db_schema_define(&st->ctx, argv[2], argv[3])) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: schema define %s failed\n", argv[2]);
        return -1;
    }
    if (0 == strcmp(argv[1], "get")) {
        char buf[65536];
        if (argc != 3) {
            fprintf(stderr, "ERROR: usage: schema get <table>\n");
            return -1;
        }
        if (0 == qs_db_schema_get(&st->ctx, argv[2], buf, sizeof(buf))) {
            printf("%s\n", buf);
            return 0;
        }
        fprintf(stderr, "ERROR: schema get %s failed (undefined or too large)\n", argv[2]);
        return -1;
    }
    if (0 == strcmp(argv[1], "validate")) {
        QS_MEMORY_POOL* scratch = scratch_alloc();
        if (NULL == scratch) {
            fprintf(stderr, "ERROR: scratch memory allocation failed\n");
            return -1;
        }
        if (argc != 4) {
            fprintf(stderr, "ERROR: usage: schema validate <table> <json>\n");
            scratch_free(scratch);
            return -1;
        }
        if (0 == qs_db_schema_validate(scratch, &st->ctx, argv[2], argv[3])) {
            printf("OK (valid)\n");
        } else {
            printf("INVALID\n");
        }
        scratch_free(scratch);
        return 0;
    }
    fprintf(stderr, "ERROR: usage: schema define|get|validate ...\n");
    return -1;
}

/* insert|get|update|delete <table> ... */
static int cmd_record(CLI_STATE* st, int argc, char* argv[])
{
    if (!st->ctx.is_open) {
        fprintf(stderr, "ERROR: no database in use (use <name>)\n");
        return -1;
    }
    if (0 == strcmp(argv[0], "insert")) {
        if (argc != 4) {
            fprintf(stderr, "ERROR: usage: insert <table> <id> <json>\n");
            return -1;
        }
        if (0 == qs_db_record_insert(&st->ctx, argv[1], argv[2], argv[3])) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: insert %s/%s failed (duplicate or invalid)\n", argv[1], argv[2]);
        return -1;
    }
    if (0 == strcmp(argv[0], "get")) {
        char buf[65536];
        if (argc != 3) {
            fprintf(stderr, "ERROR: usage: get <table> <id>\n");
            return -1;
        }
        if (0 == qs_db_record_get(&st->ctx, argv[1], argv[2], buf, sizeof(buf))) {
            printf("%s\n", buf);
            return 0;
        }
        fprintf(stderr, "ERROR: get %s/%s failed (not found or too large)\n", argv[1], argv[2]);
        return -1;
    }
    if (0 == strcmp(argv[0], "update")) {
        if (argc != 4) {
            fprintf(stderr, "ERROR: usage: update <table> <id> <json>\n");
            return -1;
        }
        if (0 == qs_db_record_update(&st->ctx, argv[1], argv[2], argv[3])) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: update %s/%s failed (not found or invalid)\n", argv[1], argv[2]);
        return -1;
    }
    if (0 == strcmp(argv[0], "delete")) {
        if (argc != 3) {
            fprintf(stderr, "ERROR: usage: delete <table> <id>\n");
            return -1;
        }
        if (0 == qs_db_record_delete(&st->ctx, argv[1], argv[2])) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: delete %s/%s failed (not found)\n", argv[1], argv[2]);
        return -1;
    }
    fprintf(stderr, "ERROR: unknown record command: %s\n", argv[0]);
    return -1;
}

/* list <table> [filter_json] [index] */
static int cmd_list(CLI_STATE* st, int argc, char* argv[])
{
    QS_MEMORY_POOL* scratch;
    char* result;
    int scan_mode = QS_DB_SCAN_FULL;
    const char* filter = NULL;

    if (!st->ctx.is_open) {
        fprintf(stderr, "ERROR: no database in use (use <name>)\n");
        return -1;
    }
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "ERROR: usage: list <table> [filter_json] [index]\n");
        return -1;
    }
    if (argc >= 3) {
        filter = argv[2];
    }
    if (argc == 4) {
        if (0 != strcmp(argv[3], "index")) {
            fprintf(stderr, "ERROR: unknown scan mode: %s (use 'index' for index scan)\n", argv[3]);
            return -1;
        }
        scan_mode = QS_DB_SCAN_INDEX;
    }
    scratch = scratch_alloc();
    if (NULL == scratch) {
        fprintf(stderr, "ERROR: scratch memory allocation failed\n");
        return -1;
    }
    result = qs_db_record_list(scratch, &st->ctx, argv[1], filter, scan_mode);
    if (NULL == result) {
        fprintf(stderr, "ERROR: list %s failed (invalid filter or record)\n", argv[1]);
        scratch_free(scratch);
        return -1;
    }
    printf("%s\n", result);
    scratch_free(scratch);
    return 0;
}

/* index add|remove|rebuild */
static int cmd_index(CLI_STATE* st, int argc, char* argv[])
{
    if (!st->ctx.is_open) {
        fprintf(stderr, "ERROR: no database in use (use <name>)\n");
        return -1;
    }
    if (0 == strcmp(argv[1], "add")) {
        if (argc != 6) {
            fprintf(stderr, "ERROR: usage: index add <table> <field> <value> <id>\n");
            return -1;
        }
        if (0 == qs_db_index_add(&st->ctx, argv[2], argv[3], argv[4], argv[5])) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: index add %s.%s=%s failed\n", argv[2], argv[3], argv[4]);
        return -1;
    }
    if (0 == strcmp(argv[1], "remove")) {
        if (argc != 6) {
            fprintf(stderr, "ERROR: usage: index remove <table> <field> <value> <id>\n");
            return -1;
        }
        if (0 == qs_db_index_remove(&st->ctx, argv[2], argv[3], argv[4], argv[5])) {
            printf("OK\n");
            return 0;
        }
        fprintf(stderr, "ERROR: index remove %s.%s=%s failed\n", argv[2], argv[3], argv[4]);
        return -1;
    }
    if (0 == strcmp(argv[1], "rebuild")) {
        QS_MEMORY_POOL* scratch = scratch_alloc();
        if (NULL == scratch) {
            fprintf(stderr, "ERROR: scratch memory allocation failed\n");
            return -1;
        }
        if (argc != 4) {
            fprintf(stderr, "ERROR: usage: index rebuild <table> <fields_json>\n");
            scratch_free(scratch);
            return -1;
        }
        if (0 == qs_db_index_rebuild(scratch, &st->ctx, argv[2], argv[3])) {
            printf("OK\n");
        } else {
            fprintf(stderr, "ERROR: index rebuild %s failed (partial index may remain)\n", argv[2]);
        }
        scratch_free(scratch);
        return 0;
    }
    fprintf(stderr, "ERROR: usage: index add|remove|rebuild ...\n");
    return -1;
}

/* next id <table> <prefix> */
static int cmd_next(CLI_STATE* st, int argc, char* argv[])
{
    char id[256];
    if (!st->ctx.is_open) {
        fprintf(stderr, "ERROR: no database in use (use <name>)\n");
        return -1;
    }
    if (argc != 4) {
        fprintf(stderr, "ERROR: usage: next id <table> <prefix>\n");
        return -1;
    }
    if (0 == qs_db_next_id(&st->ctx, argv[2], argv[3], id, sizeof(id))) {
        printf("%s\n", id);
        return 0;
    }
    fprintf(stderr, "ERROR: next id %s failed\n", argv[2]);
    return -1;
}

int main(int argc, char* argv[])
{
    CLI_STATE st;
    int i;
    int oneshot_count = 0;
    char oneshot[CLI_MAX_ARGS][CLI_MAX_LINE];

    memset(&st, 0, sizeof(st));
    st.mode = MODE_INTERACTIVE;
    strncpy(st.base_dir, CLI_DEFAULT_BASE, sizeof(st.base_dir) - 1);

    /* 引数解析: [base_dir] [-e "<command>"]... */
    for (i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "-e")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: -e requires an argument\n");
                return 2;
            }
            if (oneshot_count >= CLI_MAX_ARGS) {
                fprintf(stderr, "ERROR: too many -e arguments (max %d)\n", CLI_MAX_ARGS);
                return 2;
            }
            strncpy(oneshot[oneshot_count], argv[++i], CLI_MAX_LINE - 1);
            oneshot[oneshot_count][CLI_MAX_LINE - 1] = '\0';
            oneshot_count++;
            st.mode = MODE_ONESHOT;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "ERROR: unknown option: %s\n", argv[i]);
            return 2;
        } else {
            strncpy(st.base_dir, argv[i], sizeof(st.base_dir) - 1);
            st.base_dir[sizeof(st.base_dir) - 1] = '\0';
        }
    }

    if (0 != ensure_dir(st.base_dir)) {
        fprintf(stderr, "ERROR: cannot create base_dir: %s\n", st.base_dir);
        return 3;
    }

    if (MODE_ONESHOT == st.mode) {
        int rc;
        for (i = 0; i < oneshot_count; i++) {
            rc = execute_command(&st, oneshot[i]);
            if (-100 == rc) {
                break;
            }
            if (rc != 0) {
                qs_db_close_database(&st.ctx);
                return 1;
            }
        }
        qs_db_close_database(&st.ctx);
        return 0;
    }

    /* 対話モード */
    printf("qs_db_cli (base_dir: %s). Type 'help' for commands.\n", st.base_dir);
    for (;;) {
        char line[CLI_MAX_LINE];
        printf(CLI_PROMPT);
        fflush(stdout);
        if (NULL == fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        if (-100 == execute_command(&st, line)) {
            break;
        }
    }
    qs_db_close_database(&st.ctx);
    return 0;
}
