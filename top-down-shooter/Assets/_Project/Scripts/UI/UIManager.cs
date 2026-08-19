
using UnityEngine;

public class UIManager : Singleton<UIManager>
{
    [field: SerializeField] public MainMenuUI MainMenuUI { get; private set; }
    [field: SerializeField] public GameplayUI GameplayUI { get; private set; }
    
    public LobbyUI LobbyUI { get; private set; }
    public PlayerTypeSelectionUI PlayerTypeSelectionUI { get; private set; }
    public InfoUI InfoUI { get; private set; }
    public LeaderboardUI LeaderboardUI { get; private set; }

    protected override void Awake()
    {
        base.Awake();
        LobbyUI = GetComponentInChildren<LobbyUI>(true);
        PlayerTypeSelectionUI = GetComponentInChildren<PlayerTypeSelectionUI>(true);
        InfoUI = GetComponentInChildren<InfoUI>(true);
        LeaderboardUI = GetComponentInChildren<LeaderboardUI>(true);
    }
}

