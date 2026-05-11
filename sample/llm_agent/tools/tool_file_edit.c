#include "tool_file_edit.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

/* Execute patch command with stdin, stdout, stderr capture via fork/exec.
 * Returns the exit code of the patch command. */
static int apply_patch_via_exec(const char* resolved_path,
                                 const char* patch_data,
                                 int dry_run,
                                 char* stdout_buf,
                                 size_t stdout_size,
                                 char* stderr_buf,
                                 size_t stderr_size)
{
    /* Create pipes for stdin, stdout, stderr */
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
    
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdin_pipe[0]);   close(stdin_pipe[1]);
        close(stdout_pipe[0]);  close(stdout_pipe[1]);
        close(stderr_pipe[0]);  close(stderr_pipe[1]);
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        /* Redirect stdin */
        close(stdin_pipe[1]);
        if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }
        close(stdin_pipe[0]);

        /* Redirect stdout */
        close(stdout_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }
        close(stdout_pipe[1]);

        /* Redirect stderr */
        close(stderr_pipe[0]);
        if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }
        close(stderr_pipe[1]);

        /* Build patch command arguments */
        char* args[6];
        int arg_count = 0;
        
        args[arg_count++] = (char*)"patch";
        if (dry_run) {
            args[arg_count++] = (char*)"--dry-run";
        }
        args[arg_count++] = (char*)"-t";  /* Don't ask questions */
        args[arg_count++] = (char*)"-N";  /* Skip if already applied */
        args[arg_count++] = (char*)resolved_path;
        args[arg_count] = NULL;

        /* Execute patch command */
        execvp("patch", args);
        
        /* If execvp returns, error occurred */
        exit(EXIT_FAILURE);
    }

    /* Parent process */
    /* Close unused ends */
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    /* Write patch data to child's stdin */
    size_t patch_len = strlen(patch_data);
    size_t written = 0;
    while (written < patch_len) {
        ssize_t n = write(stdin_pipe[1], patch_data + written, patch_len - written);
        if (n == -1) {
            if (errno != EINTR) break;
        } else {
            written += n;
        }
    }
    close(stdin_pipe[1]);

    /* Read stdout */
    size_t stdout_pos = 0;
    while (stdout_pos < stdout_size - 1) {
        ssize_t n = read(stdout_pipe[0], stdout_buf + stdout_pos, 
                        stdout_size - 1 - stdout_pos);
        if (n == -1) {
            if (errno != EINTR) break;
        } else if (n == 0) {
            break;  /* EOF */
        } else {
            stdout_pos += n;
        }
    }
    stdout_buf[stdout_pos] = '\0';
    close(stdout_pipe[0]);

    /* Read stderr */
    size_t stderr_pos = 0;
    while (stderr_pos < stderr_size - 1) {
        ssize_t n = read(stderr_pipe[0], stderr_buf + stderr_pos,
                        stderr_size - 1 - stderr_pos);
        if (n == -1) {
            if (errno != EINTR) break;
        } else if (n == 0) {
            break;  /* EOF */
        } else {
            stderr_pos += n;
        }
    }
    stderr_buf[stderr_pos] = '\0';
    close(stderr_pipe[0]);

    /* Wait for child and collect exit code */
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

int tool_file_edit_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024] = "";
    char patch_data[65536] = "";
    int dry_run = 0;

    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "path", path, sizeof(path));
        tool_json_extract_str(json_args, "patch", patch_data, sizeof(patch_data));
        dry_run = tool_json_extract_int(json_args, "dry_run", 0);
    }

    /* Validate inputs */
    if (path[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"path is required\"}");
        return -1;
    }
    if (patch_data[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"patch is required\"}");
        return -1;
    }

    /* Resolve path within workspace */
    char resolved_path[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved_path, sizeof(resolved_path)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    /* Allocate buffers for stdout/stderr */
    char* stdout_buf = (char*)malloc(4096);
    char* stderr_buf = (char*)malloc(4096);
    if (!stdout_buf || !stderr_buf) {
        free(stdout_buf);
        free(stderr_buf);
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    /* Apply patch */
    int exit_code = apply_patch_via_exec(resolved_path, patch_data, dry_run,
                                         stdout_buf, 4096, stderr_buf, 4096);

    /* Escape output for JSON */
    char esc_stdout[8192];
    char esc_stderr[8192];
    tool_json_escape(stdout_buf, esc_stdout, sizeof(esc_stdout));
    tool_json_escape(stderr_buf, esc_stderr, sizeof(esc_stderr));

    /* Build response JSON */
    int success = (exit_code == 0) ? 1 : 0;
    snprintf(output, output_size,
             "{\"success\":%d,"
             "\"exit_code\":%d,"
             "\"path\":\"%s\","
             "\"dry_run\":%d,"
             "\"stdout\":\"%s\","
             "\"stderr\":\"%s\"}",
             success, exit_code, path, dry_run, esc_stdout, esc_stderr);

    free(stdout_buf);
    free(stderr_buf);

    return success ? 0 : -1;
}
