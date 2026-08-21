using UnityEngine;

public class Bullet : MonoBehaviour
{
    private float _speed = 1;

    public void Init(Vector3 position, float movementDirection, float speed)
    {
        transform.position = position;
        transform.eulerAngles = new Vector3(0, 0, movementDirection);
        _speed = speed;
    }

    private void Update()
    {
        transform.position += transform.right * (Time.deltaTime * _speed);
    }
}
