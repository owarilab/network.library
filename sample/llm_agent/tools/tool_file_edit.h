#ifndef TOOL_FILE_EDIT_H
#define TOOL_FILE_EDIT_H

#include <stddef.h>

/* Execute file_edit tool with old/new text replacement.
 * json_args: {
 *   "path":     "./file.c",
 *   "old_text": "text to find (must be unique in the file)",
 *   "new_text": "replacement text"
 * }
 * Returns 0 on success, -1 on error. */
int tool_file_edit_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_EDIT_H */
