#ifndef TOOL_FILE_PATCH_H
#define TOOL_FILE_PATCH_H

#include <stddef.h>

/* Execute file_patch tool with unified diff patch (patch command).
 * json_args: {
 *   "path": "./file.c",
 *   "patch": "--- a/file.c\n+++ b/file.c\n@@ -1,3 +1,4 @@\n...",
 *   "dry_run": 0 or 1
 * }
 * output   : JSON result written here
 * Returns 0 on success, -1 on error. */
int tool_file_patch_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_FILE_PATCH_H */
