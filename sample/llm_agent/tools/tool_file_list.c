#include "tool_file_list.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

/* ---------------------------------------------------------------
 * Minimal JSON string field extractor.
 * Finds "key":"value" and copies value into out.
 * Returns 1 if found, 0 if not.
 * --------------------------------------------------------------- */
static int extract_str(const char* json, const char* key, char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == ':' || *p == ' ') {
        if (*p == ':') { p++; break; }
        p++;
    }
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return 0;
    p++;
    size_t i = 0;
    while (*p && i < out_size - 1) {
        if (*p == '\\' && *(p+1)) { p++; out[i++] = *p++; }
        else if (*p == '"') break;
        else out[i++] = *p++;
    }
    out[i] = '\0';
    return 1;
}

/* Extract integer field: "key": N  Returns defaultval if not found. */
static int extract_int(const char* json, const char* key, int defaultval)
{
    if (!json || !key) return defaultval;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return defaultval;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return defaultval;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') return defaultval; /* string, not int */
    return atoi(p);
}

/* Append text to output buffer, returns 0 on success, -1 if overflow. */
static int buf_append(char* buf, size_t buf_size, size_t* pos, const char* text)
{
    size_t len = strlen(text);
    if (*pos + len >= buf_size - 1) return -1;
    memcpy(buf + *pos, text, len);
    *pos += len;
    buf[*pos] = '\0';
    return 0;
}

/* JSON-escape src into dst (caller must ensure dst is large enough: len*2+1). */
static void json_escape_name(const char* src, char* dst, size_t dst_size)
{
    size_t w = 0;
    for (size_t i = 0; src[i] && w < dst_size - 2; i++) {
        char c = src[i];
        if (c == '"' || c == '\\') { dst[w++] = '\\'; dst[w++] = c; }
        else if (c == '\n')        { dst[w++] = '\\'; dst[w++] = 'n'; }
        else if (c == '\r')        { dst[w++] = '\\'; dst[w++] = 'r'; }
        else                        { dst[w++] = c; }
    }
    dst[w] = '\0';
}

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
            buf_append(entries_buf, entries_size, entries_pos, ",");
        }

        char escaped[512];
        json_escape_name(rel_name, escaped, sizeof(escaped));

        char item[600];
        snprintf(item, sizeof(item), "\"%s\"", escaped);
        if (buf_append(entries_buf, entries_size, entries_pos, item) != 0) {
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
        extract_str(json_args, "path",    path,    sizeof(path));
        extract_str(json_args, "pattern", pattern, sizeof(pattern));
        recursive = extract_int(json_args, "recursive", 0);
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
