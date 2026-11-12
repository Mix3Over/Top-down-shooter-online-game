using TMPro;
using UnityEngine;

public class MainMenuUI : UIWindow
{
    [field: SerializeField] public LobbyUI LobbyUI { get; private set; }

    [SerializeField] private TMP_Text _activePlayersCountText;

    private void OnEnable()
    {
        MasterServerManager.PlayerCountChanged += UpdatePlayersCount;
    }

    private void OnDisable()
    {
        MasterServerManager.PlayerCountChanged -= UpdatePlayersCount;
    }

    public override void Open()
    {
        base.Open();
        LobbyUI.Open();
        UpdatePlayersCount();
    }

    private void UpdatePlayersCount()
    {
        _activePlayersCountText.text = $"Players online: {MasterServerManager.Instance.PlayersCount}";
    }
}
