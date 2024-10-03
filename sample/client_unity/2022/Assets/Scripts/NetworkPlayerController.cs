using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace QS
{
    public class NetworkPlayerController : MonoBehaviour
    {
        private bool isSync = false;
        private Vector3 moveToPos = Vector3.zero;
        private Quaternion moveToRot = Quaternion.identity;
        private Vector3 moveToScale = Vector3.zero;
        public float moveSpeed = 0.2f; // 移動速度

        // Start is called before the first frame update
        void Start()
        {
            
        }

        // Update is called once per frame
        void Update()
        {
            if(isSync)
            {
                // 位置を補完
                //this.transform.position = Vector3.Lerp(this.transform.position, this.moveToPos, Time.deltaTime * moveSpeed);

                // 減衰運動
                Vector3 velocity = Vector3.zero;
                float smoothTime = 0.1f;
                this.transform.position = Vector3.SmoothDamp(this.transform.position, this.moveToPos, ref velocity, smoothTime);

                // 回転を徐々に補完
                //this.transform.rotation = Quaternion.Lerp(this.transform.rotation, this.moveToRot, Time.deltaTime * moveSpeed);
                this.transform.rotation = this.moveToRot;

                // スケールを徐々に補完
                this.transform.localScale = Vector3.Lerp(this.transform.localScale, this.moveToScale, Time.deltaTime * moveSpeed);
            }
        }

        public void SetMoveToTransform(Vector3 position, Quaternion rotation, Vector3 scale)
        {
            this.moveToPos = position;
            this.moveToRot = rotation;
            this.moveToScale = scale;
            isSync = true;
        }
    }
}

