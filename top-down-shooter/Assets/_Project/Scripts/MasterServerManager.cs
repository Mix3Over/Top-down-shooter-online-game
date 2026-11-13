using System;
using System.Collections.Generic;
using UnityEngine;

public class MasterServerManager : Singleton<MasterServerManager>
{
    private enum IncomingMessageType
    {
        Heartbeat,
        PlayerInfo,
        PlayerCount,
        GameServerInfo
    }

    public static event Action PlayerCountChanged;

    public int? PlayerAccountId { get; private set; }
    public int? GameServerPort { get; private set; }

    public int PlayersCount { get; private set; }

    private Dictionary<IncomingMessageType, Action<string>> _incomingMessagesHandlers;

    protected override void Awake()
    {
        base.Awake();

        _incomingMessagesHandlers = new Dictionary<IncomingMessageType, Action<string>>()
        {
            { IncomingMessageType.PlayerInfo, HandlePlayerInfoMessage },
            { IncomingMessageType.Heartbeat, null },
            { IncomingMessageType.PlayerCount, HandlePlayerCountMessage },
            { IncomingMessageType.GameServerInfo, HandleGameServerInfoMessage },
        };
    }

    private void OnEnable()
    {
        ConnectionManager.GameDisconnected += OnGameDisconnected;
        ConnectionManager.MasterDisconnected += OnMasterDisconnected;
        ConnectionManager.MasterMessageReceived += OnMessageReceived;
    }

    private void OnDisable()
    {
        ConnectionManager.GameDisconnected -= OnGameDisconnected;
        ConnectionManager.MasterDisconnected -= OnMasterDisconnected;
        ConnectionManager.MasterMessageReceived -= OnMessageReceived;
    }

    private void OnGameDisconnected()
    {
        GameServerPort = null;
    }

    private void OnMasterDisconnected()
    {
        GameServerPort = null;
        PlayerAccountId = null;
    }

    private void OnMessageReceived(string message)
    {
        string[] parts = message.Split(";");

        if (parts.Length < 1) {
            Debug.LogWarning("Incorrect message format: " + message);
            return;
        }

        string trimmedMessage = message[1..].Trim(';');
        int.TryParse(parts[0], out int messageType);

        if (_incomingMessagesHandlers.ContainsKey((IncomingMessageType)messageType))
        {
            _incomingMessagesHandlers[(IncomingMessageType)messageType]?.Invoke(trimmedMessage);
        }
        else
        {
            Debug.LogWarning("Received unhandled message type:  " + message);
        }
    }

    private void HandlePlayerInfoMessage(string message)
    {
        Debug.Log($"Received init message: {message}");

        if (!int.TryParse(message, out int playerId))
        {
            Debug.LogError("Failed to parse PlayerAccountId: " + message);
            return;
        }

        if (PlayerAccountId != null)
        {
            Debug.LogWarning("PlayerAccountId is already set to: " + PlayerAccountId);
        }

        PlayerAccountId = playerId;
    }

    private void HandlePlayerCountMessage(string message)
    {
        Debug.Log($"Received init message: {message}");

        if (!int.TryParse(message, out int playersCount))
        {
            Debug.LogError("Failed to parse PlayerAccountId: " + message);
            return;
        }

        PlayersCount = playersCount;
        PlayerCountChanged?.Invoke();
    }

    private void HandleGameServerInfoMessage(string message)
    {
        if (!int.TryParse(message, out int port))
        {
            Debug.Log($"Incorrect GameServerInfo message format. Message: {message}");
            ConnectionManager.Instance.DisconnectGameServer();
            return;
        }

        Debug.Log($"GameServer port: {port}");
        GameServerPort = port;
    }
}
