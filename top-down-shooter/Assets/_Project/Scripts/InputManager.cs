using System;
using UnityEngine;
using UnityEngine.InputSystem;

public class InputManager : Singleton<InputManager>
{
    [SerializeField] private InputActionAsset _inputActionAsset;

    public static event Action<Vector2> MousePositionChanged;
    public static event Action<Vector2> PlayerMoved;
    public static event Action ShootingPerformed;

    private InputActionMap _gameplayActionMap;
    private InputAction _lookAction;
    private InputAction _moveAction;
    private InputAction _shootAction;

    protected override void Awake()
    {
        base.Awake();

        _gameplayActionMap = _inputActionAsset.FindActionMap("Gameplay", true);
        _lookAction = InputSystem.actions.FindAction("Look");
        _moveAction = InputSystem.actions.FindAction("Move");
        _shootAction = InputSystem.actions.FindAction("Attack");

        _lookAction.performed += _ => MousePositionChanged?.Invoke(_lookAction.ReadValue<Vector2>());
        
    }

    private void Update()
    {
        Vector2 move = _moveAction.ReadValue<Vector2>();

        if (move != Vector2.zero)
            PlayerMoved?.Invoke(_moveAction.ReadValue<Vector2>());

        if (_shootAction.IsPressed())
        ShootingPerformed?.Invoke();    
    }

    private void OnEnable()
    {
        _gameplayActionMap.Enable();
    }

    private void OnDisable()
    {
        _gameplayActionMap.Disable();
    }
}
