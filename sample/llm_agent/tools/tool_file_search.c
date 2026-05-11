#include "tool_file_search.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

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
            tool_json_escape(rel, esc, sizeof(esc));

            char item[AGENT_PATH_MAX * 2 + 4];
            if (ctx->count > 0)
                snprintf(item, sizeof(item), ",\"%s\"", esc);
            else
                snprintf(item, sizeof(item), "\"%s\"", esc);

            if (tool_buf_append(ctx->out_buf, ctx->out_size, &ctx->out_pos, item) != 0) {
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
        tool_json_extract_str(json_args, "path",    path,    sizeof(path));
        tool_json_extract_str(json_args, "pattern", pattern, sizeof(pattern));
        recursive   = tool_json_extract_int(json_args, "recursive",   1);
        max_results = tool_json_extract_int(json_args, "max_results", 50);
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
    tool_json_escape(path,    esc_path, sizeof(esc_path));
    tool_json_escape(pattern, esc_pat,  sizeof(esc_pat));

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
