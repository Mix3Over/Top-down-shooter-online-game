
using UnityEngine;
using UnityEngine.UI;

public class GameplayUI : UIWindow
{
    [field: SerializeField] public LeaderboardUI LeaderboardUI { get; private set; }
    [field: SerializeField] public PlayerTypeSelectionUI PlayerTypeSelectionUI { get; private set; }
    
    [SerializeField] private Button _disconnectButton;

    protected override void Awake()
    {
        base.Awake();
        _disconnectButton.onClick.AddListener(() =>
        {
            NetworkManager.Instance.Disconnect();
            Close();
        });
    }

    public override void Open()
    {
        base.Open();
        
        LeaderboardUI.Open();
        PlayerTypeSelectionUI.Open();
    }
}
