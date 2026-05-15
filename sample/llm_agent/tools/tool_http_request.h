#ifndef TOOL_HTTP_REQUEST_H
#define TOOL_HTTP_REQUEST_H

#include <stddef.h>

/* http_request tool — make HTTP requests to whitelisted hosts */
int tool_http_request_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_HTTP_REQUEST_H */
