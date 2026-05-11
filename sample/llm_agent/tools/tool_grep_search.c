#include "tool_grep_search.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

/* Case-insensitive strstr (simple byte scan). */
static const char* str_istr(const char* haystack, const char* needle)
{
    if (!needle[0]) return haystack;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            size_t i = 1;
            while (i < nlen &&
                   tolower((unsigned char)haystack[i]) == tolower((unsigned char)needle[i]))
                i++;
            if (i == nlen) return haystack;
        }
    }
    return NULL;
}

/* ---------------------------------------------------------------
 * Search context
 * --------------------------------------------------------------- */
typedef struct {
    const char* query;          /* substring to search for */
    const char* file_pattern;   /* fnmatch against filename (e.g. "*.c") */
    int         recursive;
    int         case_sensitive;
    int         max_results;
    char*       out_buf;        /* JSON array content (no brackets) */
    size_t      out_size;
    size_t      out_pos;
    int         count;          /* total match lines found */
    int         truncated;
    const char* workspace_root;
    size_t      workspace_root_len;
} GREP_CTX;

/* Search a single file for lines matching ctx->query.
 * rel_path: workspace-relative path for reporting. */
static void grep_file(GREP_CTX* ctx, const char* abs_path, const char* rel_path)
{
    if (ctx->truncated) return;

    FILE* f = fopen(abs_path, "r");
    if (!f) return;

    char line[4096];
    int  lineno = 0;
    char esc_path[AGENT_PATH_MAX * 2];
    char esc_line[sizeof(line) * 2];
    char item[sizeof(line) * 2 + AGENT_PATH_MAX * 2 + 64];

    tool_json_escape(rel_path, esc_path, sizeof(esc_path));

    while (fgets(line, sizeof(line), f)) {
        lineno++;
        /* Strip trailing newline for output */
        size_t llen = strlen(line);
        while (llen > 0 && (line[llen-1] == '\n' || line[llen-1] == '\r'))
            line[--llen] = '\0';

        int matched;
        if (ctx->case_sensitive)
            matched = (strstr(line, ctx->query) != NULL);
        else
            matched = (str_istr(line, ctx->query) != NULL);

        if (!matched) continue;

        tool_json_escape(line, esc_line, sizeof(esc_line));
        if (ctx->count > 0)
            snprintf(item, sizeof(item),
                     ",{\"file\":\"%s\",\"line\":%d,\"text\":\"%s\"}",
                     esc_path, lineno, esc_line);
        else
            snprintf(item, sizeof(item),
                     "{\"file\":\"%s\",\"line\":%d,\"text\":\"%s\"}",
                     esc_path, lineno, esc_line);

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
    fclose(f);
}

/* Recursive directory walker. */
static void grep_recursive(GREP_CTX* ctx, const char* abs_dir)
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

        if (S_ISDIR(st.st_mode)) {
            if (ctx->recursive) grep_recursive(ctx, full);
            continue;
        }

        /* Apply file pattern filter */
        if (ctx->file_pattern[0] && strcmp(ctx->file_pattern, "*") != 0) {
            if (fnmatch(ctx->file_pattern, entry->d_name, 0) != 0) continue;
        }

        /* Build workspace-relative path */
        const char* rel = full;
        if (strncmp(full, ctx->workspace_root, ctx->workspace_root_len) == 0) {
            rel = full + ctx->workspace_root_len;
            while (*rel == '/') rel++;
        }

        grep_file(ctx, full, rel);
    }
    closedir(d);
}

/* ---------------------------------------------------------------
 * Public entry point
 * --------------------------------------------------------------- */
int tool_grep_search_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char query[512]        = "";
    char path[1024]        = ".";
    char file_pattern[256] = "*";
    int  recursive         = 1;
    int  case_sensitive    = 0;
    int  max_results       = 50;

    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "pattern",      query,        sizeof(query));
        tool_json_extract_str(json_args, "path",         path,         sizeof(path));
        tool_json_extract_str(json_args, "file_pattern", file_pattern, sizeof(file_pattern));
        recursive      = tool_json_extract_int(json_args, "recursive",      1);
        case_sensitive = tool_json_extract_int(json_args, "case_sensitive", 0);
        max_results    = tool_json_extract_int(json_args, "max_results",    50);
    }

    if (query[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"pattern is required\"}");
        return -1;
    }
    if (path[0] == '\0') { path[0] = '.'; path[1] = '\0'; }
    if (file_pattern[0] == '\0') { file_pattern[0] = '*'; file_pattern[1] = '\0'; }
    if (max_results <= 0 || max_results > 500) max_results = 50;

    /* Workspace validation */
    char resolved[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved, sizeof(resolved)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    /* Allocate array buffer */
    size_t arr_size = output_size > 512 ? output_size - 512 : 4096;
    char*  arr_buf  = (char*)calloc(1, arr_size);
    if (!arr_buf) {
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    const char* root     = agent_get_workspace_root();
    size_t      root_len = strlen(root);

    GREP_CTX ctx = {
        .query              = query,
        .file_pattern       = file_pattern,
        .recursive          = recursive,
        .case_sensitive     = case_sensitive,
        .max_results        = max_results,
        .out_buf            = arr_buf,
        .out_size           = arr_size,
        .out_pos            = 0,
        .count              = 0,
        .truncated          = 0,
        .workspace_root     = root,
        .workspace_root_len = root_len,
    };

    /* Check if resolved is a file or directory */
    struct stat st;
    if (stat(resolved, &st) == 0 && S_ISREG(st.st_mode)) {
        const char* rel = resolved;
        if (strncmp(resolved, root, root_len) == 0) {
            rel = resolved + root_len;
            while (*rel == '/') rel++;
        }
        grep_file(&ctx, resolved, rel);
    } else {
        grep_recursive(&ctx, resolved);
    }

    /* JSON-escape inputs for output */
    char esc_query[1024];
    char esc_path[2048];
    char esc_fpat[512];
    tool_json_escape(query,        esc_query, sizeof(esc_query));
    tool_json_escape(path,         esc_path,  sizeof(esc_path));
    tool_json_escape(file_pattern, esc_fpat,  sizeof(esc_fpat));

    snprintf(output, output_size,
        "{\"matches\":[%s],"
        "\"count\":%d,"
        "\"truncated\":%s,"
        "\"query_pattern\":\"%s\","
        "\"query_path\":\"%s\","
        "\"query_file_pattern\":\"%s\"}",
        arr_buf,
        ctx.count,
        ctx.truncated ? "true" : "false",
        esc_query,
        esc_path,
        esc_fpat);

    free(arr_buf);
    return 0;
}
