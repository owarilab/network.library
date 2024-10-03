using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;
using System;

namespace QS
{
    public class PlayerController : MonoBehaviour
    {
        protected string playerId;
        public GameObject characterObject;
        public GameObject playerCameraObject;
        private Character character;
        private Camera playerCamera;
        public float cameraRadius = 10.0f;
        public float lookH = 0.0f;
        public float cameraSpeed = 0.1f;
        public float playerSpeed = 1f;
        public float cameraZoomSpeed = 5.0f;

        private Vector3 move = new Vector3(0, 0, 0);
        private Quaternion quaternion = new Quaternion(0, 0, 0, 0);

        private bool isSync = false;

        public void Initialize(GameObject cameraObject)
        {
            // player prefsからplayerIdを取得
            this.playerId = PlayerPrefs.GetString("playerId");
            if(this.playerId == "")
            {
                this.playerId = System.Guid.NewGuid().ToString();
                PlayerPrefs.SetString("playerId", this.playerId);
            }

            Debug.Log("PlayerId: " + this.playerId);

            this.character = characterObject.GetComponent<Character>();
            this.playerCameraObject = cameraObject;
            this.playerCamera = cameraObject.GetComponent<Camera>();

            SendMove();
        }

        public string PlayerId
        {
            get { return playerId; }
        }

        public void SetIsSync(bool isSync)
        {
            this.isSync = isSync;
        }

        public void SendMove()
        {
            if (!isSync)
            {
                return;
            }
            // シリアライズしたデータを送信する
            PlayerCharacterData st = new PlayerCharacterData(playerId, this.transform);
            StartCoroutine(SendHttpRequestPostJson(NetworkPlayerManager.ServerUrl + "/move", JsonUtility.ToJson(st)));
        }

        void Start()
        {

        }

        void LateUpdate()
        {
            // ボールを投げる
            //if (Input.GetMouseButtonDown(0))
            if (Input.GetMouseButton(0))
            {
                this.character.Action();
            }

            // PS4R2でボールを投げる
            float r2 = Input.GetAxis("PS4R2");
            if (r2 > 0.5)
            {
                this.character.Action();
            }

            // マウスのドラッグでカメラを回転させる
            if (Input.GetMouseButton(1))
            {
                float mouse_x = Input.GetAxis("Mouse X") * cameraSpeed;
                lookH += mouse_x;
            }

            // PS4RStickXでカメラを回転させる
            float rstick_x = Input.GetAxis("PS4RStickX") * cameraSpeed * 0.1f;
            lookH += rstick_x;


            // スペースキーを押している間ジャンプ
            if (Input.GetKey(KeyCode.Space))
            {
                this.character.Jump();
            }

            // PS4Jumpでジャンプ
            if (Input.GetButtonDown("PS4Jump"))
            {
                this.character.Jump();
            }

            // キャラクターの移動
            float x = Input.GetAxis("Horizontal");
            float z = Input.GetAxis("Vertical");
            if(x != 0 || z != 0)
            {
                move.x = x;
                move.z = z;
                move.y = 0;
                move = playerCameraObject.transform.TransformDirection(move) * playerSpeed * Time.deltaTime;
                move.y = 0;

                this.character.Move(move);

                if (move != Vector3.zero) {
                    quaternion = Quaternion.LookRotation(move - Vector3.zero, Vector3.up);
                    this.character.LookAt(quaternion);
                }

                SendMove();
            }
            
            if(this.character.IsJumping || this.character.IsFalling)
            {
                SendMove();
            }

            // カメラをプレイヤーを中心に半径cameraRadiusの周りをlookHの角度移動する
            float lx = cameraRadius * Mathf.Sin(lookH);
            float lz = cameraRadius * Mathf.Cos(lookH);
            Vector3 cameraLookPosition = new Vector3(
                lx + this.characterObject.transform.position.x,
                1.0f + this.characterObject.transform.position.y,
                lz + this.characterObject.transform.position.z
            );

            // カメラをキャラクターに近づける
            this.playerCameraObject.transform.position = Vector3.Lerp(this.playerCameraObject.transform.position, cameraLookPosition, Time.deltaTime * 10);
            this.playerCameraObject.transform.LookAt(this.characterObject.transform);

            // マウスのホイールでカメラのズームイン・ズームアウト
            float mouse_scroll = Input.GetAxis("Mouse ScrollWheel");
            this.playerCamera.fieldOfView -= mouse_scroll * cameraZoomSpeed;
        }

        private IEnumerator SendHttpRequestGet(string url)
        {
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
            }
            request.Dispose();
        }

        private IEnumerator SendHttpRequestPostJson(string url, string json)
        {
            UnityWebRequest request = new UnityWebRequest(url, "POST");
            byte[] bodyRaw = System.Text.Encoding.UTF8.GetBytes(json);
            request.uploadHandler = (UploadHandler)new UploadHandlerRaw(bodyRaw);
            request.downloadHandler = (DownloadHandler)new DownloadHandlerBuffer();
            request.SetRequestHeader("Content-Type", "application/json");

            yield return request.SendWebRequest();

            if (request.result == UnityWebRequest.Result.ConnectionError || request.result == UnityWebRequest.Result.ProtocolError)
            {
                Debug.Log(request.error);
            }
            else
            {
                Debug.Log(request.downloadHandler.text);
            }
            request.Dispose();
        }
    }
}