using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;
using System;
using System.Linq;

namespace QS
{
    public class NetworkPlayerManager : MonoBehaviour
    {
        private static string serverUrl = "http://localhost:8080";

        //typedef void(UNITY_INTERFACE_API *libqs_api_callback_unity_debug_log)(const char* str);
        public delegate void libqs_api_callback_unity_debug_log(string str);

        //typedef int (UNITY_INTERFACE_API *libqs_api_callback_unity_on_recv_http_request)(void* http_request_parameter);
        public delegate int libqs_api_callback_unity_on_recv_http_request(IntPtr http_request_parameter);

        // UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_debug_log(libqs_api_callback_unity_debug_log callback);
        [DllImport("libqs.dll")]
        public static extern void libqs_api_set_debug_log(libqs_api_callback_unity_debug_log callback);

        // UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_recv_http_request(libqs_api_callback_unity_on_recv_http_request callback);
        [DllImport("libqs.dll")]
        public static extern void libqs_api_set_on_recv_http_request(libqs_api_callback_unity_on_recv_http_request callback);

        // UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_update();
        [DllImport("libqs.dll")]
        public static extern int libqs_api_update();

        // UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_free();
        [DllImport("libqs.dll")]
        public static extern int libqs_api_free();

        // UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init();
        [DllImport("libqs.dll")]
        public static extern int libqs_api_init();

        // UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init_http_server(uint32_t port, uint32_t max_connection);
        [DllImport("libqs.dll")]
        public static extern int libqs_api_init_http_server(uint port, uint max_connection);

        // UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_response(void* http_request_parameter, const char* response);
        [DllImport("libqs.dll")]
        public static extern int libqs_api_send_response(IntPtr http_request_parameter, IntPtr response);

        // UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_http_response_json(void* http_request_parameter,char* json);
        [DllImport("libqs.dll")]
        public static extern int libqs_api_send_http_response_json(IntPtr http_request_parameter, IntPtr json);

        //UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_method(void* http_request_parameter);
        [DllImport("libqs.dll")]
        public static extern IntPtr libqs_api_get_http_method(IntPtr http_request_parameter);

        //UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_path(void* http_request_parameter);
        [DllImport("libqs.dll")]
        public static extern IntPtr libqs_api_get_http_path(IntPtr http_request_parameter);

        //UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_get_parameter(void* http_request_parameter, char* name);
        [DllImport("libqs.dll")]
        public static extern IntPtr libqs_api_get_http_get_parameter(IntPtr http_request_parameter, IntPtr name);
        
        //UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_parameter(void* http_request_parameter, char* name);
        [DllImport("libqs.dll")]
        public static extern IntPtr libqs_api_get_http_post_parameter(IntPtr http_request_parameter, IntPtr name);

        //UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_body(void* http_request_parameter);
        [DllImport("libqs.dll")]
        public static extern IntPtr libqs_api_get_http_post_body(IntPtr http_request_parameter);

        // UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand()
        [DllImport("libqs.dll")]
        public static extern uint libqs_api_rand();

        static Dictionary<string, PlayerCharacterData> networkPlayerCharacterInfos = new Dictionary<string, PlayerCharacterData>();

        Dictionary<string, NetworkPlayerController> networkPlayerControllers = new Dictionary<string, NetworkPlayerController>();

        public GameObject networkPlayerPrefab;

        // serializable json response class
        [Serializable]
        public class HttpResponse
        {
            public string method;
            public string path;
            public string message;
            public string status;

            public List<PlayerCharacterData> players = new List<PlayerCharacterData>();

            public HttpResponse(string method, string path, string message, string status)
            {
                this.method = method;
                this.path = path;
                this.message = message;
                this.status = status;
            }
        }

        public static string ServerUrl
        {
            get { return serverUrl; }
        }

        // Start is called before the first frame update
        void Start()
        {
            NetworkPlayerManager.Initialize();
        }

