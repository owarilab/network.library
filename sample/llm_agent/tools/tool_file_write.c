#include "tool_file_write.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>

/* Resolve a path within the workspace for writing (doesn't require file to exist).
 * Returns 0 on success, -1 on failure. */
static int resolve_write_path(const char* user_path, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0 || !user_path || user_path[0] == '\0') {
        return -1;
    }

    const char* root = agent_get_workspace_root();
    const char* rel = user_path;

    /* Security: reject absolute paths and ".." */
    if (rel[0] == '/') return -1;
    if (strstr(rel, "..") != NULL) return -1;

    /* Remove leading "./" if present */
    while (rel[0] == '.' && rel[1] == '/') rel += 2;

    /* Build the target path */
    char target[AGENT_PATH_MAX];
    if (rel[0] == '\0' || (rel[0] == '.' && rel[1] == '\0')) {
        snprintf(target, sizeof(target), "%s", root);
    } else {
        snprintf(target, sizeof(target), "%s/%s", root, rel);
    }

    /* Resolve the target (parent directory must exist or be creatable) */
    char resolved[AGENT_PATH_MAX];
    if (realpath(target, resolved) != NULL) {
        /* File/directory already exists, use its resolved path */
        strncpy(out, resolved, out_size - 1);
        out[out_size - 1] = '\0';
        return 0;
    }

    /* File doesn't exist. Resolve parent directory instead. */
    char parent_buf[AGENT_PATH_MAX];
    strncpy(parent_buf, target, sizeof(parent_buf) - 1);
    parent_buf[sizeof(parent_buf) - 1] = '\0';
    char* parent_dir = dirname(parent_buf);

    if (!parent_dir || parent_dir[0] == '\0' || strcmp(parent_dir, ".") == 0) {
        /* Parent is "." - use workspace root */
        strncpy(out, root, out_size - 1);
        out[out_size - 1] = '\0';
        return 0;
    }

    char parent_resolved[AGENT_PATH_MAX];
    if (realpath(parent_dir, parent_resolved) == NULL) {
        /* Parent directory doesn't exist - will be created later */
        /* For now, validate that the target is within workspace conceptually */
        if (strncmp(target, root, strlen(root)) != 0) {
            return -1; /* Outside workspace */
        }
        strncpy(out, target, out_size - 1);
        out[out_size - 1] = '\0';
        return 0;
    }

    /* Parent exists. Check if it's within workspace. */
    size_t root_len = strlen(root);
    if (strncmp(parent_resolved, root, root_len) != 0 &&
        !(strcmp(parent_resolved, root) == 0)) {
        return -1; /* Parent is outside workspace */
    }

    /* Reconstruct full path with basename */
    char basename_buf[AGENT_PATH_MAX];
    strncpy(basename_buf, target, sizeof(basename_buf) - 1);
    basename_buf[sizeof(basename_buf) - 1] = '\0';
    char* filename = basename(basename_buf);

    snprintf(out, out_size, "%s/%s", parent_resolved, filename);
    return 0;
}

/* Create directory recursively (simple approach: mkdir -p style).
 * Returns 0 on success, -1 on failure. */
static int mkdir_recursive(const char* path)
{
    if (!path || path[0] == '\0') return -1;

    char buf[AGENT_PATH_MAX];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Find the parent directory */
    char* dir = dirname(buf);
    if (!dir || dir[0] == '\0' || strcmp(dir, ".") == 0 || strcmp(dir, "/") == 0) {
        return 0; /* parent is root or current dir, nothing to do */
    }

    /* Check if parent exists */
    struct stat st;
    if (stat(dir, &st) == 0) {
        return 0; /* parent exists */
    }

    /* Recursively create parent, then this directory */
    if (mkdir_recursive(dir) != 0) return -1;

    if (mkdir(path, 0755) != 0) return -1;
    return 0;
}

int tool_file_write_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024]        = "";
    char content[16384]    = "";
    char mode[16]          = "write";
    int  create_dirs       = 0;

    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "path",         path,         sizeof(path));
        tool_json_extract_str(json_args, "content",      content,      sizeof(content));
        tool_json_extract_str(json_args, "mode",         mode,         sizeof(mode));
        create_dirs = tool_json_extract_int(json_args, "create_dirs", 0);
    }

    /* Validate inputs */
    if (path[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"path is required\"}");
        return -1;
    }

    if (strcmp(mode, "write") != 0 && strcmp(mode, "append") != 0) {
        snprintf(output, output_size, "{\"error\":\"mode must be 'write' or 'append'\"}");
        return -1;
    }

    /* Resolve path within workspace for writing */
    char resolved_path[AGENT_PATH_MAX];
    if (resolve_write_path(path, resolved_path, sizeof(resolved_path)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or invalid\"}");
        return -1;
    }

    /* Create parent directories if requested */
    if (create_dirs) {
        char dir_path[AGENT_PATH_MAX];
        strncpy(dir_path, resolved_path, sizeof(dir_path) - 1);
        dir_path[sizeof(dir_path) - 1] = '\0';
        
        char* dir = dirname(dir_path);
        if (dir && dir[0] != '\0' && strcmp(dir, ".") != 0) {
            struct stat st;
            if (stat(dir, &st) != 0) {
                if (mkdir_recursive(dir) != 0) {
                    snprintf(output, output_size,
                             "{\"error\":\"failed to create parent directories\"}");
                    return -1;
                }
            }
        }
    }

    /* Open file for writing or appending */
    const char* fopen_mode = (strcmp(mode, "append") == 0) ? "ab" : "wb";
    FILE* fp = fopen(resolved_path, fopen_mode);
    if (!fp) {
        snprintf(output, output_size,
                 "{\"error\":\"cannot open file for writing: %s\"}", path);
        return -1;
    }

    /* Write content */
    size_t content_len = strlen(content);
    size_t written = fwrite(content, 1, content_len, fp);
    int write_error = (written != content_len);

    if (fclose(fp) != 0) write_error = 1;

    if (write_error) {
        snprintf(output, output_size,
                 "{\"error\":\"failed to write to file\"}");
        return -1;
    }

    /* Return success with file info */
    snprintf(output, output_size,
             "{\"success\":true,"
             "\"path\":\"%s\","
             "\"mode\":\"%s\","
             "\"bytes_written\":%zu}",
             path, mode, written);

    return 0;
}
