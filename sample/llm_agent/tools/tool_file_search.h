#ifndef TOOL_FILE_SEARCH_H
#define TOOL_FILE_SEARCH_H

#include <stddef.h>

/* Execute file_search tool.
 * json_args: {"path":".","pattern":"*.c","recursive":1,"max_results":50}
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_file_search_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_SEARCH_H */
