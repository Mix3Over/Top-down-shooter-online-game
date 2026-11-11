using NUnit.Framework;
using System.Net;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;

public class LobbyUI : UIWindow
{
    [SerializeField] private TMP_InputField _nameInputField;
    [SerializeField] private TMP_InputField _addressInputField;
    [SerializeField] private TMP_InputField _portInputField;

    [SerializeField] private Button _joinButton;

    [SerializeField] private List<Button> _gameModeButtons;

    public string PlayerName => _nameInputField.text;

    protected override void Awake()
    {
        base.Awake();
        _joinButton.onClick.AddListener(OnJoinButton);
        _nameInputField.onValueChanged.AddListener(OnPlayerNameValueChanged);
    
        for (int i = 0; i < _gameModeButtons.Count; i++)
        {
            int index = i;
            _gameModeButtons[i].onClick.AddListener(() => OnGameModeSelected(index));
            _gameModeButtons[i].GetComponentInChildren<TMP_Text>().text = ((GameType)index).ToString().Replace('_', ' ');
        }

        ShowJoinStep();
    }

    private void OnJoinButton()
    {
        /*
        if(!int.TryParse(_portInputField.text, out int port)) {
            UIManager.Instance.InfoUI.Open("Incorrect port!", "Ok", null);
            return;
        }
        */
        if(_nameInputField.text.Length == 0) {
            UIManager.Instance.InfoUI.Open("Enter your name!", "Ok", null);
            return;
        }

        /*
        if (!IPAddress.TryParse(_addressInputField.text, out IPAddress ipAddr)) {
            UIManager.Instance.InfoUI.Open("Incorrect IP address!", "Ok", null);
            return;
        }
        */
        _nameInputField.text = _nameInputField.text.Replace('~', '.').Replace(':', '.').Replace(';', '.').Replace('!', '.');

        _nameInputField.gameObject.SetActive(false);
        _joinButton.gameObject.SetActive(false);

        foreach (Button btn in _gameModeButtons)
        {
            if (btn != null)
                btn.gameObject.SetActive(true);
        }
        //ConnectionManager.Instance.ConnectToGameServer(_nameInputField.text);
        //Close();
    }

    private void OnGameModeSelected(int gameTypeIndex)
    {
        GameManager.SelectedGameType = (GameType)gameTypeIndex;
        Debug.Log("Selected game mode: " + GameManager.SelectedGameType);

        ConnectionManager.Instance.ConnectToGameServer(_nameInputField.text);
        Close();
    }
    public override void Open()
    {
        base.Open();
        ShowJoinStep();
    }

    private void ShowJoinStep()
    {
        _nameInputField.gameObject.SetActive(true);
        _joinButton.gameObject.SetActive(true);

        foreach (Button btn in _gameModeButtons)
        {
            if (btn != null)
                btn.gameObject.SetActive(false);
        }
    }
    private void OnPlayerNameValueChanged(string value)
    {
        _nameInputField.text = value.Replace('~', '.').Replace(':', '.').Replace(';', '.').Replace('!', '.');
    }

    public void SelectGameModeAndContinue(int gameTypeIndex)
    {
        GameManager.Instance.SetGameType(gameTypeIndex);
        Debug.Log("Selected game mode:" + GameManager.SelectedGameType);

        OnJoinButton();
    }
}
