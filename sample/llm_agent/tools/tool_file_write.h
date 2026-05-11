#ifndef TOOL_FILE_WRITE_H
#define TOOL_FILE_WRITE_H

#include <stddef.h>

/* Execute file_write tool.
 * json_args: {
 *   "path": "./file.txt",
 *   "content": "content to write",
 *   "mode": "write" or "append",
 *   "create_dirs": 0 or 1
 * }
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_file_write_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_WRITE_H */
