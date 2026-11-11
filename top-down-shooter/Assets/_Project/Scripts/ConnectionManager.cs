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
    public event Action<string> TcpMessageReceived;
    public event Action<string> UdpMessageReceived;

    public int PlayerID { get; private set; }
    public bool IsConnected { get; private set; }
    public int UdpLocalPort { get; private set; }

    private const int TimeoutSeconds = 3;

    private Socket _masterTcpSocket;
    private Socket _gameTcpSocket;

    private UdpClient _udpClient;

    private CancellationTokenSource _masterServerDisconnectCts;
    private CancellationTokenSource _gameServerDisconnectCts;

    private float _lastMessageReceivedTime;

    private bool _receivedInitMessage;
    private int? _gameServerPort;

    private void Start()
    {
        TcpMessageReceived += _ => _lastMessageReceivedTime = Time.time;
        UdpMessageReceived += _ => _lastMessageReceivedTime = Time.time;

        ConnectToMasterServer();
    }

    private void Update()
    {
        if (!IsConnected || Time.time - _lastMessageReceivedTime < TimeoutSeconds)
            return;

        Debug.Log("Connection timed out due to inactivity.");
        DisconnectGameServer();
        if(UIManager.Instance != null && UIManager.Instance.InfoUI != null)
            UIManager.Instance.InfoUI.Open("Connection timed out.", "OK", null);
    }

    public async void ConnectToMasterServer()
    {
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

        CancellationTokenSource connectCts = new();
        _masterServerDisconnectCts = new CancellationTokenSource();

        UIManager.Instance.InfoUI.Open($"Connecting to master server\n{ipAddr}:{debugConnectionConfig.Port}", "", null, false);

        CreateSocketResult result = await NetworkService.CreateTcpSocket(ipAddr, debugConnectionConfig.Port, TimeoutSeconds, connectCts.Token);

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

            _masterServerDisconnectCts?.Cancel();
            _masterServerDisconnectCts?.Dispose();
            _masterServerDisconnectCts = null;

            return;
        }

        Debug.Log("Connected to MasterServer via TCP");
        _masterTcpSocket = result.Socket;

        UIManager.Instance.InfoUI.Close();
    }

    public async void ConnectToGameServer(string playerName)
    {
        using CancellationTokenSource connectCts = new();
        _gameServerDisconnectCts = new CancellationTokenSource();

        TcpSendMessageToMasterServer("0;");

        NetworkService.StartTcpLoopAsync(_masterTcpSocket, OnMasterServerMessageReceived, OnMasterServerDisconnected, connectCts.Token);

        await UniTask.WaitWhile(() => _gameServerPort == null, cancellationToken: connectCts.Token);
        int port = _gameServerPort!.Value;

        Debug.Log($"GameServer port: {port}");

        // ToDo: IpAddress should be also sent by master server
        DebugConnectionConfig debugConnectionConfig = Resources.Load<DebugConnectionConfig>("DebugConnectionConfig");
        IPAddress ipAddr = IPAddress.Parse(debugConnectionConfig.IpAddress);

        UIManager.Instance.InfoUI.Open($"Hello {playerName}!\nConnecting to {debugConnectionConfig.IpAddress}:{port}", "Cancel", connectCts.Cancel);

        CreateSocketResult result = await NetworkService.CreateTcpSocket(ipAddr, port, TimeoutSeconds, connectCts.Token);

        if (!result.Success)
        {
            switch (result.FailReason)
            {
                case FailReason.Unknown:
                    DisconnectGameServer();
                    Debug.Log("Failed to connect to server. Check your internet connection.");
                    return;

                case FailReason.Timeout:
                    DisconnectGameServer();
                    UIManager.Instance.InfoUI.Open("Connection timed out.", "OK", null);
                    return;

                case FailReason.Canceled:
                    DisconnectGameServer();
                    connectCts.Dispose();
                    Debug.LogWarning("User canceled connection");
                    return;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        _gameTcpSocket = result.Socket;

        Debug.Log("Connected to GameServer via TCP");
        string message = await NetworkService.WaitForMessageAsync(_gameTcpSocket, connectCts.Token);
        string[] messageParts = message.Trim('~').Split(';');

        if (messageParts.Length != 3 || !int.TryParse(messageParts[0], out int messageType) || messageType != (int)GameManager.MessageType.ServerInfo) {
            Debug.LogError("Incorrect ServerInfo message format: " +  message);
            return;
        }
        if (!int.TryParse(messageParts[1], out int udpPort)) {
            Debug.LogError("Failed parsing a UDP port: " +  messageParts[0]);
            return;
        }
        if (!int.TryParse(messageParts[2], out int playerID)) {
            Debug.LogError("Failed parsing a playerID: " + messageParts[1]);
            return;
        }

        PlayerID = playerID;

        Debug.Log($"Local player ID: {playerID}. GameServer UDP port: {udpPort}");

        NetworkService.CreateUdpClient(ipAddr, udpPort, out _udpClient, out int udpLocalPort);
        UdpLocalPort = udpLocalPort;

        _lastMessageReceivedTime = Time.time;
        IsConnected = true;

        SceneLoader.StartGame();

        NetworkService.StartTcpLoopAsync(
            _gameTcpSocket,
            tcpMessage => TcpMessageReceived?.Invoke(tcpMessage),
            () =>
            {
                DisconnectGameServer();
                if(UIManager.Instance != null && UIManager.Instance.InfoUI != null)
                    UIManager.Instance.InfoUI.Open("Disconnected from server.", "OK", null);
            },
            _gameServerDisconnectCts.Token);
        NetworkService.StartUdpLoopAsync(_udpClient, udpMessage => UdpMessageReceived?.Invoke(udpMessage), _gameServerDisconnectCts.Token);

        UIManager.Instance.InfoUI.Close();
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

    public void DisconnectGameServer(bool openUI = true)
    {
        _gameServerDisconnectCts?.Cancel();
        _gameServerDisconnectCts?.Dispose();
        _gameServerDisconnectCts = null;
        IsConnected = false;
        _udpClient?.Close();

        NetworkService.CloseSocket(_gameTcpSocket);

        if(openUI)
            SceneLoader.OpenMainMenu();

        Debug.Log("Disconnected from GameServer");
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
        Debug.LogWarning("TODO: Handle disconnection from GameServer");
    }

    private void OnDestroy()
    {
        DisconnectGameServer(false);
        NetworkService.CloseSocket(_masterTcpSocket);

        _masterServerDisconnectCts?.Cancel();
        _masterServerDisconnectCts?.Dispose();
        _masterServerDisconnectCts = null;
    }
}
