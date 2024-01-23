#include "libqs.h"

static LIBQS_WORKING_DATA libqs_working_data;

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init()
{
    return api_qs_init();
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_sum(int param1, int param2)
{
    return param1 + param2;
}

UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand()
{
    return api_qs_rand();
}
