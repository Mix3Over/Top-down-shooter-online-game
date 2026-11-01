using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public static class NetworkService
{
    public enum FailReason
    {
        Unknown,
        Timeout,
        Canceled
    }

    public struct CreateSocketResult
    {
        public readonly bool Success;
        public readonly Socket Socket;
        public readonly FailReason FailReason;
        public readonly string ErrorMessage;

        public CreateSocketResult(FailReason failReason, string errorMessage = null)
        {
            Success = false;
            Socket = null;
            FailReason = failReason;
            ErrorMessage = errorMessage;
        }

        public CreateSocketResult(Socket socket)
        {
            Success = true;
            Socket = socket;
            FailReason = default;
            ErrorMessage = null;
        }
    }

    public static async Task<CreateSocketResult> CreateTcpSocket(IPAddress ipAddr, int port, float timeout, CancellationToken cancellationToken)
    {
        IPEndPoint ipEndPoint = new(ipAddr, port);

        Socket socket = new(ipEndPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

        Task connectTask = socket.ConnectAsync(ipEndPoint);
        Task timeoutTask = Task.Delay(TimeSpan.FromSeconds(timeout), cancellationToken: cancellationToken);

        try
        {
            Task finished = await Task.WhenAny(connectTask, timeoutTask);

            if (finished == timeoutTask){
                socket.Dispose();
                return new CreateSocketResult(FailReason.Timeout);
            }

            await connectTask;
        }
        catch (OperationCanceledException)
        {
            socket.Dispose();
            return new CreateSocketResult(FailReason.Canceled);
        }
        catch (Exception ex) {
            socket.Dispose();
            return new CreateSocketResult(FailReason.Unknown, ex.Message);
        }

        if (!socket.Connected)
        {
            return new CreateSocketResult(FailReason.Unknown);
        }

        return new CreateSocketResult(socket);
    }

    public static void CreateUdpClient(IPAddress ipAddr, int port, out UdpClient udpClient, out int localUdpPort)
    {
        udpClient = new UdpClient();
        udpClient.Connect(ipAddr.ToString(), port);

        IPEndPoint localEndPoint = (IPEndPoint)udpClient.Client.LocalEndPoint;
        localUdpPort = localEndPoint.Port;
    }

    public static async void StartUdpLoopAsync(UdpClient udpClient, Action<string> onMessageReceived, CancellationToken cancellationToken)
    {
        Debug.Log("[UDP] Start read");

        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                UdpReceiveResult result = await udpClient.ReceiveAsync();
                string msg = Encoding.UTF8.GetString(result.Buffer).TrimEnd('~');
                onMessageReceived?.Invoke(msg);
            }
            catch when (cancellationToken.IsCancellationRequested)
            {
                break;
            }
            catch (Exception ex)
            {
                Debug.LogError("[UDP] Read exception: " + ex.Message);
            }
        }
        Debug.Log("[UDP] End read");
    }

    public static async void StartTcpLoopAsync(Socket socket, Action<string> onMessageReceived, Action onDisconnected, CancellationToken cancellationToken)
    {
        Debug.Log("[TCP] Started read loop...");
        string message = "";

        while (true) {
            try {
                while (message.IndexOf('~') == -1) {
                    string part = await WaitForMessageAsync(socket, cancellationToken);
                    Debug.Log("[TCP] Received part: " + part);
                    if (part == string.Empty) {
                        onDisconnected?.Invoke();
                        return;
                    }
                    message += part;
                }
                string messageToProcess = message[..message.IndexOf('~')];

                if(message.IndexOf('~') + 1 >= message.Length)
                    message = "";
                else
                    message = message[(message.IndexOf('~') + 1)..];

                Debug.Log("[TCP] Received: " + messageToProcess);
                onMessageReceived?.Invoke(messageToProcess);
            }
            catch (SocketException ex) {
                Debug.LogError("[TCP] Socket exception: " + ex.Message);
            }
        }
    }

    public static async Task SendMessageAsync(Socket socket, string message)
    {
        message += '~';
        byte[] messageBytes = Encoding.UTF8.GetBytes(message);
        _ = await socket.SendAsync(messageBytes, SocketFlags.None);
        Debug.Log($"[TCP] Sent message: \"{message}\"");
    }

    public static async Task SendMessageAsync(UdpClient udpClient, string message)
    {
        byte[] data = Encoding.UTF8.GetBytes(message);
        await udpClient.SendAsync(data, data.Length);
    }

    public static async Task<string> WaitForMessageAsync(Socket socket, CancellationToken token)
    {
        try
        {
            byte[] buffer = new byte[1_024];
            int received = await socket.ReceiveAsync(buffer, SocketFlags.None, cancellationToken: token);

            return Encoding.UTF8.GetString(buffer, 0, received);
        }
        catch (OperationCanceledException) { }
        catch (Exception ex) {
            Debug.LogError("[TCP] Read exception: " + ex.Message);
        }
        return string.Empty;
    }

    public static void CloseSocket(Socket socket)
    {
        if (socket == null)
            return;

        if(socket.Connected)
            socket.Shutdown(SocketShutdown.Both);
        socket.Close();
        socket.Dispose();
    }
}