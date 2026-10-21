using System;
using UnityEngine;
using QS.Native;

namespace QS.Samples
{
    public class QsApiSmokeTest : MonoBehaviour
    {
        [SerializeField] private int port = 8080;
        [SerializeField] private int maxConnection = 32;

        private bool started;

        private void Start()
        {
            int result = LibQsServerApi.Initialize((uint)port, (uint)maxConnection, OnDebugLog, OnHttpRequest);
            started = result >= 0;
            Debug.Log("QsApiSmokeTest.Initialize result=" + result);

            if (started)
            {
                Debug.Log("QsApiSmokeTest.rand=" + LibQsNative.libqs_api_rand());
            }
        }

        private void FixedUpdate()
        {
            if (!started)
            {
                return;
            }

            LibQsServerApi.Update();
        }

        private void OnDestroy()
        {
            if (!started)
            {
                return;
            }

            LibQsServerApi.Shutdown();
            started = false;
        }

        private static void OnDebugLog(IntPtr message)
        {
            Debug.Log("[libqs] " + LibQsMarshal.PtrToString(message));
        }

        private static int OnHttpRequest(IntPtr request)
        {
            string method = LibQsServerApi.GetHttpMethod(request);
            string path = LibQsServerApi.GetHttpPath(request);

            string json = "{\"status\":\"ok\",\"method\":\"" + method + "\",\"path\":\"" + path + "\"}";
            return LibQsServerApi.SendJsonResponse(request, json);
        }
    }
}
