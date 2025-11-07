/*
 * Copyright (c) Katsuya Owari
 */

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace QS
{
    public class Character : MonoBehaviour
    {
        private bool isJumping = false;
        private bool isFalling = false;
        private float reloadTime = 0;
        private float jumpY = 0;
        private float hoverTime = 0;
        private Vector3 jumpVector = new Vector3(0, 0, 0);
        private Vector3 JumpHeight = new Vector3(0, 0, 0);
        private Vector3 position = new Vector3(0, 0, 0);
        private CharacterController characterController;

        public bool IsJumping
        {
            get { return isJumping; }
        }

        public bool IsFalling
        {
            get { return isFalling; }
        }

        void Start()
        {
            characterController = GetComponent<CharacterController>();
            position = transform.position;
        }

        public void Move(Vector3 move)
        {
            characterController.Move(move);
        }

        public void LookAt(Quaternion quaternion)
        {
            transform.rotation = quaternion;
        }

        public void Action()
        {
            if (reloadTime > 0)
            {
                return;
            }
            string prefabName = "ball";
            GameObject ballPrefab = Resources.Load<GameObject>($"Prefabs/{prefabName}");
            if (ballPrefab != null)
            {
                Vector3 position = transform.position + transform.forward * 2;
                GameObject ball = Instantiate(ballPrefab, position, transform.rotation);
                ball.GetComponent<Rigidbody>().AddForce(transform.forward * 1000);
            }
            else
            {
                Debug.LogError($"Prefab '{prefabName}' not found in Resources/Prefabs directory.");
            }
            reloadTime = 0.2f;
        }

        public void Jump(float jumpPower = 10.0f, float minJumpPower = 1.5f, float maxJumpPower = 5.0f)
        {
            if(isFalling){
                return;
            }
            if(isJumping){
                if(JumpHeight.y < maxJumpPower){
                    JumpHeight.y += jumpPower * Time.deltaTime;
                }
                return;
            }
            jumpVector.y = jumpPower;
            JumpHeight.y = this.transform.position.y + minJumpPower;
            isJumping = true;
            hoverTime = 0;
        }

        void LateUpdate()
        {
            if (reloadTime > 0)
            {
                reloadTime -= Time.deltaTime;
                if(reloadTime < 0){
                    reloadTime = 0;
                }
            }
            if (isFalling)
            {
                characterController.Move((jumpVector * -1) * Time.deltaTime);
                if(this.transform.position.y < 0 || jumpY == this.transform.position.y){
                    isFalling = false;
                    isJumping = false;
                    JumpHeight.y = 0;
                    hoverTime = 0;
                }
                jumpY = this.transform.position.y;
            }
            else if (isJumping)
            {
                if(this.transform.position.y > JumpHeight.y){
                    hoverTime += Time.deltaTime;
                    if(hoverTime > 0.2f){
                        isFalling = true;
                    }
                }else{
                    characterController.Move(jumpVector * Time.deltaTime);
                }
            }
        }
    }
}