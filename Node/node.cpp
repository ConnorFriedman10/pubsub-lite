#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <functional>
#include "node.h"

Node::Node(std::string name) : name_(name), sock_(INVALID_SOCKET) {}

void Node::publish(std::string topic, std::string message) {
    // Use a local socket so we never overwrite sock_, which subscribe() keeps open
    SOCKET pubSock = INVALID_SOCKET;

    do {
        pubSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (pubSock == INVALID_SOCKET)
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        sockaddr_in brokerAddr{};
        brokerAddr.sin_family = AF_INET;
        brokerAddr.sin_port   = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &brokerAddr.sin_addr);

        if (connect(pubSock, (sockaddr*)&brokerAddr, sizeof(brokerAddr)) == SOCKET_ERROR)
            { std::cerr << "connect() failed: " << WSAGetLastError() << "  Is broker running?\n"; break; }

        // Build the wire format: [MSG_PUBLISH][topicLen][topic][payloadLen][payload]
        char buffer[BUFFER_SIZE] = {};
        uint8_t topicLen   = (uint8_t)topic.size();
        uint8_t payloadLen = (uint8_t)message.size();

        buffer[0] = MSG_PUBLISH;
        buffer[1] = topicLen;
        memcpy(buffer + 2, topic.c_str(), topicLen);
        buffer[2 + topicLen] = payloadLen;
        memcpy(buffer + 2 + topicLen + 1, message.c_str(), payloadLen);

        int totalLen = 2 + topicLen + 1 + payloadLen;
        if (send(pubSock, buffer, totalLen, 0) == SOCKET_ERROR)
            std::cerr << "send() failed: " << WSAGetLastError() << "\n";
        else
            std::cout << "Published \"" << message << "\" to topic \"" << topic << "\".\n";

    } while (false);

    // Publish is fire-and-forget — close after each message
    if (pubSock != INVALID_SOCKET) closesocket(pubSock);
}

void Node::subscribe(std::string topic, std::function<void(const std::string&)> callback) {
    // Store topic and callback for spin() to use when messages arrive
    subscribedTopic_ = topic;
    callback_        = callback;

    do {
        // Connect to the broker and keep the connection open
        sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_ == INVALID_SOCKET)
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        sockaddr_in brokerAddr{};
        brokerAddr.sin_family = AF_INET;
        brokerAddr.sin_port   = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &brokerAddr.sin_addr);

        if (connect(sock_, (sockaddr*)&brokerAddr, sizeof(brokerAddr)) == SOCKET_ERROR)
            { std::cerr << "connect() failed: " << WSAGetLastError() << "  Is broker running?\n"; break; }

        // Send the SUBSCRIBE message: [MSG_SUBSCRIBE][topicLen][topic]
        char buffer[BUFFER_SIZE] = {};
        uint8_t topicLen = (uint8_t)topic.size();

        buffer[0] = MSG_SUBSCRIBE;
        buffer[1] = topicLen;
        memcpy(buffer + 2, topic.c_str(), topicLen);

        if (send(sock_, buffer, 2 + topicLen, 0) == SOCKET_ERROR)
            { std::cerr << "send() failed: " << WSAGetLastError() << "\n"; break; }

        std::cout << "Node \"" << name_ << "\" subscribed to \"" << topic << "\".\n";
        return; // leave sock_ open — spin() will recv() on it

    } while (false);

    // Only reached on error
    if (sock_ != INVALID_SOCKET) { closesocket(sock_); sock_ = INVALID_SOCKET; }
}

void Node::spin() {
    if (sock_ == INVALID_SOCKET)
        { std::cerr << "spin() called before subscribe().\n"; return; }

    std::cout << "Node \"" << name_ << "\" spinning...\n";

    // spin() blocks the calling thread for its entire lifetime.
    // If a node needs to both publish and subscribe, run spin() on a
    // std::thread and call publish() from the main thread.
    while (true) {
        char buffer[BUFFER_SIZE] = {};
        int bytesReceived = recv(sock_, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived > 0)
            callback_(std::string(buffer, bytesReceived));
        else {
            std::cerr << "Broker disconnected or recv() failed.\n";
            break;
        }
    }

    if (sock_ != INVALID_SOCKET) { closesocket(sock_); sock_ = INVALID_SOCKET; }
}
