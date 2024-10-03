using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CubeObject : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {
        // 時計回りに回転
        float rotationSpeed = 180f;
        transform.Rotate(0, rotationSpeed * Time.deltaTime, 0);
    }
}
