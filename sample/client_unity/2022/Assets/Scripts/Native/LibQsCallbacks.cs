using System;
using System.Runtime.InteropServices;

namespace QS.Native
{
#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
#else
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
#endif
    public delegate void LibQsDebugLogCallback(IntPtr message);

#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
#else
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
#endif
    public delegate int LibQsOnRecvHttpRequestCallback(IntPtr httpRequestParameter);

    // Keeps delegates alive while native code holds callback pointers.
    public static class LibQsCallbackRegistry
    {
        private static LibQsDebugLogCallback debugLogCallback;
        private static LibQsOnRecvHttpRequestCallback httpRequestCallback;

        public static void SetDebugLogCallback(LibQsDebugLogCallback callback)
        {
            debugLogCallback = callback;
            LibQsNative.libqs_api_set_debug_log(debugLogCallback);
        }

        public static void SetHttpRequestCallback(LibQsOnRecvHttpRequestCallback callback)
        {
            httpRequestCallback = callback;
            LibQsNative.libqs_api_set_on_recv_http_request(httpRequestCallback);
        }

        public static void Clear()
        {
            debugLogCallback = null;
            httpRequestCallback = null;
            LibQsNative.libqs_api_set_debug_log(null);
            LibQsNative.libqs_api_set_on_recv_http_request(null);
        }
    }
}
