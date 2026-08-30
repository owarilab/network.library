#include "tool_file_info.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_INFO_SAMPLE_SIZE 4096

static const char* file_extension(const char* path)
{
    const char* slash = strrchr(path, '/');
    const char* dot = strrchr(path, '.');
    if (!dot || (slash && dot < slash) || dot == path || dot[1] == '\0') return "";
    return dot;
}

static int detect_binary(const char* path, int* is_binary)
{
    unsigned char sample[FILE_INFO_SAMPLE_SIZE];
    FILE* fp = fopen(path, "rb");
    if (!fp) return -1;

    size_t length = fread(sample, 1, sizeof(sample), fp);
    fclose(fp);

    *is_binary = 0;
    for (size_t i = 0; i < length; i++) {
        if (sample[i] == 0) {
            *is_binary = 1;
            break;
        }
    }
    return 0;
}

static int count_text_lines(const char* path, long long* line_count)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) return -1;

    unsigned char buffer[8192];
    size_t length;
    long long count = 0;
    int has_content = 0;
    int last_byte = '\n';

    while ((length = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        has_content = 1;
        for (size_t i = 0; i < length; i++) {
            if (buffer[i] == '\n') count++;
            last_byte = buffer[i];
        }
    }
    int read_error = ferror(fp);
    fclose(fp);
    if (read_error) return -1;

    if (has_content && last_byte != '\n') count++;
    *line_count = count;
    return 0;
}

int tool_file_info_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    char path[1024] = "";
    int include_line_count = 0;
    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "path", path, sizeof(path));
        include_line_count = tool_json_extract_int(json_args, "include_line_count", 0);
    }
    if (!path[0]) {
        snprintf(output, output_size, "{\"error\":\"path is required\"}");
        return -1;
    }

    char resolved_path[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved_path, sizeof(resolved_path)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    struct stat st;
    if (stat(resolved_path, &st) != 0) {
        snprintf(output, output_size, "{\"error\":\"cannot stat file: %s\"}", path);
        return -1;
    }

    int is_directory = S_ISDIR(st.st_mode) ? 1 : 0;
    int readable = access(resolved_path, R_OK) == 0 ? 1 : 0;
    int is_binary = 0;
    if (!is_directory && readable && detect_binary(resolved_path, &is_binary) != 0) {
        readable = 0;
    }

    long long line_count = 0;
    int line_count_available = 0;
    if (include_line_count && !is_directory && readable && !is_binary &&
        count_text_lines(resolved_path, &line_count) == 0) {
        line_count_available = 1;
    }

    char escaped_path[2048];
    char escaped_extension[256];
    tool_json_escape(path, escaped_path, sizeof(escaped_path));
    tool_json_escape(file_extension(path), escaped_extension, sizeof(escaped_extension));

    int written = snprintf(output, output_size,
        "{\"path\":\"%s\",\"exists\":true,\"is_directory\":%s,"
        "\"file_size\":%lld,\"readable\":%s,\"is_binary\":%s,"
        "\"binary_detection_method\":\"%s\",\"extension\":\"%s\","
        "\"line_count\":%s,\"line_count_available\":%s}",
        escaped_path,
        is_directory ? "true" : "false",
        (long long)st.st_size,
        readable ? "true" : "false",
        is_binary ? "true" : "false",
        is_binary ? "nul_byte" : "sample_without_nul_byte",
        escaped_extension,
        line_count_available ? "0" : "null",
        line_count_available ? "true" : "false");

    if (line_count_available) {
        written = snprintf(output, output_size,
            "{\"path\":\"%s\",\"exists\":true,\"is_directory\":%s,"
            "\"file_size\":%lld,\"readable\":%s,\"is_binary\":%s,"
            "\"binary_detection_method\":\"%s\",\"extension\":\"%s\","
            "\"line_count\":%lld,\"line_count_available\":true}",
            escaped_path,
            is_directory ? "true" : "false",
            (long long)st.st_size,
            readable ? "true" : "false",
            is_binary ? "true" : "false",
            is_binary ? "nul_byte" : "sample_without_nul_byte",
            escaped_extension,
            line_count);
    }

    return (written > 0 && (size_t)written < output_size) ? 0 : -1;
}