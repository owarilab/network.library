#define UNITY_INTERFACE_API __stdcall
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)

#ifdef __cplusplus
extern "C"
{
#endif

#pragma comment(lib,"ws2_32.lib")
#include "qs_api.h"

typedef struct {
    QS_MEMORY_CONTEXT main_memory;
} LIBQS_WORKING_DATA;

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init();
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_sum(int param1, int param2);
UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand();

#ifdef __cplusplus
}
#endif
