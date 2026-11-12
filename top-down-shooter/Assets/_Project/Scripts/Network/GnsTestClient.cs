using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using Valve.Sockets;

// Test client for GameNetworkingSockets.
// Connects to the server on Start and raises events for status changes and received messages.
// GnsTestUI (on the same GameObject) shows them on screen.
//
// The native library is shared by every GnsTestClient in the scene:
// it is initialized by the first one and shut down only when the last one is destroyed.
// Shutting it down while another client still uses it crashes Unity.
public class GnsTestClient : MonoBehaviour
{
    [SerializeField] private string serverIp = "127.0.0.1";
    [SerializeField] private ushort serverPort = 7777; // 7777 = gnsEcho (default), 27020 = Valve's example_chat server

    // Both events are raised on Unity's main thread (from Update), so UI code can use them directly.
    public event Action<string> StatusChanged;
    public event Action<string> MessageReceived;

    public bool IsConnected { get; private set; }
    public string Status { get; private set; } = "Not started";

    // ---- shared native library state (one for all clients)
    private static int libraryUsers;
    private static NetworkingUtils utils;
    private static StatusCallback statusCallback; // static field: the GC must never collect it while native code can call it
    private static readonly Dictionary<uint, GnsTestClient> clientsByConnection = new Dictionary<uint, GnsTestClient>();

    // ---- this client's state
    private NetworkingSockets client;
    private uint connection;
    private bool usesLibrary;
    private readonly NetworkingMessage[] messages = new NetworkingMessage[20];

    // Resets static state when "Enter Play Mode without domain reload" is used.
    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.SubsystemRegistration)]
    private static void ResetStatics()
    {
        libraryUsers = 0;
        utils = null;
        statusCallback = null;
        clientsByConnection.Clear();
    }

    private void Start()
    {
        if (!AcquireLibrary())
            return;

        client = new NetworkingSockets();

        string ip = serverIp.Trim(); // a pasted IP often ends with a space
        if (!System.Net.IPAddress.TryParse(ip, out _))
        {
            SetStatus("Invalid Server Ip: '" + serverIp + "'");
            return;
        }

        Address address = new Address();
        address.SetAddress(ip, serverPort);
        connection = client.Connect(ref address);
        clientsByConnection[connection] = this;
        SetStatus("Connecting to " + ip + ":" + serverPort + "...");
    }

    private bool AcquireLibrary()
    {
        if (libraryUsers == 0)
        {
            var error = new StringBuilder(Library.maxErrorMessageLength);
            if (!Library.Initialize(error))
            {
                SetStatus("Initialize failed: " + error);
                return false;
            }

            utils = new NetworkingUtils();
            statusCallback = OnAnyConnectionStatusChanged;
            utils.SetStatusCallback(statusCallback);
        }

        libraryUsers++;
        usesLibrary = true;
        return true;
    }

    private static void ReleaseLibrary()
    {
        libraryUsers--;
        if (libraryUsers > 0)
            return;

        // Last client is gone: now it's safe to shut the native library down.
        utils?.Dispose();
        utils = null;
        Library.Deinitialize();
        statusCallback = null;
        clientsByConnection.Clear();
        libraryUsers = 0;
    }

    // The status callback is global in GNS, so route each event to the client that owns the connection.
    private static void OnAnyConnectionStatusChanged(ref StatusInfo info)
    {
        if (clientsByConnection.TryGetValue(info.connection, out GnsTestClient owner) && owner != null)
            owner.HandleStatus(ref info);
    }

    // Called from RunCallbacks(), so it runs on Unity's main thread.
    private void HandleStatus(ref StatusInfo info)
    {
        switch (info.connectionInfo.state)
        {
            case ConnectionState.Connected:
                IsConnected = true;
                SetStatus("Connected");
                break;

            case ConnectionState.ClosedByPeer:
            case ConnectionState.ProblemDetectedLocally:
                IsConnected = false;
                SetStatus("Disconnected: " + info.connectionInfo.endDebug);
                CloseOwnConnection();
                break;
        }
    }

    private void Update()
    {
        if (client == null)
            return;

        client.RunCallbacks();

        if (connection == 0)
            return;

        int count = client.ReceiveMessagesOnConnection(connection, messages, messages.Length);
        for (int i = 0; i < count; i++)
        {
            byte[] buffer = new byte[messages[i].length];
            messages[i].CopyTo(buffer);
            messages[i].Destroy(); // always free the message

            string text = Encoding.UTF8.GetString(buffer);
            Debug.Log("[GNS] Received: " + text);
            MessageReceived?.Invoke(text);
        }
    }

    // Sends a reliable text message. Returns false if we are not connected.
    public bool Send(string text)
    {
        if (client == null || !IsConnected || connection == 0)
            return false;

        client.SendMessageToConnection(connection, Encoding.UTF8.GetBytes(text), SendFlags.Reliable);
        Debug.Log("[GNS] Sent: " + text);
        return true;
    }

    private void SetStatus(string status)
    {
        Status = status;
        Debug.Log("[GNS] " + status);
        StatusChanged?.Invoke(status);
    }

    private void CloseOwnConnection()
    {
        if (client == null || connection == 0)
            return;

        clientsByConnection.Remove(connection);
        client.CloseConnection(connection);
        connection = 0;
        IsConnected = false;
    }

    // Runs when Play mode stops (and when the object is destroyed).
    // Order matters: close our connection first, then release the shared library.
    private void OnDestroy()
    {
        CloseOwnConnection();
        client = null;

        if (usesLibrary)
        {
            usesLibrary = false;
            ReleaseLibrary();
        }
    }
}
