using System.Text;
using UnityEngine;
using Valve.Sockets;

// Test client for GameNetworkingSockets.
// Put it on an empty GameObject in a test scene and press Play.
// It connects to the server, sends "hello" once connected and logs everything it receives.
public class GnsTestClient : MonoBehaviour
{
    [SerializeField] private string serverIp = "127.0.0.1";
    [SerializeField] private ushort serverPort = 27020; // 27020 = Valve's example_chat server, 7777 = your gnsEcho

    private NetworkingSockets client;
    private NetworkingUtils utils;
    private uint connection;
    private StatusCallback statusCallback; // keep it in a field, otherwise the GC can collect it while native code still uses it
    private readonly NetworkingMessage[] messages = new NetworkingMessage[20];
    private bool initialized;

    private void Start()
    {
        var error = new StringBuilder(Library.maxErrorMessageLength);
        if (!Library.Initialize(error))
        {
            Debug.LogError("[GNS] Initialize failed: " + error);
            return;
        }
        initialized = true;

        client = new NetworkingSockets();
        utils = new NetworkingUtils();

        statusCallback = OnStatusChanged;
        utils.SetStatusCallback(statusCallback);

        string ip = serverIp.Trim(); // a pasted IP often ends with a space
        if (!System.Net.IPAddress.TryParse(ip, out _))
        {
            Debug.LogError("[GNS] Invalid Server Ip: '" + serverIp + "'");
            return;
        }

        Address address = new Address();
        address.SetAddress(ip, serverPort);
        connection = client.Connect(ref address);
        Debug.Log("[GNS] Connecting to " + ip + ":" + serverPort);
    }

    // Called from client.RunCallbacks(), so it runs on Unity's main thread.
    private void OnStatusChanged(ref StatusInfo info)
    {
        switch (info.connectionInfo.state)
        {
            case ConnectionState.Connected:
                Debug.Log("[GNS] Connected");
                Send("hello from Unity");
                break;

            case ConnectionState.ClosedByPeer:
            case ConnectionState.ProblemDetectedLocally:
                Debug.LogWarning("[GNS] Disconnected: " + info.connectionInfo.endDebug);
                client.CloseConnection(info.connection);
                if (info.connection == connection)
                    connection = 0;
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
            Debug.Log("[GNS] Received: " + Encoding.UTF8.GetString(buffer));
            messages[i].Destroy(); // always free the message
        }
    }

    public void Send(string text)
    {
        if (client == null || connection == 0)
            return;

        client.SendMessageToConnection(connection, Encoding.UTF8.GetBytes(text), SendFlags.Reliable);
        Debug.Log("[GNS] Sent: " + text);
    }

    private void OnApplicationQuit()
    {
        if (client != null && connection != 0)
            client.CloseConnection(connection);

        utils?.Dispose();

        if (initialized)
            Library.Deinitialize();

        client = null;
        initialized = false;
    }
}
