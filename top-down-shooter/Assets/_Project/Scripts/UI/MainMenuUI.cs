
using UnityEngine;

public class MainMenuUI : UIWindow
{
    [field: SerializeField] public LobbyUI LobbyUI { get; private set; }

    public override void Open()
    {
        base.Open();
        LobbyUI.Open();
    }
}
