#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "protocol.h"
#pragma comment(lib, "ws2_32.lib")

const int PORT        = 9999;
const int BUFFER_SIZE = 4096;

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr{};
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    ::bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSock, SOMAXCONN);

    std::cout << "Broker listening on port " << PORT << "...\n";

    fd_set master;
    FD_ZERO(&master);
    FD_SET(listenSock, &master);

    std::vector<SOCKET> clients;
    std::unordered_map<std::string, std::vector<SOCKET>> registry;
    // Per-socket accumulation buffer: handles messages split across recv() calls.
    std::unordered_map<SOCKET, std::vector<char>> rxbufs;

    while (true) {
        fd_set readSet = master;

        if (select(0, &readSet, NULL, NULL, NULL) == SOCKET_ERROR) {
            std::cerr << "select() failed: " << WSAGetLastError() << "\n";
            break;
        }

        if (FD_ISSET(listenSock, &readSet)) {
            sockaddr_in clientAddr{};
            int clientAddrLen = sizeof(clientAddr);
            SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSock != INVALID_SOCKET) {
                FD_SET(clientSock, &master);
                clients.push_back(clientSock);
                rxbufs[clientSock] = {};
            }
        }

        for (SOCKET& client : clients) {
            if (!FD_ISSET(client, &readSet)) continue;

            char chunk[BUFFER_SIZE];
            int n = recv(client, chunk, BUFFER_SIZE, 0);

            if (n <= 0) {
                FD_CLR(client, &master);
                SOCKET dead = client;
                closesocket(dead);
                client = INVALID_SOCKET;
                rxbufs.erase(dead);

                for (auto& [topic, sockets] : registry)
                    sockets.erase(std::remove(sockets.begin(), sockets.end(), dead), sockets.end());

            } else {
                auto& rxbuf = rxbufs[client];
                rxbuf.insert(rxbuf.end(), chunk, chunk + n);

                size_t offset = 0;
                while (offset < rxbuf.size()) {
                    // Need at least 2 bytes for the type + topicLen header.
                    if (offset + 2 > rxbuf.size()) break;

                    uint8_t msgType  = (uint8_t)rxbuf[offset];
                    uint8_t topicLen = (uint8_t)rxbuf[offset + 1];

                    // Wait for the full topic to arrive.
                    if (offset + 2 + topicLen > rxbuf.size()) break;

                    std::string topic(rxbuf.data() + offset + 2, topicLen);

                    if (msgType == MSG_SUBSCRIBE) {
                        registry[topic].push_back(client);
                        std::cout << "Node subscribed to \"" << topic << "\".\n";
                        offset += 2 + topicLen;

                    } else if (msgType == MSG_PUBLISH) {
                        // Wait for payloadLen byte and full payload.
                        if (offset + 2 + topicLen + 1 > rxbuf.size()) break;
                        uint8_t payloadLen = (uint8_t)rxbuf[offset + 2 + topicLen];
                        if (offset + 2 + topicLen + 1 + payloadLen > rxbuf.size()) break;

                        const char* payload = rxbuf.data() + offset + 2 + topicLen + 1;

                        char delivery[258];  // 1-byte type + 1-byte len + up to 255 bytes payload
                        delivery[0] = MSG_DATA;
                        delivery[1] = (char)payloadLen;
                        memcpy(delivery + 2, payload, payloadLen);

                        for (SOCKET& sub : registry[topic]) {
                            if (send(sub, delivery, 2 + payloadLen, 0) == SOCKET_ERROR) {
                                closesocket(sub);
                                sub = INVALID_SOCKET;
                            }
                        }
                        registry[topic].erase(
                            std::remove(registry[topic].begin(), registry[topic].end(), (SOCKET)INVALID_SOCKET),
                            registry[topic].end()
                        );
                        offset += 2 + topicLen + 1 + payloadLen;

                    } else {
                        break;
                    }
                }

                // Drop consumed bytes from the front of the buffer.
                if (offset > 0)
                    rxbuf.erase(rxbuf.begin(), rxbuf.begin() + offset);
            }
        }

        clients.erase(std::remove(clients.begin(), clients.end(), (SOCKET)INVALID_SOCKET), clients.end());
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}