        void LateUpdate()
        {
            // loop networkPlayerCharacterInfos
            foreach(KeyValuePair<string, PlayerCharacterData> pair in networkPlayerCharacterInfos)
            {
                string playerId = pair.Key;
                PlayerCharacterData playerCharacterData = pair.Value;
                if(networkPlayerControllers.ContainsKey(playerId))
                {
                    NetworkPlayerController networkPlayerController = networkPlayerControllers[playerId];
                    networkPlayerController.SetMoveToTransform(playerCharacterData.position, playerCharacterData.rotation, playerCharacterData.scale);

                    //networkPlayerController.transform.position = playerCharacterData.position;
                    //networkPlayerController.transform.rotation = playerCharacterData.rotation;
                    //networkPlayerController.transform.localScale = playerCharacterData.scale;
                }else{
                    GameObject networkPlayer = Instantiate(networkPlayerPrefab, playerCharacterData.position, playerCharacterData.rotation);
                    NetworkPlayerController networkPlayerController = networkPlayer.GetComponent<NetworkPlayerController>();
                    networkPlayerControllers.Add(playerId, networkPlayerController);
                }
            }
        }

        void FixedUpdate()
        {
            libqs_api_update();
        }

        // OnDestroy
        void OnDestroy()
        {
            Debug.Log("OnDestroy");
            libqs_api_free();
            System.GC.Collect();
            Resources.UnloadUnusedAssets();
        }

        public static void Initialize()
        {
            int result = libqs_api_init();
            Debug.Log("libqs_api_init() = " + result);
            libqs_api_set_debug_log(NetworkPlayerManager.DebugLog);

            result = libqs_api_init_http_server(8080, 100);
            Debug.Log("libqs_api_init_http_server(8080, 100) = " + result);
            libqs_api_set_on_recv_http_request(NetworkPlayerManager.OnRecvHttpRequest);

            uint rand = libqs_api_rand();
            Debug.Log("libqs_api_rand() = " + rand);
        }

        public static void DebugLog(string str)
        {
            Debug.Log("libqs_api_debug_log : " + str);
        }

        public static int OnRecvHttpRequest(IntPtr http_request_parameter)
        {
            int http_status_code = 404;
            IntPtr method_ptr = libqs_api_get_http_method(http_request_parameter);
            string method = Marshal.PtrToStringAnsi(method_ptr);
            //Debug.Log("method = " + method);
            IntPtr path_ptr = libqs_api_get_http_path(http_request_parameter);
            string path = Marshal.PtrToStringAnsi(path_ptr);
            //Debug.Log("path = " + path);
            if(method == "POST")
            {
                IntPtr post_body_ptr = libqs_api_get_http_post_body(http_request_parameter);
                string post_body = Marshal.PtrToStringAnsi(post_body_ptr);
                //Debug.Log("post_body = " + post_body);

                if(path=="/move")
                {
                    //Debug.Log("/move");
                    PlayerCharacterData playerCharacterData = JsonUtility.FromJson<PlayerCharacterData>(post_body);
                    //Debug.Log("playerCharacterData.playerId = " + playerCharacterData.playerId);
                    //Debug.Log("playerCharacterData.position = " + playerCharacterData.position);
                    //Debug.Log("playerCharacterData.rotation = " + playerCharacterData.rotation);
                    //Debug.Log("playerCharacterData.scale = " + playerCharacterData.scale);

                    networkPlayerCharacterInfos[playerCharacterData.playerId] = playerCharacterData;

                    HttpResponse httpResponse = new HttpResponse(method, path, "OK", "200");
                    string json = JsonUtility.ToJson(httpResponse);
                    IntPtr json_ptr = Marshal.StringToHGlobalAnsi(json);
                    return libqs_api_send_http_response_json(http_request_parameter, json_ptr);
                }
            }

            if(method == "GET")
            {
                if(path=="/player/sync")
                {
                    //Debug.Log("/player/sync");
                    HttpResponse httpResponse = new HttpResponse(method, path, "OK", "200");
                    httpResponse.players = networkPlayerCharacterInfos.Values.ToList();
                    string json = JsonUtility.ToJson(httpResponse);
                    IntPtr json_ptr = Marshal.StringToHGlobalAnsi(json);
                    return libqs_api_send_http_response_json(http_request_parameter, json_ptr);
                }
            }

            return http_status_code;
        }
    }
}
