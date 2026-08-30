#ifndef TOOL_FILE_INFO_H
#define TOOL_FILE_INFO_H

#include <stddef.h>

/* Execute file_info tool.
 * json_args: {"path":"file.txt","include_line_count":0}
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_file_info_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_INFO_H */