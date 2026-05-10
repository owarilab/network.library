#ifndef TOOL_GREP_SEARCH_H
#define TOOL_GREP_SEARCH_H

#include <stddef.h>

/* Execute grep_search tool.
 * json_args: {"path":".","pattern":"TODO","file_pattern":"*.c",
 *             "recursive":1,"case_sensitive":0,"max_results":50}
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_grep_search_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_GREP_SEARCH_H */
