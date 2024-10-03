using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace QS
{
    [System.Serializable]
    public class PlayerCharacterData
    {
        public string playerId;
        public Vector3 position;
        public Quaternion rotation;
        public Vector3 scale;

        public PlayerCharacterData(string playerId, Transform transform)
        {
            this.playerId = playerId;
            position = transform.position;
            rotation = transform.rotation;
            scale = transform.localScale;
        }

        public void SetTransform(Transform transform)
        {
            transform.position = position;
            transform.rotation = rotation;
            transform.localScale = scale;
        }
    }
}