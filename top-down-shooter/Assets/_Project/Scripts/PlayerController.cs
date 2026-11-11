using UnityEngine;

[RequireComponent (typeof(Player))]
public class PlayerController : MonoBehaviour
{
    private Player _player;
    private bool _isShooting;
    private float _lastShootTime;

    private void Awake()
    {
        _player = GetComponent<Player>();
    }

    private void OnEnable()
    {
        InputManager.PlayerMoved += Move;
        InputManager.ShootingPerformed += Shoot;
        InputManager.MousePositionChanged += UpdateLookDirection;
    }

    private void OnDisable()
    {
        InputManager.PlayerMoved -= Move;
        InputManager.ShootingPerformed -= Shoot;
        InputManager.MousePositionChanged -= UpdateLookDirection;
    }

    private void Update()
    {
        SendTransformData();
    }

    private void Move(Vector2 move)
    {
        Vector3 movementVector3 = move;

        if (movementVector3.magnitude > 1)
            movementVector3.Normalize();

        Vector3 position = transform.position + movementVector3 * (_player.PlayerTypeData.MovementSpeed * Time.deltaTime);
        if(position.x > 30f)
            position.x = 30f;
        if(position.x < -30f)
            position.x = -30f;
        if(position.y > 20f)
            position.y = 20f;
        if(position.y < -20f)
            position.y = -20f;

        transform.position = position;
    }

    private void UpdateLookDirection(Vector2 mousePosition)
    {
        Vector3 mouseWorldPosition = Camera.main.ScreenToWorldPoint(mousePosition);

        Vector2 direction = mouseWorldPosition - transform.position;
        _player.Visual.transform.rotation = Quaternion.FromToRotation(Vector3.right, direction);
    }

    private void Shoot()
    {
        if (!ConnectionManager.Instance.IsConnected)
            return;

        if (Time.time - _lastShootTime < _player.PlayerTypeData.ShootingCooldown)
            return;

        _lastShootTime = Time.time;
        Vector2Int position = transform.position.ToServer();
        ConnectionManager.Instance.TcpSendMessageToGameServer(
            $"{(int)GameManager.OutgoingMessageType.Shoot};{position.x};{position.y};{Mathf.RoundToInt(_player.Visual.transform.eulerAngles.z)};");
    }

    private void SendTransformData()
    {
        if (!ConnectionManager.Instance.IsConnected)
            return;

        Vector2Int position = transform.position.ToServer();
        ConnectionManager.Instance.UdpSendMessageToGameServer(
            $"{(int)GameManager.OutgoingMessageType.UpdUpdate};{ConnectionManager.Instance.PlayerID};{position.x};{position.y};{Mathf.RoundToInt(_player.Visual.transform.eulerAngles.z)};");
    }
}
