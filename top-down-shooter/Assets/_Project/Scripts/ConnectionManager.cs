using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Cysharp.Threading.Tasks;
using UnityEngine;

using CreateSocketResult = NetworkService.CreateSocketResult;
using FailReason = NetworkService.FailReason;

public class ConnectionManager : Singleton<ConnectionManager>
{
    public static event Action<string> TcpMessageReceived;
    public static event Action<string> UdpMessageReceived;

    public int PlayerID { get; private set; }
    public int PlayerAccountID { get; private set; }

    public bool IsConnectedToGameSever { get; private set; }
    public bool IsConnectedToMasterServer { get; private set; }

    public int LocalUdpPort { get; private set; }

    private const int TimeoutSeconds = 3;

    private Socket _masterTcpSocket;
    private Socket _gameTcpSocket;

    private UdpClient _udpClient;

    private CancellationTokenSource _masterServerCts;
    private CancellationTokenSource _gameServerCts;

    private float _lastGameMessageTime;

    private bool _receivedInitMessage;
    private int? _gameServerPort;

    private void Start()
    {
        TcpMessageReceived += _ => _lastGameMessageTime = Time.time;
        UdpMessageReceived += _ => _lastGameMessageTime = Time.time;

        ConnectToMasterServer();
    }

    private void Update()
    {
        if (!IsConnectedToGameSever || Time.time - _lastGameMessageTime < TimeoutSeconds)
            return;

        Debug.Log("Connection timed out due to inactivity.");
        DisconnectGameServer();

        if(UIManager.Instance != null && UIManager.Instance.InfoUI != null)
            UIManager.Instance.InfoUI.Open("Connection timed out.", "OK", null);
    }

    public async void ConnectToMasterServer()
    {
        await UniTask.WaitUntil(() => SceneLoader.IsUILoaded);

        _masterServerCts?.Cancel();
        _masterServerCts?.Dispose();
        _masterServerCts = new CancellationTokenSource();

        IsConnectedToMasterServer = false;

        DebugConnectionConfig debugConnectionConfig = Resources.Load<DebugConnectionConfig>("DebugConnectionConfig");

        if (debugConnectionConfig == null)
        {
            Debug.LogError("DebugConnectionConfig.asset doesn't exist. You need to create it (Create => Debug Connection Config) and put it in Resources folder");

            return;
        }

        if (!IPAddress.TryParse(debugConnectionConfig.IpAddress, out IPAddress ipAddr))
        {
            Debug.LogError("IpAddress in DebugConnectionConfig.asset is invalid");

            return;
        }

        UIManager.Instance.InfoUI.Open($"Connecting to master server\n{ipAddr}:{debugConnectionConfig.Port}", "", null, false);

        CreateSocketResult result = await NetworkService.CreateTcpSocket(ipAddr, debugConnectionConfig.Port, TimeoutSeconds, _masterServerCts.Token);

        if (!result.Success)
        {
            switch (result.FailReason)
            {
                case FailReason.Unknown:
                    UIManager.Instance.InfoUI.Open($"Failed to connect to server:\n{result.ErrorMessage}", "Try again", ConnectToMasterServer);
                    break;

                case FailReason.Timeout:
                    UIManager.Instance.InfoUI.Open("Connection to master server timed out", "Try again", ConnectToMasterServer);
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }

            _masterServerCts?.Cancel();
            _masterServerCts?.Dispose();
            _masterServerCts = null;

            return;
        }

        Debug.Log("Connected to MasterServer via TCP");
        _masterTcpSocket = result.Socket;

        NetworkService.StartTcpLoop(_masterTcpSocket, OnMasterServerMessageReceived, OnMasterServerDisconnected, _masterServerCts.Token);

        IsConnectedToMasterServer = true;
        UIManager.Instance.InfoUI.Close();
    }

    public async void ConnectToGameServer(string playerName)
    {
        bool success = await ConnectToGameServerInternal(playerName);

        if (!success)
        {
            DisconnectGameServer();
        }
    }

    public async void TcpSendMessageToGameServer(string message)
    {
        await NetworkService.SendMessageAsync(_gameTcpSocket, message);
    }

    public async void TcpSendMessageToMasterServer(string message)
    {
        await NetworkService.SendMessageAsync(_masterTcpSocket, message);
    }

    public async void UdpSendMessageToGameServer(string message)
    {
        await NetworkService.SendMessageAsync(_udpClient, message);
    }

    public void DisconnectGameServer(bool openMainMenu = true, bool showInfo = false)
    {
        IsConnectedToGameSever = false;

        _gameServerPort = null;

        _gameServerCts?.Cancel();
        _gameServerCts?.Dispose();
        _gameServerCts = null;

        _udpClient?.Close();
        NetworkService.CloseSocket(_gameTcpSocket);

        if (openMainMenu && IsConnectedToMasterServer)
        {
            SceneLoader.OpenMainMenuAsync().Forget();
        }

        if (showInfo && IsConnectedToMasterServer)
        {
            if(UIManager.Instance != null && UIManager.Instance.InfoUI != null)
                UIManager.Instance.InfoUI.Open("Disconnected from GAME server.", "OK", null);
        }

        Debug.Log("Disconnected from GameServer");
    }

