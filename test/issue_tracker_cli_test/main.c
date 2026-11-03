/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

#include "qs_random.h"

static int pass_count = 0;
static int fail_count = 0;

/* 断片化テスト用: ランダム文字列のタイトルを生成する */
static void frag_random_title(char* buf, size_t buf_size)
{
    size_t len = 1 + (size_t)(qs_rand_32() % 250);
    if (len >= buf_size) {
        len = buf_size - 1;
    }
    const char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t i;
    for (i = 0; i < len; i++) {
        buf[i] = chars[qs_rand_32() % 62];
    }
    buf[len] = '\0';
}

static int run_cmd(const char* cmd)
{
    int rc = system(cmd);
    if (rc == -1) {
        return -1;
    }
    if (WIFEXITED(rc)) {
        return WEXITSTATUS(rc);
    }
    return -1;
}

static void check_exit(const char* name, int expected_exit, const char* cmd)
{
    int rc = run_cmd(cmd);
    if (rc == expected_exit) {
        pass_count++;
        printf("[PASS] %s\n", name);
        return;
    }
    fail_count++;
    printf("[FAIL] %s (expected=%d actual=%d)\n", name, expected_exit, rc);
}

int main(void)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("[FAIL] getcwd\n");
        return 1;
    }

    char data_dir[PATH_MAX];
    if (strlen(cwd) + strlen("/tmp_issue_tracker_cli_test_data") + 1 >= sizeof(data_dir)) {
        printf("[FAIL] data_dir path too long\n");
        return 1;
    }
    strcpy(data_dir, cwd);
    strcat(data_dir, "/tmp_issue_tracker_cli_test_data");

    const char* cli_bin = "../../sample/issue_tracker_cli/qs_issue_tracker_cli";

    check_exit(
        "build issue_tracker_cli",
        0,
        "cd ../../sample/issue_tracker_cli && make >/dev/null"
    );

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", data_dir);
    check_exit("cleanup before test", 0, cmd);

    snprintf(cmd, sizeof(cmd), "%s init --base-dir \"%s\" >/dev/null", cli_bin, data_dir);
    check_exit("init stores", 0, cmd);

    snprintf(cmd, sizeof(cmd), "test -f \"%s/system.db\"", data_dir);
    check_exit("system store created", 0, cmd);
    snprintf(cmd, sizeof(cmd), "test -f \"%s/system_settings.json\"", data_dir);
    check_exit("system settings created", 0, cmd);
    snprintf(cmd, sizeof(cmd), "test -f \"%s/schema_user.json\"", data_dir);
    check_exit("schema_user created", 0, cmd);
    snprintf(cmd, sizeof(cmd), "test -f \"%s/schema_label.json\"", data_dir);
    check_exit("schema_label created", 0, cmd);
    snprintf(cmd, sizeof(cmd), "test -f \"%s/schema_issue.json\"", data_dir);
    check_exit("schema_issue created", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s schema list --base-dir \"%s\" | grep -q '\"entity\":\"issue\"'",
        cli_bin,
        data_dir
    );
    check_exit("schema list", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s schema get --base-dir \"%s\" --entity issue | grep -q '\"name\":\"status\"'",
        cli_bin,
        data_dir
    );
    check_exit("schema get issue", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s schema get --base-dir \"%s\" --entity invalid >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("schema get invalid entity fails", 2, cmd);

    snprintf(cmd, sizeof(cmd), "%s init --base-dir \"%s\" >/dev/null 2>&1", cli_bin, data_dir);
    check_exit("init without force fails", 4, cmd);

    snprintf(cmd, sizeof(cmd), "%s init --base-dir \"%s\" --force >/dev/null", cli_bin, data_dir);
    check_exit("init with force", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s user create --base-dir \"%s\" --id u1 --json '{\"id\":\"u1\",\"name\":\"alice\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("user create", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s user create --base-dir \"%s\" --json '{\"name\":\"bob\"}' | grep -q 'generated id: usr_'",
        cli_bin,
        data_dir
    );
    check_exit("user auto id create", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s label create --base-dir \"%s\" --id bug --json '{\"id\":\"bug\",\"name\":\"Bug\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("label create", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s label create --base-dir \"%s\" --json '{\"name\":\"Feature\"}' | grep -q 'generated id: lbl_'",
        cli_bin,
        data_dir
    );
    check_exit("label auto id create", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s user create --base-dir \"%s\" --json '{\"name\":\"x\",\"unknown\":1}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("user create unknown field fails", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s user list --base-dir \"%s\" | grep -q '\"id\":\"u1\"'",
        cli_bin,
        data_dir
    );
    check_exit("user list", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s label list --base-dir \"%s\" | grep -q '\"id\":\"bug\"'",
        cli_bin,
        data_dir
    );
    check_exit("label list", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s user update --base-dir \"%s\" --id u1 --json '{\"id\":\"u999\",\"name\":\"alice3\"}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("user update id mismatch fails", 2, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s user update --base-dir \"%s\" --id u1 --json '{\"id\":\"u1\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("user partial update", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s user get --base-dir \"%s\" --id u1 | grep -q '\"name\":\"alice\"'",
        cli_bin,
        data_dir
    );
    check_exit("user partial update keeps old value", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s user unset --base-dir \"%s\" --id u1 --field name >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("user unset name", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s user get --base-dir \"%s\" --id u1 | grep -q '\"name\":\"\"'",
        cli_bin,
        data_dir
    );
    check_exit("user unset resets default", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s user unset --base-dir \"%s\" --id u1 --field id >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("user unset id disallowed", 2, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s user update --base-dir \"%s\" --id u1 --json '{\"id\":\"u1\",\"name\":\"alice3\",\"unknown\":1}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("user update unknown field fails", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue create --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"title\":\"bug\",\"status\":\"open\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"]}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("issue create", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue create --base-dir \"%s\" --json '{\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"]}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("issue create default title/status", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue create --base-dir \"%s\" --id iss_bad_ref --json '{\"id\":\"iss_bad_ref\",\"title\":\"bad\",\"status\":\"open\",\"assigneeIds\":[\"u999\"],\"labelIds\":[\"bug\"]}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue create with missing assignee fails", 2, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue create --base-dir \"%s\" --id iss_bad_status --json '{\"id\":\"iss_bad_status\",\"title\":\"bad\",\"status\":\"invalid_status\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"]}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue create with invalid status fails", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue create --base-dir \"%s\" --json '{\"title\":\"bad\",\"status\":\"open\",\"unknown\":1}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue create unknown field fails", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue create --base-dir \"%s\" --json '{\"title\":\"auto\",\"status\":\"open\"}' | grep -q 'generated id: iss_'",
        cli_bin,
        data_dir
    );
    check_exit("issue auto id create", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list by status(open)", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s user create --base-dir \"%s\" --id u2 --json '{\"id\":\"u2\",\"name\":\"bob\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("user create u2", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s label create --base-dir \"%s\" --id feature --json '{\"id\":\"feature\",\"name\":\"Feature\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("label create feature", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"title\":\"bug2\",\"status\":\"in_progress\",\"assigneeIds\":[\"u2\"],\"labelIds\":[\"feature\"]}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("issue update", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"title\":\"bug3\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("issue partial update", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue get --base-dir \"%s\" --id iss1 | grep -q '\"status\":\"in_progress\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue partial update keeps status", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"description\":\"memo\"}' >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("issue update description", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue unset --base-dir \"%s\" --id iss1 --field description >/dev/null",
        cli_bin,
        data_dir
    );
    check_exit("issue unset description", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue get --base-dir \"%s\" --id iss1 | grep -q '\"description\":\"\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue unset description resets default", 0, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue unset --base-dir \"%s\" --id iss1 --field labelIds >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue unset labelIds disallowed", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"title\":\"badref\",\"status\":\"in_progress\",\"assigneeIds\":[\"u2\"],\"labelIds\":[\"not_exists\"]}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue update with missing label fails", 2, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"status\":\"invalid_status\"}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue update with invalid status fails", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss999\",\"title\":\"x\",\"status\":\"in_progress\"}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue update id mismatch fails", 2, cmd);
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"title\":\"x\",\"status\":\"in_progress\",\"unknown\":1}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue update unknown field fails", 2, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue removed from old status index", 1, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --assignee u2 | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list by assignee index", 0, cmd);

    /* full scan: same results as index scan */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress --scan full | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list by status full scan", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --assignee u2 --scan full | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list by assignee full scan", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --label feature --scan full | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list by label full scan", 0, cmd);

    /* full scan: no filter returns all issues (iss1 + 2 auto-id) */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --scan full | grep -c '\"id\"' | grep -q '3'",
        cli_bin,
        data_dir
    );
    check_exit("issue list full scan no filter", 0, cmd);

    /* full scan: non-matching filter returns no issues */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status closed --scan full | grep -q '\"id\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list full scan empty result", 1, cmd);

    /* composite AND: index scan (iss1 = in_progress/u2/feature) */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress --assignee u2 | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: status+assignee", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress --label feature | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: status+label", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --assignee u2 --label feature | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: assignee+label", 0, cmd);

    /* composite AND: all three filters */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress --assignee u2 --label feature | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: all three", 0, cmd);

    /* composite AND: order independence */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --assignee u2 --status in_progress | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: order independent", 0, cmd);

    /* composite AND: full scan matches index scan */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress --assignee u2 --scan full | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite full: status+assignee", 0, cmd);

    /* composite AND: non-matching combination returns empty */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress --assignee u1 | grep -q '\"id\"'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: non-matching empty", 1, cmd);

    /* composite AND: open+u1 matches only iss_000001 (iss1 is in_progress, iss_000002 has no assignee) */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open --assignee u1 | grep -c '\"id\"' | grep -q '1'",
        cli_bin,
        data_dir
    );
    check_exit("composite index: open+u1 count 1", 0, cmd);

    /* invalid scan value fails */
    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --scan invalid >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue list invalid scan fails", 2, cmd);

    snprintf(cmd, sizeof(cmd), "%s compact --base-dir \"%s\" --store all >/dev/null", cli_bin, data_dir);
    check_exit("compact all stores", 0, cmd);

    snprintf(cmd, sizeof(cmd), "%s issue delete --base-dir \"%s\" --id iss1 >/dev/null", cli_bin, data_dir);
    check_exit("issue delete", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status in_progress | grep -q '\"id\":\"iss1\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue removed from status index after delete", 1, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin,
        data_dir
    );
    check_exit("verify ok on consistent index", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s reindex --base-dir \"%s\"",
        cli_bin,
        data_dir
    );
    check_exit("reindex rebuilds index", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin,
        data_dir
    );
    check_exit("verify ok after reindex", 0, cmd);

    snprintf(
        cmd,
        sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open | grep -q '\"id\":\"iss_000001\"'",
        cli_bin,
        data_dir
    );
    check_exit("issue list works after reindex", 0, cmd);

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", data_dir);
    check_exit("cleanup after test", 0, cmd);

    /* --- stress test: 500 issues --- */
    char stress_dir[PATH_MAX];
    if (strlen(cwd) + strlen("/tmp_issue_stress_data") + 1 >= sizeof(stress_dir)) {
        printf("[FAIL] stress_dir path too long\n");
        return 1;
    }
    strcpy(stress_dir, cwd);
    strcat(stress_dir, "/tmp_issue_stress_data");

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", stress_dir);
    check_exit("stress: cleanup before", 0, cmd);

    snprintf(cmd, sizeof(cmd), "%s init --base-dir \"%s\" >/dev/null", cli_bin, stress_dir);
    check_exit("stress: init", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s user create --base-dir \"%s\" --id su1 --json '{\"id\":\"su1\",\"name\":\"stress\"}' >/dev/null",
        cli_bin, stress_dir);
    check_exit("stress: user create", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s label create --base-dir \"%s\" --id sl1 --json '{\"id\":\"sl1\",\"name\":\"Stress\"}' >/dev/null",
        cli_bin, stress_dir);
    check_exit("stress: label create", 0, cmd);

    /* 500 issues via shell loop */
    snprintf(cmd, sizeof(cmd),
        "for i in $(seq 1 500); do "
        "%s issue create --base-dir \"%s\" "
        "--json \"{\\\"title\\\":\\\"stress_$i\\\",\\\"status\\\":\\\"open\\\",\\\"assigneeIds\\\":[\\\"su1\\\"],\\\"labelIds\\\":[\\\"sl1\\\"]}\" "
        ">/dev/null 2>&1 || exit 1; done",
        cli_bin, stress_dir);
    check_exit("stress: create 500 issues", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin, stress_dir);
    check_exit("stress: verify ok after 500 issues", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s reindex --base-dir \"%s\" | grep -q 'reindex: rebuilt index from 500 issues'",
        cli_bin, stress_dir);
    check_exit("stress: reindex 500 issues", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin, stress_dir);
    check_exit("stress: verify ok after reindex", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: list 500 by status", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --assignee su1 | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: list 500 by assignee", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --label sl1 | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: list 500 by label", 0, cmd);

    /* full scan: same count as index scan */
    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open --scan full | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: list 500 by status full scan", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --assignee su1 --scan full | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: list 500 by assignee full scan", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --label sl1 --scan full | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: list 500 by label full scan", 0, cmd);

    /* composite AND: all 500 issues are open/su1/sl1 */
    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open --assignee su1 | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: composite index status+assignee 500", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open --assignee su1 --label sl1 | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: composite index all three 500", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open --assignee su1 --scan full | grep -c '\"id\"' | grep -q '500'",
        cli_bin, stress_dir);
    check_exit("stress: composite full status+assignee 500", 0, cmd);

    /* composite AND: non-matching assignee returns empty */
    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" --status open --assignee nobody | grep -q '\"id\"'",
        cli_bin, stress_dir);
    check_exit("stress: composite index non-matching empty", 1, cmd);

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", stress_dir);
    check_exit("stress: cleanup after", 0, cmd);

    /* --- fragmentation test: random create/update/delete --- */
    char frag_dir[PATH_MAX];
    if (strlen(cwd) + strlen("/tmp_issue_frag_data") + 1 >= sizeof(frag_dir)) {
        printf("[FAIL] frag_dir path too long\n");
        return 1;
    }
    strcpy(frag_dir, cwd);
    strcat(frag_dir, "/tmp_issue_frag_data");

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", frag_dir);
    check_exit("frag: cleanup before", 0, cmd);

    snprintf(cmd, sizeof(cmd), "%s init --base-dir \"%s\" >/dev/null", cli_bin, frag_dir);
    check_exit("frag: init", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s user create --base-dir \"%s\" --id fu1 --json '{\"id\":\"fu1\",\"name\":\"frag\"}' >/dev/null",
        cli_bin, frag_dir);
    check_exit("frag: user create", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s label create --base-dir \"%s\" --id fl1 --json '{\"id\":\"fl1\",\"name\":\"Frag\"}' >/dev/null",
        cli_bin, frag_dir);
    check_exit("frag: label create", 0, cmd);

    qs_srand_32();

    const char* frag_id_file = "/tmp/frag_random_id.txt";
    const char* frag_log_file = "/tmp/frag_debug.log";
    FILE* frag_log = fopen(frag_log_file, "w");
    if (frag_log) {
        fprintf(frag_log, "frag debug log: seed=%u\n", qs_xorshift_32_seed);
    }

    int frag_ops = 2000;
    int frag_errors = 0;
    int frag_creates = 0, frag_updates = 0, frag_deletes = 0;
    int frag_first_fail_iter = -1;
    int iter;
    for (iter = 0; iter < frag_ops; iter++) {
        uint32_t action = qs_rand_32() % 100;
        char title[256];
        frag_random_title(title, sizeof(title));
        int rc = 0;
        const char* action_name = "create";
        char target_id[128] = "";

        if (action < 40) {
            /* create */
            snprintf(cmd, sizeof(cmd),
                "%s issue create --base-dir \"%s\" "
                "--json '{\"title\":\"%s\",\"status\":\"open\",\"assigneeIds\":[\"fu1\"],\"labelIds\":[\"fl1\"]}' "
                ">/dev/null 2>&1",
                cli_bin, frag_dir, title);
            rc = run_cmd(cmd);
            if (rc != 0) frag_errors++;
            else frag_creates++;
        } else if (action < 70) {
            /* update: pick random existing issue */
            action_name = "update";
            snprintf(cmd, sizeof(cmd),
                "%s issue list --base-dir \"%s\" 2>/dev/null > \"%s\"",
                cli_bin, frag_dir, frag_id_file);
            run_cmd(cmd);
            FILE* f = fopen(frag_id_file, "r");
            if (f) {
                char line[2048];
                char ids[1024][64];
                int id_count = 0;
                while (fgets(line, sizeof(line), f) && id_count < 1024) {
                    char* id_start = strstr(line, "\"id\":\"");
                    if (id_start) {
                        id_start += strlen("\"id\":\"");
                        char* id_end = strchr(id_start, '\"');
                        if (id_end && (size_t)(id_end - id_start) < sizeof(ids[0])) {
                            size_t id_size = (size_t)(id_end - id_start);
                            memcpy(ids[id_count], id_start, id_size);
                            ids[id_count][id_size] = '\0';
                            id_count++;
                        }
                    }
                }
                fclose(f);
                if (id_count > 0) {
                    snprintf(target_id, sizeof(target_id), "%s", ids[qs_rand_32() % id_count]);
                }
            }
            if (target_id[0] != '\0') {
                snprintf(cmd, sizeof(cmd),
                    "%s issue update --base-dir \"%s\" --id \"%s\" "
                    "--json '{\"id\":\"%s\",\"title\":\"%s\",\"status\":\"open\",\"assigneeIds\":[\"fu1\"],\"labelIds\":[\"fl1\"]}' >/dev/null 2>&1",
                    cli_bin, frag_dir, target_id, target_id, title);
                rc = run_cmd(cmd);
                if (rc != 0) frag_errors++;
                else frag_updates++;
            } else {
                action_name = "create_fallback";
                snprintf(cmd, sizeof(cmd),
                    "%s issue create --base-dir \"%s\" "
                    "--json '{\"title\":\"%s\",\"status\":\"open\"}' >/dev/null 2>&1",
                    cli_bin, frag_dir, title);
                rc = run_cmd(cmd);
                if (rc != 0) frag_errors++;
                else frag_creates++;
            }
        } else {
            /* delete: pick random existing issue, delete if exists else create */
            action_name = "delete";
            snprintf(cmd, sizeof(cmd),
                "%s issue list --base-dir \"%s\" 2>/dev/null > \"%s\"",
                cli_bin, frag_dir, frag_id_file);
            run_cmd(cmd);
            FILE* f = fopen(frag_id_file, "r");
            if (f) {
                char line[2048];
                char ids[1024][64];
                int id_count = 0;
                while (fgets(line, sizeof(line), f) && id_count < 1024) {
                    char* id_start = strstr(line, "\"id\":\"");
                    if (id_start) {
                        id_start += strlen("\"id\":\"");
                        char* id_end = strchr(id_start, '\"');
                        if (id_end && (size_t)(id_end - id_start) < sizeof(ids[0])) {
                            size_t id_size = (size_t)(id_end - id_start);
                            memcpy(ids[id_count], id_start, id_size);
                            ids[id_count][id_size] = '\0';
                            id_count++;
                        }
                    }
                }
                fclose(f);
                if (id_count > 0) {
                    snprintf(target_id, sizeof(target_id), "%s", ids[qs_rand_32() % id_count]);
                }
            }
            if (target_id[0] != '\0') {
                snprintf(cmd, sizeof(cmd),
                    "%s issue delete --base-dir \"%s\" --id \"%s\" >/dev/null 2>&1",
                    cli_bin, frag_dir, target_id);
                rc = run_cmd(cmd);
                if (rc != 0) frag_errors++;
                else frag_deletes++;
            } else {
                action_name = "create_fallback";
                snprintf(cmd, sizeof(cmd),
                    "%s issue create --base-dir \"%s\" "
                    "--json '{\"title\":\"%s\",\"status\":\"open\"}' >/dev/null 2>&1",
                    cli_bin, frag_dir, title);
                rc = run_cmd(cmd);
                if (rc != 0) frag_errors++;
                else frag_creates++;
            }
        }

        /* log failed operations */
        if (rc != 0 && frag_log) {
            fprintf(frag_log, "iter=%d action=%s id=%s rc=%d title=%.40s\n",
                iter, action_name, target_id, rc, title);
        }

        /* periodic verify every 500 ops to find when inconsistency appears */
        if ((iter + 1) % 500 == 0) {
            char vfile[256];
            snprintf(vfile, sizeof(vfile), "/tmp/frag_verify_%d.txt", iter + 1);
            snprintf(cmd, sizeof(cmd),
                "%s verify --base-dir \"%s\" > \"%s\" 2>&1",
                cli_bin, frag_dir, vfile);
            run_cmd(cmd);
            snprintf(cmd, sizeof(cmd), "grep -q 'verify: ok' \"%s\"", vfile);
            int vrc = run_cmd(cmd);
            if (frag_log) {
                fprintf(frag_log, "periodic_verify iter=%d rc=%d\n", iter + 1, vrc);
            }
            if (vrc != 0 && frag_first_fail_iter < 0) {
                frag_first_fail_iter = iter + 1;
                if (frag_log) {
                    fprintf(frag_log, "FIRST INCONSISTENCY at iter=%d\n", iter + 1);
                }
                /* keep the first failing verify output for inspection */
                char keep_file[256];
                snprintf(keep_file, sizeof(keep_file), "/tmp/frag_verify_firstfail.txt");
                rename(vfile, keep_file);
                if (frag_log) {
                    fprintf(frag_log, "kept verify output: %s\n", keep_file);
                }
            } else {
                unlink(vfile);
            }
        }
    }

    if (frag_log) {
        fclose(frag_log);
    }

    printf("frag: ops=%d creates=%d updates=%d deletes=%d errors=%d first_inconsistency=%d\n",
        frag_ops, frag_creates, frag_updates, frag_deletes, frag_errors, frag_first_fail_iter);

    /* save full verify output for inspection */
    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" > /tmp/frag_verify_final.txt 2>&1",
        cli_bin, frag_dir);
    run_cmd(cmd);
    printf("frag: verify output saved to /tmp/frag_verify_final.txt\n");

    snprintf(cmd, sizeof(cmd), "%s compact --base-dir \"%s\" --store all >/dev/null", cli_bin, frag_dir);
    check_exit("frag: compact all", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" > /tmp/frag_verify_postcompact.txt 2>&1",
        cli_bin, frag_dir);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd),
        "grep -q 'verify: ok' /tmp/frag_verify_postcompact.txt");
    check_exit("frag: verify ok after fragmentation", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s reindex --base-dir \"%s\" | grep -q 'reindex: rebuilt index'",
        cli_bin, frag_dir);
    check_exit("frag: reindex after fragmentation", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin, frag_dir);
    check_exit("frag: verify ok after reindex", 0, cmd);

    snprintf(cmd, sizeof(cmd),
        "%s issue list --base-dir \"%s\" | grep -c '\"id\"' | grep -q '[0-9]'",
        cli_bin, frag_dir);
    check_exit("frag: issue list works", 0, cmd);

    unlink(frag_id_file);
    if (fail_count > 0) {
        printf("frag: data dir kept for inspection: %s\n", frag_dir);
        printf("frag: debug log kept: %s\n", frag_log_file);
        printf("frag: verify output kept: /tmp/frag_verify_final.txt\n");
        printf("frag: post-compact verify kept: /tmp/frag_verify_postcompact.txt\n");
        printf("frag: first-fail verify kept: /tmp/frag_verify_firstfail.txt\n");
    } else {
        unlink(frag_log_file);
        unlink("/tmp/frag_verify_final.txt");
        unlink("/tmp/frag_verify_postcompact.txt");
        unlink("/tmp/frag_verify_firstfail.txt");
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", frag_dir);
        check_exit("frag: cleanup after", 0, cmd);
    }

    /* ==================== comment 統合テスト ==================== */
    char cmt_dir[PATH_MAX];
    if (strlen(cwd) + strlen("/tmp_issue_cmt_test_data") + 1 >= sizeof(cmt_dir)) {
        printf("[FAIL] cmt_dir path too long\n");
        return 1;
    }
    strcpy(cmt_dir, cwd);
    strcat(cmt_dir, "/tmp_issue_cmt_test_data");

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", cmt_dir);
    check_exit("cmt: cleanup before", 0, cmd);
    snprintf(cmd, sizeof(cmd), "%s init --base-dir \"%s\" >/dev/null", cli_bin, cmt_dir);
    check_exit("cmt: init", 0, cmd);
    snprintf(cmd, sizeof(cmd), "test -f \"%s/issue_comment.db\"", cmt_dir);
    check_exit("cmt: comment store created", 0, cmd);
    snprintf(cmd, sizeof(cmd), "test -f \"%s/schema_comment.json\"", cmt_dir);
    check_exit("cmt: schema_comment created", 0, cmd);
    snprintf(
        cmd, sizeof(cmd),
        "%s schema get --base-dir \"%s\" --entity comment | grep -q '\"name\":\"body\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: schema get comment", 0, cmd);

    /* user / issue 準備 */
    snprintf(cmd, sizeof(cmd),
        "%s user create --id u1 --json '{\"name\":\"alice\"}' --base-dir \"%s\" >/dev/null",
        cli_bin, cmt_dir);
    check_exit("cmt: user create", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue create --id iss1 --json '{\"title\":\"t1\",\"status\":\"open\"}' --base-dir \"%s\" >/dev/null",
        cli_bin, cmt_dir);
    check_exit("cmt: issue create", 0, cmd);

    /* comment create */
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --issue iss1 --json '{\"body\":\"first comment\",\"userId\":\"u1\"}' --base-dir \"%s\" | grep -q 'generated id: cmt_000001'",
        cli_bin, cmt_dir);
    check_exit("cmt: create auto id", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --issue iss1 --id cmt_x --json '{\"body\":\"second\"}' --base-dir \"%s\" | grep -q 'created: comment:cmt_x'",
        cli_bin, cmt_dir);
    check_exit("cmt: create explicit id", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --issue iss1 --id cmt_x --json '{\"body\":\"dup\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: create duplicate id fails", 4, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --issue iss_missing --json '{\"body\":\"x\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: create unknown issue fails", 5, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --issue iss1 --json '{\"body\":\"x\",\"userId\":\"u_missing\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: create unknown user fails", 2, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --issue iss1 --json '{\"body\":\"\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: create empty body fails", 2, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment create --json '{\"body\":\"x\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: create without --issue fails", 2, cmd);

    /* comment get */
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_000001 --base-dir \"%s\" | grep -q '\"issueId\":\"iss1\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: get issueId", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_000001 --base-dir \"%s\" | grep -q '\"createdAtJst\":\"[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: get createdAtJst format", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_000001 --base-dir \"%s\" | grep -q '\"user\":{\"id\":\"u1\",\"name\":\"alice\"}'",
        cli_bin, cmt_dir);
    check_exit("cmt: get user enrichment", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_x --base-dir \"%s\" | grep -q '\"body\":\"second\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: get without userId has no user field", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_missing --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: get missing fails", 5, cmd);

    /* comment list */
    snprintf(cmd, sizeof(cmd),
        "%s issue comment list --issue iss1 --base-dir \"%s\" | grep -c '\"id\"' | grep -q 2",
        cli_bin, cmt_dir);
    check_exit("cmt: list count 2", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment list --issue iss_missing --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: list unknown issue fails", 5, cmd);

    /* comment update */
    snprintf(cmd, sizeof(cmd),
        "%s issue comment update --id cmt_x --json '{\"body\":\"updated body\"}' --base-dir \"%s\" | grep -q 'updated: comment:cmt_x'",
        cli_bin, cmt_dir);
    check_exit("cmt: update body", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_x --base-dir \"%s\" | grep -q '\"body\":\"updated body\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: update reflected", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment update --id cmt_x --json '{\"issueId\":\"iss2\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: update immutable issueId fails", 2, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment update --id cmt_x --json '{\"userId\":\"u2\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: update immutable userId fails", 2, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment update --id cmt_x --json '{\"createdAt\":123}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: update immutable createdAt fails", 2, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment update --id cmt_x --json '{\"body\":\"\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: update empty body fails", 2, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment update --id cmt_missing --json '{\"body\":\"x\"}' --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: update missing fails", 5, cmd);

    /* issue get --with-comments */
    snprintf(cmd, sizeof(cmd),
        "%s issue get --id iss1 --base-dir \"%s\" | grep -q '\"title\":\"t1\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: issue get without comments (backward compat)", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue get --id iss1 --base-dir \"%s\" | grep -q '\"comments\"' >/dev/null",
        cli_bin, cmt_dir);
    check_exit("cmt: issue get plain has no comments field", 1, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue get --id iss1 --with-comments --base-dir \"%s\" | grep -q '\"comments\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: issue get --with-comments has comments", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue get --id iss1 --with-comments --base-dir \"%s\" | grep -q '\"createdAtJst\"'",
        cli_bin, cmt_dir);
    check_exit("cmt: with-comments has createdAtJst", 0, cmd);

    /* verify / reindex with comments */
    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin, cmt_dir);
    check_exit("cmt: verify ok with comments", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s reindex --base-dir \"%s\" | grep -q 'reindex: rebuilt index from 1 issues, 2 comments'",
        cli_bin, cmt_dir);
    check_exit("cmt: reindex with comments", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin, cmt_dir);
    check_exit("cmt: verify ok after reindex", 0, cmd);

    /* comment delete */
    snprintf(cmd, sizeof(cmd),
        "%s issue comment delete --id cmt_x --base-dir \"%s\" | grep -q 'deleted: comment:cmt_x'",
        cli_bin, cmt_dir);
    check_exit("cmt: delete", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_x --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: deleted comment gone", 5, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment list --issue iss1 --base-dir \"%s\" | grep -c '\"id\"' | grep -q 1",
        cli_bin, cmt_dir);
    check_exit("cmt: list count 1 after delete", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment delete --id cmt_x --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: delete missing fails", 5, cmd);

    /* issue delete カスケード削除 */
    snprintf(cmd, sizeof(cmd),
        "%s issue delete --id iss1 --base-dir \"%s\" | grep -q 'deleted: issue:iss1'",
        cli_bin, cmt_dir);
    check_exit("cmt: issue delete", 0, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s issue comment get --id cmt_000001 --base-dir \"%s\" >/dev/null 2>&1",
        cli_bin, cmt_dir);
    check_exit("cmt: cascade delete removed comment", 5, cmd);
    snprintf(cmd, sizeof(cmd),
        "%s verify --base-dir \"%s\" | grep -q 'verify: ok'",
        cli_bin, cmt_dir);
    check_exit("cmt: verify ok after cascade delete", 0, cmd);

    /* compact comment */
    snprintf(cmd, sizeof(cmd),
        "%s compact --store comment --base-dir \"%s\" >/dev/null",
        cli_bin, cmt_dir);
    check_exit("cmt: compact comment", 0, cmd);

    if (fail_count > 0) {
        printf("cmt: data dir kept for inspection: %s\n", cmt_dir);
    } else {
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", cmt_dir);
        check_exit("cmt: cleanup after", 0, cmd);
    }

    printf("\nresult: pass=%d fail=%d\n", pass_count, fail_count);
    return (fail_count == 0) ? 0 : 1;
}
