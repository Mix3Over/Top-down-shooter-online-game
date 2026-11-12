using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem.UI;
using UnityEngine.UI;

// Simple test UI for GnsTestClient: a text field, a Send button, and the server's reply.
// Put it on the same GameObject as GnsTestClient. The UI is built from code on Start,
// so you don't need to set anything up in the scene.
[RequireComponent(typeof(GnsTestClient))]
public class GnsTestUI : MonoBehaviour
{
    private GnsTestClient client;

    private TMP_InputField messageInput;
    private Button sendButton;
    private TMP_Text statusText;
    private TMP_Text replyText;

    private void Awake()
    {
        client = GetComponent<GnsTestClient>();
        BuildUI();
    }

    private void OnEnable()
    {
        client.StatusChanged += OnStatusChanged;
        client.MessageReceived += OnMessageReceived;
    }

    private void OnDisable()
    {
        client.StatusChanged -= OnStatusChanged;
        client.MessageReceived -= OnMessageReceived;
    }

    private void Start()
    {
        OnStatusChanged(client.Status);
        sendButton.onClick.AddListener(SendMessageFromInput);
        messageInput.onSubmit.AddListener(_ => SendMessageFromInput()); // Enter also sends
    }

    private void Update()
    {
        sendButton.interactable = client.IsConnected;
    }

    private void SendMessageFromInput()
    {
        string text = messageInput.text;
        if (string.IsNullOrWhiteSpace(text))
            return;

        if (client.Send(text))
        {
            replyText.text = "Sent: " + text + "\nWaiting for reply...";
            messageInput.text = string.Empty;
        }
        else
        {
            replyText.text = "Not connected - can't send.";
        }

        messageInput.ActivateInputField(); // keep typing without clicking the field again
    }

    private void OnStatusChanged(string status)
    {
        if (statusText != null)
            statusText.text = "Status: " + status;
    }

    private void OnMessageReceived(string message)
    {
        replyText.text = "Reply from server:\n" + message;
    }

    // ---------------------------------------------------------------- UI building

    private void BuildUI()
    {
        EnsureEventSystem();

        var canvasGO = new GameObject("GnsTestCanvas", typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
        var canvas = canvasGO.GetComponent<Canvas>();
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        var scaler = canvasGO.GetComponent<CanvasScaler>();
        scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
        scaler.referenceResolution = new Vector2(1920, 1080);

        var resources = new TMP_DefaultControls.Resources();

        statusText = CreateText(canvasGO.transform, resources, "Status", new Vector2(0, -60), new Vector2(900, 50), 28);
        statusText.text = "Status: -";

        GameObject inputGO = TMP_DefaultControls.CreateInputField(resources);
        inputGO.name = "MessageInput";
        inputGO.transform.SetParent(canvasGO.transform, false);
        Place(inputGO.GetComponent<RectTransform>(), new Vector2(-90, -140), new Vector2(700, 60));
        messageInput = inputGO.GetComponent<TMP_InputField>();
        messageInput.pointSize = 28;
        ((TMP_Text)messageInput.placeholder).text = "Type a message...";

        GameObject buttonGO = TMP_DefaultControls.CreateButton(resources);
        buttonGO.name = "SendButton";
        buttonGO.transform.SetParent(canvasGO.transform, false);
        Place(buttonGO.GetComponent<RectTransform>(), new Vector2(370, -140), new Vector2(180, 60));
        sendButton = buttonGO.GetComponent<Button>();
        var buttonLabel = buttonGO.GetComponentInChildren<TMP_Text>();
        buttonLabel.text = "Send";
        buttonLabel.fontSize = 28;

        replyText = CreateText(canvasGO.transform, resources, "Reply", new Vector2(0, -330), new Vector2(900, 280), 32);
        replyText.text = "Type a message and press Send.";
    }

    private static TMP_Text CreateText(Transform parent, TMP_DefaultControls.Resources resources, string name,
                                       Vector2 position, Vector2 size, float fontSize)
    {
        GameObject go = TMP_DefaultControls.CreateText(resources);
        go.name = name;
        go.transform.SetParent(parent, false);
        Place(go.GetComponent<RectTransform>(), position, size);
        var text = go.GetComponent<TMP_Text>();
        text.fontSize = fontSize;
        text.color = Color.white;
        text.alignment = TextAlignmentOptions.Top;
        return text;
    }

    // Anchors the element to the top-center of the screen.
    private static void Place(RectTransform rect, Vector2 position, Vector2 size)
    {
        rect.anchorMin = rect.anchorMax = new Vector2(0.5f, 1f);
        rect.pivot = new Vector2(0.5f, 0.5f);
        rect.anchoredPosition = position;
        rect.sizeDelta = size;
    }

    // UI clicks need an EventSystem. This project uses the new Input System,
    // so it needs InputSystemUIInputModule instead of the old StandaloneInputModule.
    private static void EnsureEventSystem()
    {
        if (FindAnyObjectByType<EventSystem>() != null)
            return;

        var eventSystemGO = new GameObject("EventSystem", typeof(EventSystem), typeof(InputSystemUIInputModule));
        eventSystemGO.GetComponent<InputSystemUIInputModule>().AssignDefaultActions(); // mouse click, Enter, etc.
    }
}
