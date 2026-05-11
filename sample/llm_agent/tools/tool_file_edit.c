#include "tool_file_edit.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tool_file_edit_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024] = "";
    char old_text[65536] = "";
    char new_text[65536] = "";

    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "path",     path,     sizeof(path));
        tool_json_extract_str(json_args, "old_text", old_text, sizeof(old_text));
        tool_json_extract_str(json_args, "new_text", new_text, sizeof(new_text));
    }

    /* Validate inputs */
    if (path[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"path is required\"}");
        return -1;
    }
    if (old_text[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"old_text is required\"}");
        return -1;
    }

    /* Resolve path within workspace */
    char resolved_path[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved_path, sizeof(resolved_path)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    /* Read entire file */
    FILE* fp = fopen(resolved_path, "rb");
    if (!fp) {
        snprintf(output, output_size, "{\"error\":\"cannot open file\"}");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < 0 || file_size > 4 * 1024 * 1024) {
        fclose(fp);
        snprintf(output, output_size, "{\"error\":\"file too large or unreadable\"}");
        return -1;
    }

    char* file_buf = (char*)malloc((size_t)file_size + 1);
    if (!file_buf) {
        fclose(fp);
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }
    size_t read_bytes = fread(file_buf, 1, (size_t)file_size, fp);
    fclose(fp);
    file_buf[read_bytes] = '\0';

    /* Find old_text in file content */
    const char* pos = strstr(file_buf, old_text);
    if (!pos) {
        free(file_buf);
        snprintf(output, output_size,
                 "{\"success\":0,\"error\":\"old_text not found in file\"}");
        return -1;
    }

    /* Check for duplicate matches — require unique context */
    if (strstr(pos + 1, old_text) != NULL) {
        free(file_buf);
        snprintf(output, output_size,
                 "{\"success\":0,\"error\":\"old_text matches multiple locations — provide more context\"}");
        return -1;
    }

    /* Build new content: prefix + new_text + suffix */
    size_t prefix_len = (size_t)(pos - file_buf);
    size_t old_len    = strlen(old_text);
    size_t new_len    = strlen(new_text);
    size_t suffix_len = read_bytes - prefix_len - old_len;
    size_t new_size   = prefix_len + new_len + suffix_len;

    char* new_buf = (char*)malloc(new_size + 1);
    if (!new_buf) {
        free(file_buf);
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    memcpy(new_buf,                        file_buf,          prefix_len);
    memcpy(new_buf + prefix_len,           new_text,          new_len);
    memcpy(new_buf + prefix_len + new_len, pos + old_len,     suffix_len);
    new_buf[new_size] = '\0';

    free(file_buf);

    /* Write new content back */
    fp = fopen(resolved_path, "wb");
    if (!fp) {
        free(new_buf);
        snprintf(output, output_size, "{\"error\":\"cannot write file\"}");
        return -1;
    }
    size_t written = fwrite(new_buf, 1, new_size, fp);
    fclose(fp);
    free(new_buf);

    if (written != new_size) {
        snprintf(output, output_size, "{\"error\":\"write incomplete\"}");
        return -1;
    }

    snprintf(output, output_size,
             "{\"success\":1,\"path\":\"%s\",\"bytes_written\":%zu}",
             path, written);
    return 0;
}
