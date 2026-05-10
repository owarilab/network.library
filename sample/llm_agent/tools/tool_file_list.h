#ifndef TOOL_FILE_LIST_H
#define TOOL_FILE_LIST_H

#include <stddef.h>

/* Execute file_list tool.
 * json_args: {"path":"./","recursive":0,"pattern":"*"}
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_file_list_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_LIST_H */
