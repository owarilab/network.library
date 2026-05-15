#ifndef TOOL_URL_WHITELIST_GET_H
#define TOOL_URL_WHITELIST_GET_H

#include <stddef.h>

/* url_whitelist_get tool — list allowed hosts for http_request */
int tool_url_whitelist_get_execute(const char* json_args, char* output, size_t output_size);

#endif /* TOOL_URL_WHITELIST_GET_H */
