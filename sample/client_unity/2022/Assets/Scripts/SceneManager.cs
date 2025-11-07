/*
 * Copyright (c) Katsuya Owari
 */

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace QS
{
    public class SceneManager : MonoBehaviour
    {
        public GameObject cameraObject;
        public GameObject sponePoint;
        public GameObject playerPrefab;
        public GameObject networkPlayerPrefab;

        protected GameObject playerObject;
        protected PlayerController playerController;

        protected bool isSync = false;

        protected void CreatePlayer()
        {
            // プレイヤーを生成
            playerObject = Instantiate(playerPrefab, sponePoint.transform.position, Quaternion.identity);
            playerController = playerObject.GetComponent<PlayerController>();
            playerController.Initialize(cameraObject);
            playerController.SetIsSync(isSync);
        }

        // Start is called before the first frame update
        void Start()
        {
            // プレイヤーを生成
            CreatePlayer();
        }

        // Update is called once per frame
        void Update()
        {
            
        }
    }

}