/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

static int pass_count = 0;
static int fail_count = 0;

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
        "%s issue update --base-dir \"%s\" --id iss1 --json '{\"id\":\"iss1\",\"title\":\"badref\",\"status\":\"in_progress\",\"assigneeIds\":[\"u2\"],\"labelIds\":[\"not_exists\"]}' >/dev/null 2>&1",
        cli_bin,
        data_dir
    );
    check_exit("issue update with missing label fails", 2, cmd);

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

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", data_dir);
    check_exit("cleanup after test", 0, cmd);

    printf("\nresult: pass=%d fail=%d\n", pass_count, fail_count);
    return (fail_count == 0) ? 0 : 1;
}
