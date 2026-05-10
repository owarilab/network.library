#ifndef TOOL_FILE_READ_H
#define TOOL_FILE_READ_H

#include <stddef.h>

/* Execute file_read tool.
 * json_args: {"path":"./main.c","start_line":1,"end_line":100}
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_file_read_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_READ_H */