    private async UniTask<bool> ConnectToGameServerInternal(string playerName)
    {
        IsConnectedToGameSever = false;
        _gameServerCts?.Cancel();
        _gameServerCts?.Dispose();
        _gameServerCts = new CancellationTokenSource();
        _gameServerPort = null;

        UIManager.Instance.InfoUI.Open($"Hello {playerName}!\nWaiting for game server connection info", "Cancel", _gameServerCts.Cancel);

        TcpSendMessageToMasterServer($"0;{(int)GameManager.SelectedGameType};");
        await UniTask.WaitWhile(() => _gameServerPort == null, cancellationToken: _gameServerCts.Token);
        int port = _gameServerPort!.Value;

        Debug.Log($"GameServer port: {port}");

        // ToDo: IpAddress should be also sent by master server
        DebugConnectionConfig debugConnectionConfig = Resources.Load<DebugConnectionConfig>("DebugConnectionConfig");
        IPAddress ipAddr = IPAddress.Parse(debugConnectionConfig.IpAddress);

        UIManager.Instance.InfoUI.Open($"Hello {playerName}!\nConnecting to {debugConnectionConfig.IpAddress}:{port}", "Cancel", _gameServerCts.Cancel);

        CreateSocketResult result = await NetworkService.CreateTcpSocket(ipAddr, port, TimeoutSeconds, _gameServerCts.Token);

        if (!result.Success)
        {
            switch (result.FailReason)
            {
                case FailReason.Unknown:
                    UIManager.Instance.InfoUI.Open("Failed to connect to server. Check your internet connection.", "OK", null);
                    return false;

                case FailReason.Timeout:
                    UIManager.Instance.InfoUI.Open("Connection timed out.", "OK", null);
                    return false;

                case FailReason.Canceled:
                    Debug.LogWarning("User canceled connection");
                    return false;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        _gameTcpSocket = result.Socket;

        Debug.Log("Connected to GameServer via TCP");
        string message = await NetworkService.WaitForMessageAsync(_gameTcpSocket, _gameServerCts.Token);
        string[] messageParts = message.Trim('~').Split(';');

        if (messageParts.Length != 3 || !int.TryParse(messageParts[0], out int messageType) || messageType != (int)GameManager.IncomingMessageType.ServerInfo) {
            Debug.LogError("Incorrect ServerInfo message format: " +  message);
            return false;
        }
        if (!int.TryParse(messageParts[1], out int udpPort)) {
            Debug.LogError("Failed parsing a UDP port: " +  messageParts[0]);
            return false;
        }
        if (!int.TryParse(messageParts[2], out int playerID)) {
            Debug.LogError("Failed parsing a playerID: " + messageParts[1]);
            return false;
        }

        PlayerID = playerID;

        Debug.Log($"Local player ID: {playerID}. GameServer UDP port: {udpPort}");

        NetworkService.CreateUdpClient(ipAddr, udpPort, out _udpClient, out int udpLocalPort);
        LocalUdpPort = udpLocalPort;

        _lastGameMessageTime = Time.time;
        IsConnectedToGameSever = true;

        await SceneLoader.StartGameAsync();

        NetworkService.StartTcpLoop(
            _gameTcpSocket,
            tcpMessage => TcpMessageReceived?.Invoke(tcpMessage),
            () =>
            {
                DisconnectGameServer(showInfo: true);
            },
            _gameServerCts.Token);

        NetworkService.StartUdpLoop(_udpClient, udpMessage => UdpMessageReceived?.Invoke(udpMessage), _gameServerCts.Token);

        UIManager.Instance.InfoUI.Close();

        return true;
    }

    private void OnMasterServerMessageReceived(string message)
    {
        if (!_receivedInitMessage)
        {
            _receivedInitMessage = true;
            Debug.Log($"Received init message: {message}");

            return;
        }

        string[] data = message.Trim().TrimEnd('~').Split(";");
        if (data.Length != 2)
        {
            Debug.Log($"Incorrect GameServerInfo message format. Message: {message}");
            DisconnectGameServer();
            return;
        }

        if (!int.TryParse(data[1], out int port))
        {
            DisconnectGameServer();
            Debug.Log($"Failed to parse port. Message: {message}");
            return;
        }
        Debug.Log($"GameServer port: {port}");
        _gameServerPort = port;
    }

    private void OnMasterServerDisconnected()
    {
        HandleMasterServerDisconnect();
    }

    private void HandleMasterServerDisconnect(bool openMainMenu = true)
    {
        if(IsConnectedToGameSever)
            DisconnectGameServer(false);

        IsConnectedToMasterServer = false;
        _receivedInitMessage = false;

        NetworkService.CloseSocket(_masterTcpSocket);

        _masterServerCts?.Cancel();
        _masterServerCts?.Dispose();
        _masterServerCts = null;

        if (openMainMenu)
        {
            SceneLoader.OpenMainMenuAsync().Forget();
            Debug.LogWarning("Lost connection with master server!");
            UIManager.Instance.InfoUI.Open("Lost connection with master server!", "Reconnect", ConnectToMasterServer);
        }
    }

    private void OnDestroy()
    {
        DisconnectGameServer(false);
        HandleMasterServerDisconnect(false);
    }
}
