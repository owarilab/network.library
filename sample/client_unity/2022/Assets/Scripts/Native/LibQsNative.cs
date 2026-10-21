using System;
using System.Runtime.InteropServices;

namespace QS.Native
{
    public static class LibQsNative
    {
        private const string LibraryName = "libqs";

#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
        private const CallingConvention NativeCallingConvention = CallingConvention.StdCall;
#else
        private const CallingConvention NativeCallingConvention = CallingConvention.Cdecl;
#endif

        // Lifecycle
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_init();

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_init_http_server(uint server_port, uint max_connection);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_update();

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_free();

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern uint libqs_api_rand();

        // Callback registration
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern void libqs_api_set_debug_log(LibQsDebugLogCallback callback);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern void libqs_api_set_on_recv_http_request(LibQsOnRecvHttpRequestCallback callback);

        // HTTP and response
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_get_http_method(IntPtr http_request_parameter);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_get_http_path(IntPtr http_request_parameter);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_get_http_get_parameter(IntPtr http_request_parameter, [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_get_http_post_parameter(IntPtr http_request_parameter, [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_get_http_post_body(IntPtr http_request_parameter);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_send_response(IntPtr http_request_parameter, [MarshalAs(UnmanagedType.LPStr)] string response);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_send_http_response_json(IntPtr http_request_parameter, [MarshalAs(UnmanagedType.LPStr)] string json);

        // JSON / Array / Object
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_object_create(IntPtr memory_context, IntPtr object_context);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_object_push_string(IntPtr object_context, [MarshalAs(UnmanagedType.LPStr)] string name, [MarshalAs(UnmanagedType.LPStr)] string value);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_object_push_integer(IntPtr object_context, [MarshalAs(UnmanagedType.LPStr)] string name, int value);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_json_encode_object(IntPtr object_context, UIntPtr buffer_size);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_json_decode_object(IntPtr memory_context, IntPtr object_context, [MarshalAs(UnmanagedType.LPStr)] string json);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_object_exist(IntPtr object_context, [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_object_get_string(IntPtr object_context, [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_object_get_integer_val(IntPtr object_context, [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_array_create(IntPtr memory_context, IntPtr array_context);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_array_push_string(IntPtr array_context, [MarshalAs(UnmanagedType.LPStr)] string value);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_array_get_length(IntPtr array_context);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_array_get_string(IntPtr array_context, int offset);

        // KVS
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_kvs_create_b1mb(IntPtr memory_context, IntPtr kvs_context);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_kvs_set(IntPtr kvs_context, [MarshalAs(UnmanagedType.LPStr)] string key, [MarshalAs(UnmanagedType.LPStr)] string value, int life_time);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_kvs_get(IntPtr kvs_context, [MarshalAs(UnmanagedType.LPStr)] string key);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_kvs_delete(IntPtr kvs_context, [MarshalAs(UnmanagedType.LPStr)] string key);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_kvs_key_length(IntPtr kvs_context);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_persistence_kvs_memory_free(IntPtr kvs_context);

        // Logger
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_create(IntPtr memory_context, IntPtr logger_context, int level);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_open(IntPtr logger_context, [MarshalAs(UnmanagedType.LPStr)] string file_path);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_close(IntPtr logger_context);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_set_level(IntPtr logger_context, int level);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_debug(IntPtr logger_context, [MarshalAs(UnmanagedType.LPStr)] string message);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_info(IntPtr logger_context, [MarshalAs(UnmanagedType.LPStr)] string message);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_warn(IntPtr logger_context, [MarshalAs(UnmanagedType.LPStr)] string message);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern int libqs_api_logger_error(IntPtr logger_context, [MarshalAs(UnmanagedType.LPStr)] string message);

        // Utility
        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_uniqid(IntPtr memory_context, int length);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_base64_encode(IntPtr memory_context, byte[] data, UIntPtr length);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_base64_decode(IntPtr memory_context, [MarshalAs(UnmanagedType.LPStr)] string data);

        [DllImport(LibraryName, CallingConvention = NativeCallingConvention)]
        public static extern IntPtr libqs_api_sha1_encode(IntPtr memory_context, byte[] data, UIntPtr length);
    }
}
