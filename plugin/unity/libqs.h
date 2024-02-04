
#ifdef _WINDOWS
#define UNITY_INTERFACE_API __stdcall
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)
#else
//#define UNITY_INTERFACE_API __attribute__((cdecl))
#define UNITY_INTERFACE_API 
#define UNITY_INTERFACE_EXPORT 
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WINDOWS
#pragma comment(lib,"ws2_32.lib")
#endif

#include "qs_api.h"

// unity c#側にchar*を受け取る関数の関数ポインタの宣言
typedef void(UNITY_INTERFACE_API *libqs_api_callback_unity_debug_log)(char* str);

// unity c#側にvoid*を受け取り、intを返す関数の関数ポインタの宣言
typedef int (UNITY_INTERFACE_API *libqs_api_callback_unity_on_recv_http_request)(void* http_request_parameter);

typedef struct {
    QS_MEMORY_CONTEXT main_memory;
    QS_MEMORY_CONTEXT temporary_memory;
    QS_SERVER_CONTEXT* server_context;
    libqs_api_callback_unity_debug_log debug_log;
    libqs_api_callback_unity_on_recv_http_request on_recv_http_request;
} LIBQS_WORKING_DATA;

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init();
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init_http_server(uint32_t server_port, uint32_t max_connection);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_update();
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_free();
UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand();
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_debug_log(libqs_api_callback_unity_debug_log callback);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_recv_http_request(libqs_api_callback_unity_on_recv_http_request callback);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_response(void* http_request_parameter,char* response);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_http_response_json(void* http_request_parameter,char* json);

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_method(void* http_request_parameter);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_path(void* http_request_parameter);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_get_parameter(void* http_request_parameter, char* name);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_parameter(void* http_request_parameter, char* name);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_body(void* http_request_parameter);

void libqs_api_debug_log(char* str);

#ifdef __cplusplus
}
#endif
