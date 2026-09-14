using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class PlayerTypeSelectionUI : UIWindow
{
    [SerializeField] private List<Button> _buttons;

    public int SelectedPlayerType { get; private set; } = -1;

    protected override void Awake()
    {
        base.Awake();
        for (int i = 0; i < _buttons.Count; i++)
        {
            int index = i;
            _buttons[i].onClick.AddListener(() => OnPlayerTypeSelected(index));
        }
    }

    private void OnPlayerTypeSelected(int playerType)
    {
        Debug.Log("Button for player type " + playerType + " pressed");
        SelectedPlayerType = playerType;
        GameManager.Instance.SpawnLocalPlayer(playerType);
        Close();
    }
}
