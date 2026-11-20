using System.Numerics;
using System.Runtime.CompilerServices;
using System.Threading.Tasks.Dataflow;
using UnityEngine;

[RequireComponent(typeof(Player))]
public class PlayerController : MonoBehaviour
{
    private Player _player;
    private bool _isShooting;
    private Vector2 _moveInput;
    private Vector2 _lastMousePosition;

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
        SendInputData();

        _moveInput = Vector2.zero;
        _isShooting = false;
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
        int moveX = MathF.RoundToInt(normalizedMove.X * 100f);
        int moveY = MathF.RoundToInt(normalizedMove.Y * 100f);
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
            return MathF.RoundToInt(_player.Visual.transform.eulerAngles.z);

        Vector3 mouseWorldPosition = Camera.main.ScreenToWorldPoint(_lastMousePosition);
        Vector2 direction = mouseWorldPosition - transform.position;
        Quaternion targetRot = Quaternion.FromToRotation(Vector3.right, direction);
        return MathF.RoundToInt(targetRot.eulerAngles.z);
    }
}
