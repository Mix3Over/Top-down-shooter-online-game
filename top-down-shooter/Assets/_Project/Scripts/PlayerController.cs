using UnityEngine;

[RequireComponent(typeof(Player))]
public class PlayerController : MonoBehaviour
{
    private Player _player;
    private bool _isShooting;
    private Vector2 _moveInput;
    private Vector2 _lastMousePosition;
    private float _sendTimer;
    private const float SendInterval = 1f / 32f;

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
        _sendTimer += Time.deltaTime;
        if (_sendTimer >= SendInterval)
        {
            _sendTimer = 0f;
            SendInputData();
            _moveInput = Vector2.zero;
            _isShooting = false;
        }
    }

    private void Move(Vector2 move)
    {
        _moveInput = move;
    }

    private void UpdateLookDirection(Vector2 mousePosition)
    {
        _lastMousePosition = mousePosition;
    }

    private void Shoot()
    {
        _isShooting = true;
    }

    private void SendInputData()
    {
        if (!ConnectionManager.Instance.IsConnectedToGameSever)
            return;

        Vector2 normalizedMove = _moveInput.magnitude > 1f ? _moveInput.normalized : _moveInput;
        int moveX = Mathf.RoundToInt(normalizedMove.x * 100f);
        int moveY = Mathf.RoundToInt(normalizedMove.y * 100f);
        int rotation = CalculateTargetRotation();
        int shootFlag = _isShooting ? 1 : 0;

        ConnectionManager.Instance.SendMessageToGameServer(
             $"{(int)GameManager.OutgoingMessageType.UpdUpdate};" +
             $"{ConnectionManager.Instance.PlayerID};" +
             $"{moveX};" +
             $"{moveY};" +
             $"{rotation};" +
             $"{shootFlag};",
             reliable: false
             );
    }

    private int CalculateTargetRotation()
    {
        if (Camera.main == null)
            return Mathf.RoundToInt(_player.Visual.transform.eulerAngles.z);

        Vector3 mouseWorldPosition = Camera.main.ScreenToWorldPoint(_lastMousePosition);
        Vector2 direction = mouseWorldPosition - transform.position;
        Quaternion targetRot = Quaternion.FromToRotation(Vector3.right, direction);
        return Mathf.RoundToInt(targetRot.eulerAngles.z);
    }
}
