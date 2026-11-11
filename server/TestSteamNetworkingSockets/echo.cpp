#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>

ISteamNetworkingSockets* net = nullptr;
HSteamListenSocket listenSock = k_HSteamListenSocket_Invalid;
HSteamNetPollGroup pollGroup = k_HSteamNetPollGroup_Invalid;

void onStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting:
            // TODO: net->AcceptConnection(info->m_hConn), then net->SetConnectionPollGroup(info->m_hConn, pollGroup)
            break;
        case k_ESteamNetworkingConnectionState_Connected:
            // TODO: printf "client connected"
            break;
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            // TODO: net->CloseConnection(info->m_hConn, 0, nullptr, false)
            break;
        default: break;
    }
}

int main(int argc, char** argv) {
    int port = argc > 1 ? atoi(argv[1]) : 7777;

    // TODO 1: SteamNetworkingErrMsg err; if (!GameNetworkingSockets_Init(nullptr, err)) { print err; return 1; }
    // TODO 2: net = SteamNetworkingSockets();
    // TODO 3: SteamNetworkingIPAddr addr; addr.Clear(); addr.m_port = port;
    // TODO 4: SteamNetworkingConfigValue_t opt;
    //         opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)onStatusChanged);
    // TODO 5: listenSock = net->CreateListenSocketIP(addr, 1, &opt);
    //         pollGroup = net->CreatePollGroup();
    //         printf("Listening on port %d\n", port);

    (void)port;

    while (true) {
        // TODO: remove this check once TODO 2 is done
        if (!net) { printf("net is null - finish TODO 1 and 2\n"); return 1; }

        net->RunCallbacks();

        // TODO 6: ISteamNetworkingMessage* msgs[32];
        //         int n = net->ReceiveMessagesOnPollGroup(pollGroup, msgs, 32);
        //         for each msg: print it, send it back with
        //         net->SendMessageToConnection(msg->m_conn, msg->m_pData, msg->m_cbSize,
        //                                      k_nSteamNetworkingSend_Reliable, nullptr);
        //         then msg->Release();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
