using System;

namespace QS.Native
{
    public static class LibQsServerApi
    {
        public static int Initialize(uint port, uint maxConnection, LibQsDebugLogCallback debugLog, LibQsOnRecvHttpRequestCallback onHttpRequest)
        {
            var initResult = LibQsNative.libqs_api_init();
            if (initResult < 0)
            {
                return initResult;
            }

            LibQsCallbackRegistry.SetDebugLogCallback(debugLog);
            var serverResult = LibQsNative.libqs_api_init_http_server(port, maxConnection);
            if (serverResult < 0)
            {
                return serverResult;
            }

            LibQsCallbackRegistry.SetHttpRequestCallback(onHttpRequest);
            return serverResult;
        }

        public static void Update()
        {
            LibQsNative.libqs_api_update();
        }

        public static void Shutdown()
        {
            LibQsCallbackRegistry.Clear();
            LibQsNative.libqs_api_free();
        }

        public static string GetHttpMethod(IntPtr request)
        {
            return LibQsMarshal.PtrToString(LibQsNative.libqs_api_get_http_method(request));
        }

        public static string GetHttpPath(IntPtr request)
        {
            return LibQsMarshal.PtrToString(LibQsNative.libqs_api_get_http_path(request));
        }

        public static string GetHttpPostBody(IntPtr request)
        {
            return LibQsMarshal.PtrToString(LibQsNative.libqs_api_get_http_post_body(request));
        }

        public static int SendJsonResponse(IntPtr request, string json)
        {
            return LibQsNative.libqs_api_send_http_response_json(request, json);
        }

        public static int SendTextResponse(IntPtr request, string text)
        {
            return LibQsNative.libqs_api_send_response(request, text);
        }
    }
}
