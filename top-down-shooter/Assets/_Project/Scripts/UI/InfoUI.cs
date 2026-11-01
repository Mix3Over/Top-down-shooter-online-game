using System;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class InfoUI : UIWindow
{
    [SerializeField] private TMP_Text _text;
    [SerializeField] private Button _confirmButton;

    private Action _buttonPressed;

    override protected void Awake()
    {
        base.Awake();
        _confirmButton.onClick.AddListener(OnConfirmButtonClicked);
    }

    public void Open(string text, string buttonText, Action buttonPressed, bool showCloseButton = true)
    {
        _text.text = text;
        _confirmButton.GetComponentInChildren<TMP_Text>().text = buttonText;
        _confirmButton.gameObject.SetActive(showCloseButton);
        _buttonPressed = buttonPressed;
        Open();
    }

    private void OnConfirmButtonClicked()
    {
        Close();
        _buttonPressed?.Invoke();
    }

    public override void Close()
    {
        _text.text = "";
        _confirmButton.GetComponentInChildren<TMP_Text>().text = "";
        base.Close();
    }
}
