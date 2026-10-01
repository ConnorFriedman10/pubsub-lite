#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "protocol.h"
#pragma comment(lib, "ws2_32.lib")

const int PORT        = 9999;
const int BUFFER_SIZE = 512;

int main() {
    // Part 1: Start Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Part 3: Broker setup: create socket, bind, listen
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr{};
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    ::bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSock, SOMAXCONN);

    std::cout << "Broker listening on port " << PORT << "...\n";

    // Part 4: select() loop — watch many sockets at once
    fd_set master;                                              // the permanent set of all sockets to watch
    FD_ZERO(&master);
    FD_SET(listenSock, &master);

    std::vector<SOCKET> clients;                               // all currently connected node sockets
    std::unordered_map<std::string, std::vector<SOCKET>> registry; // topic -> list of subscriber sockets

    while (true) {
        fd_set readSet = master;    // copy — select() modifies the set it receives

        if (select(0, &readSet, NULL, NULL, NULL) == SOCKET_ERROR) {
            std::cerr << "select() failed: " << WSAGetLastError() << "\n";
            break;
        }

        // A new node is trying to connect
        if (FD_ISSET(listenSock, &readSet)) {
            sockaddr_in clientAddr{};
            int clientAddrLen = sizeof(clientAddr);
            SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSock != INVALID_SOCKET) {
                FD_SET(clientSock, &master);
                clients.push_back(clientSock);
                std::cout << "New node connected.\n";
            }
        }

        // Part 5: Check each connected node for incoming messages
        for (SOCKET& client : clients) {
            if (!FD_ISSET(client, &readSet)) continue;

            char buffer[BUFFER_SIZE] = {};
            int bytesReceived = recv(client, buffer, BUFFER_SIZE - 1, 0);

            if (bytesReceived <= 0) {
                // Node disconnected — mark for removal
                std::cout << "A node disconnected.\n";
                FD_CLR(client, &master);
                closesocket(client);
                client = INVALID_SOCKET;
            } else {
                // Parse the topic out of the buffer
                uint8_t topicLen = buffer[1];
                std::string topic(buffer + 2, topicLen);

                if (buffer[0] == MSG_SUBSCRIBE) {
                    // Register this socket as a subscriber for the topic
                    registry[topic].push_back(client);
                    std::cout << "Node subscribed to \"" << topic << "\".\n";

                } else if (buffer[0] == MSG_PUBLISH) {
                    // Extract the payload sitting after the topic
                    uint8_t payloadLen = buffer[2 + topicLen];
                    const char* payload = buffer + 2 + topicLen + 1;

                    std::cout << "Routing \"" << topic << "\" to "
                              << registry[topic].size() << " subscriber(s).\n";

                    // Forward to every subscriber on this topic
                    for (SOCKET& sub : registry[topic]) {
                        if (send(sub, payload, payloadLen, 0) == SOCKET_ERROR) {
                            // Subscriber is gone — mark for removal from registry
                            closesocket(sub);
                            sub = INVALID_SOCKET;
                        }
                    }

                    // Clean up any stale subscriber sockets
                    registry[topic].erase(
                        std::remove(registry[topic].begin(), registry[topic].end(), (SOCKET)INVALID_SOCKET),
                        registry[topic].end()
                    );
                }
            }
        }

        // Remove disconnected clients from the client list
        clients.erase(
            std::remove(clients.begin(), clients.end(), (SOCKET)INVALID_SOCKET),
            clients.end()
        );
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}
