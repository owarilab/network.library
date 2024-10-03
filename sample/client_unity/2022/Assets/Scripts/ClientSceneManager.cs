using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;
using System;

namespace QS
{
    public class ClientSceneManager : SceneManager
    {
        Dictionary<string, PlayerCharacterData> networkPlayerCharacterInfos = new Dictionary<string, PlayerCharacterData>();
        Dictionary<string, NetworkPlayerController> networkPlayerControllers = new Dictionary<string, NetworkPlayerController>();

        // Start is called before the first frame update
        void Start()
        {
            isSync = true;
            // プレイヤーを生成
            CreatePlayer();
            StartCoroutine(SyncPlayerCoroutine());
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
                    //networkPlayerController.SetNetworkPlayerManager(this);
                    networkPlayerControllers.Add(playerId, networkPlayerController);
                }
            }
        }

        private IEnumerator SyncPlayerCoroutine()
        {
            while (true)
            {
                yield return SendPlayerSyncRequest();
                yield return new WaitForSeconds(0.01f);
            }
            yield return null;
        }

        private IEnumerator SendPlayerSyncRequest()
        {
            string url = NetworkPlayerManager.ServerUrl + "/player/sync";
            UnityWebRequest request = new UnityWebRequest(url, "GET");
            request.downloadHandler = new DownloadHandlerBuffer();
            yield return request.SendWebRequest();
            if (request.result == UnityWebRequest.Result.ConnectionError || request.result == UnityWebRequest.Result.ProtocolError)
            {
                Debug.Log(request.error);
            }
            else
            {
                Debug.Log(request.downloadHandler.text);
                string json = request.downloadHandler.text;
                NetworkPlayerManager.HttpResponse httpResponse = JsonUtility.FromJson<NetworkPlayerManager.HttpResponse>(json);
                if(httpResponse.players!=null)
                {
                    foreach (PlayerCharacterData playerData in httpResponse.players)
                    {
                        if (playerData.playerId != playerController.PlayerId)
                        {
                            networkPlayerCharacterInfos[playerData.playerId] = playerData;
                        }
                    }
                }
            }
            request.Dispose();
        }
    }
}