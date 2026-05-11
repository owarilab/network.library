#include "tool_file_list.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

/* Recursive directory listing into output buffer. */
static int list_dir_recursive(
    const char* base_path,
    const char* rel_prefix,
    const char* pattern,
    int         recursive,
    char*       entries_buf,  /* JSON array content (no brackets) */
    size_t      entries_size,
    size_t*     entries_pos,
    int*        is_dir_buf,   /* parallel boolean array */
    int*        count)
{
    DIR* d = opendir(base_path);
    if (!d) return -1;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* Build full path for stat */
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        int is_directory = S_ISDIR(st.st_mode) ? 1 : 0;

        /* Build relative display name */
        char rel_name[4096];
        if (rel_prefix && rel_prefix[0]) {
            snprintf(rel_name, sizeof(rel_name), "%s/%s", rel_prefix, entry->d_name);
        } else {
            snprintf(rel_name, sizeof(rel_name), "%s", entry->d_name);
        }

        /* Pattern filter (apply only to files, not directories) */
        if (!is_directory && pattern && pattern[0] && strcmp(pattern, "*") != 0) {
            if (fnmatch(pattern, entry->d_name, 0) != 0) {
                /* Recurse into subdirs even if they don't match pattern */
                if (recursive && is_directory) goto recurse;
                continue;
            }
        }

        /* Append to entries JSON array */
        if (*count > 0) {
            tool_buf_append(entries_buf, entries_size, entries_pos, ",");
        }

        char escaped[512];
        tool_json_escape(rel_name, escaped, sizeof(escaped));

        char item[600];
        snprintf(item, sizeof(item), "\"%s\"", escaped);
        if (tool_buf_append(entries_buf, entries_size, entries_pos, item) != 0) {
            closedir(d);
            return -1; /* overflow */
        }

        is_dir_buf[*count] = is_directory;
        (*count)++;

        recurse:
        if (recursive && is_directory) {
            list_dir_recursive(full_path, rel_name, pattern,
                               recursive, entries_buf, entries_size,
                               entries_pos, is_dir_buf, count);
        }
    }
    closedir(d);
    return 0;
}

int tool_file_list_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024]    = ".";
    char pattern[256]  = "*";
    int  recursive     = 0;

    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "path",    path,    sizeof(path));
        tool_json_extract_str(json_args, "pattern", pattern, sizeof(pattern));
        recursive = tool_json_extract_int(json_args, "recursive", 0);
    }

    /* Validate path and resolve inside configured workspace */
    if (path[0] == '\0') {
        path[0] = '.'; path[1] = '\0';
    }
    if (pattern[0] == '\0') {
        pattern[0] = '*'; pattern[1] = '\0';
    }

    char resolved_path[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved_path, sizeof(resolved_path)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    /* Allocate working buffers */
    size_t  entries_size = output_size > 64 ? output_size - 64 : 1024;
    char*   entries_buf  = (char*)calloc(1, entries_size);
    int*    is_dir_buf   = (int*)calloc(output_size / 4, sizeof(int));
    int     max_entries  = (int)(output_size / 4);

    if (!entries_buf || !is_dir_buf) {
        free(entries_buf); free(is_dir_buf);
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    size_t entries_pos = 0;
    int    count       = 0;
    (void)max_entries;

    int ret = list_dir_recursive(resolved_path, "", pattern, recursive,
                                 entries_buf, entries_size,
                                 &entries_pos, is_dir_buf, &count);

    if (ret != 0) {
        free(entries_buf); free(is_dir_buf);
        snprintf(output, output_size, "{\"error\":\"cannot open directory: %s\"}", path);
        return -1;
    }

    /* Build is_directory JSON array */
    size_t isdir_size = (size_t)(count * 3 + 4);
    char*  isdir_buf  = (char*)calloc(1, isdir_size);
    if (!isdir_buf) {
        free(entries_buf); free(is_dir_buf);
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }
    isdir_buf[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(isdir_buf, ",");
        strcat(isdir_buf, is_dir_buf[i] ? "1" : "0");
    }

    /* Compose final JSON */
    int written = snprintf(output, output_size,
        "{\"entries\":[%s],\"is_directory\":[%s],\"count\":%d,\"path\":\"%s\"}",
        entries_buf, isdir_buf, count, path);

    free(entries_buf);
    free(is_dir_buf);
    free(isdir_buf);

    return (written > 0 && (size_t)written < output_size) ? 0 : -1;
}
