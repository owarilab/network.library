#include "tool_file_search.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

/* ---------------------------------------------------------------
 * Minimal JSON field extractors (same pattern as other tools)
 * --------------------------------------------------------------- */
static int extract_str(const char* json, const char* key, char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return 0;
    p++;
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
    if (*p == '"') return defaultval;
    return atoi(p);
}

/* JSON-escape src into dst. */
static void json_escape(const char* src, char* dst, size_t dst_size)
{
    size_t w = 0;
    for (size_t i = 0; src[i] && w + 2 < dst_size; i++) {
        char c = src[i];
        if      (c == '"')  { dst[w++] = '\\'; dst[w++] = '"';  }
        else if (c == '\\') { dst[w++] = '\\'; dst[w++] = '\\'; }
        else if (c == '\n') { dst[w++] = '\\'; dst[w++] = 'n';  }
        else if (c == '\r') { dst[w++] = '\\'; dst[w++] = 'r';  }
        else                { dst[w++] = c; }
    }
    dst[w] = '\0';
}

/* Append text to output buffer. Returns 0 on success, -1 on overflow. */
static int buf_append(char* buf, size_t buf_size, size_t* pos, const char* text)
{
    size_t len = strlen(text);
    if (*pos + len >= buf_size - 1) return -1;
    memcpy(buf + *pos, text, len);
    *pos += len;
    buf[*pos] = '\0';
    return 0;
}

/* ---------------------------------------------------------------
 * Recursive file search worker
 * Walks directory tree, collects paths matching pattern.
 * --------------------------------------------------------------- */
typedef struct {
    const char* pattern;      /* fnmatch pattern for filename */
    int         recursive;
    int         max_results;
    char*       out_buf;      /* JSON array content (no brackets) */
    size_t      out_size;
    size_t      out_pos;
    int         count;
    int         truncated;
    const char* workspace_root; /* for computing relative paths */
    size_t      workspace_root_len;
} SEARCH_CTX;

static void search_recursive(SEARCH_CTX* ctx, const char* abs_dir)
{
    if (ctx->truncated) return;

    DIR* d = opendir(abs_dir);
    if (!d) return;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (ctx->truncated) break;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full[AGENT_PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", abs_dir, entry->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        int is_dir = S_ISDIR(st.st_mode);

        if (!is_dir) {
            /* Apply filename pattern */
            if (ctx->pattern[0] && strcmp(ctx->pattern, "*") != 0) {
                if (fnmatch(ctx->pattern, entry->d_name, 0) != 0) continue;
            }

            /* Build workspace-relative path */
            const char* rel = full;
            if (strncmp(full, ctx->workspace_root, ctx->workspace_root_len) == 0) {
                rel = full + ctx->workspace_root_len;
                while (*rel == '/') rel++;
            }

            /* Append to JSON array */
            char esc[AGENT_PATH_MAX * 2];
            json_escape(rel, esc, sizeof(esc));

            char item[AGENT_PATH_MAX * 2 + 4];
            if (ctx->count > 0)
                snprintf(item, sizeof(item), ",\"%s\"", esc);
            else
                snprintf(item, sizeof(item), "\"%s\"", esc);

            if (buf_append(ctx->out_buf, ctx->out_size, &ctx->out_pos, item) != 0) {
                ctx->truncated = 1;
                break;
            }

            ctx->count++;
            if (ctx->max_results > 0 && ctx->count >= ctx->max_results) {
                ctx->truncated = 1;
                break;
            }
        }

        if (is_dir && ctx->recursive) {
            search_recursive(ctx, full);
        }
    }
    closedir(d);
}

/* ---------------------------------------------------------------
 * Public entry point
 * --------------------------------------------------------------- */
int tool_file_search_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024]   = ".";
    char pattern[256] = "*";
    int  recursive    = 1;
    int  max_results  = 50;

    if (json_args && json_args[0]) {
        extract_str(json_args, "path",    path,    sizeof(path));
        extract_str(json_args, "pattern", pattern, sizeof(pattern));
        recursive   = extract_int(json_args, "recursive",   1);
        max_results = extract_int(json_args, "max_results", 50);
    }
    if (path[0] == '\0')    { path[0]    = '.'; path[1]    = '\0'; }
    if (pattern[0] == '\0') { pattern[0] = '*'; pattern[1] = '\0'; }
    if (max_results <= 0 || max_results > 500) max_results = 50;

    /* Workspace validation */
    char resolved[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved, sizeof(resolved)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    /* Allocate array buffer (slightly smaller than output to leave room for wrapper) */
    size_t arr_size = output_size > 256 ? output_size - 256 : 1024;
    char*  arr_buf  = (char*)calloc(1, arr_size);
    if (!arr_buf) {
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    const char* root     = agent_get_workspace_root();
    size_t      root_len = strlen(root);

    SEARCH_CTX ctx = {
        .pattern           = pattern,
        .recursive         = recursive,
        .max_results       = max_results,
        .out_buf           = arr_buf,
        .out_size          = arr_size,
        .out_pos           = 0,
        .count             = 0,
        .truncated         = 0,
        .workspace_root    = root,
        .workspace_root_len = root_len,
    };

    search_recursive(&ctx, resolved);

    /* JSON-escape inputs for output */
    char esc_path[2048];
    char esc_pat[512];
    json_escape(path,    esc_path, sizeof(esc_path));
    json_escape(pattern, esc_pat,  sizeof(esc_pat));

    snprintf(output, output_size,
        "{\"matches\":[%s],"
        "\"count\":%d,"
        "\"truncated\":%s,"
        "\"query_path\":\"%s\","
        "\"query_pattern\":\"%s\"}",
        arr_buf,
        ctx.count,
        ctx.truncated ? "true" : "false",
        esc_path,
        esc_pat);

    free(arr_buf);
    return 0;
}
