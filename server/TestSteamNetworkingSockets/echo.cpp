// Minimal GameNetworkingSockets echo server.
// Every message a client sends is printed and sent back to that client as "Server got: <text>".
// Usage: ./build/gnsEcho [port]   (default 7777)

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>

ISteamNetworkingSockets* net = nullptr;
HSteamListenSocket listenSock = k_HSteamListenSocket_Invalid;
HSteamNetPollGroup pollGroup = k_HSteamNetPollGroup_Invalid;

void onStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting:
            // A client wants to connect: accept it and put it in our poll group
            if (net->AcceptConnection(info->m_hConn) != k_EResultOK) {
                net->CloseConnection(info->m_hConn, 0, nullptr, false);
                printf("Can't accept connection (it was already closed?)\n");
                break;
            }
            net->SetConnectionPollGroup(info->m_hConn, pollGroup);
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            printf("Client connected: %s\n", info->m_info.m_szConnectionDescription);
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            printf("Client disconnected: %s (%s)\n", info->m_info.m_szConnectionDescription, info->m_info.m_szEndDebug);
            net->CloseConnection(info->m_hConn, 0, nullptr, false);
            break;

        default:
            break;
    }
}

int main(int argc, char** argv) {
    int port = argc > 1 ? atoi(argv[1]) : 7777;

    SteamNetworkingErrMsg err;
    if (!GameNetworkingSockets_Init(nullptr, err)) {
        printf("GameNetworkingSockets_Init failed: %s\n", err);
        return 1;
    }
    net = SteamNetworkingSockets();

    SteamNetworkingIPAddr addr;
    addr.Clear();
    addr.m_port = (uint16)port;

    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)onStatusChanged);

    listenSock = net->CreateListenSocketIP(addr, 1, &opt);
    if (listenSock == k_HSteamListenSocket_Invalid) {
        printf("Failed to listen on port %d\n", port);
        return 1;
    }
    pollGroup = net->CreatePollGroup();
    printf("Listening on port %d\n", port);
    fflush(stdout);

    while (true) {
        net->RunCallbacks();

        ISteamNetworkingMessage* msgs[32];
        int n = net->ReceiveMessagesOnPollGroup(pollGroup, msgs, 32);
        for (int i = 0; i < n; i++) {
            std::string text((const char*)msgs[i]->m_pData, msgs[i]->m_cbSize);
            printf("Received: %s\n", text.c_str());

            std::string reply = "Server got: " + text;
            net->SendMessageToConnection(msgs[i]->m_conn, reply.data(), (uint32)reply.size(),
                                         k_nSteamNetworkingSend_Reliable, nullptr);
            msgs[i]->Release();
        }
        fflush(stdout);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
