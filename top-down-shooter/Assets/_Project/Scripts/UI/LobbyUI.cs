using System.Net;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class LobbyUI : UIWindow
{
    [SerializeField] private TMP_InputField _nameInputField;
    [SerializeField] private TMP_InputField _addressInputField;
    [SerializeField] private TMP_InputField _portInputField;

    [SerializeField] private Button _joinButton;

    public string PlayerName => _nameInputField.text;

    protected override void Awake()
    {
        base.Awake();
        _joinButton.onClick.AddListener(OnJoinButton);
        _nameInputField.onValueChanged.AddListener(OnPlayerNameValueChanged);
    }

    private void OnJoinButton()
    {
        if(!int.TryParse(_portInputField.text, out int port)) {
            UIManager.Instance.InfoUI.Open("Incorrect port!", "Ok", null);
            return;
        }
        if(_nameInputField.text.Length == 0) {
            UIManager.Instance.InfoUI.Open("Enter your name!", "Ok", null);
            return;
        }

        if (!IPAddress.TryParse(_addressInputField.text, out IPAddress ipAddr)) {
            UIManager.Instance.InfoUI.Open("Incorrect IP address!", "Ok", null);
            return;
        }
        _nameInputField.text = _nameInputField.text.Replace('~', '.').Replace(':', '.').Replace(';', '.').Replace('!', '.');

        NetworkManager.Instance.ConnectToServer(ipAddr, port, _nameInputField.text);
        Close();
    }

    private void OnPlayerNameValueChanged(string value)
    {
        _nameInputField.text = value.Replace('~', '.').Replace(':', '.').Replace(';', '.').Replace('!', '.');
    }
}
