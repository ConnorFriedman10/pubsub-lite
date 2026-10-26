#include <iostream>
#include <cstring>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <functional>
#include "node.h"

Node::Node(std::string name) : name_(name), sock_(INVALID_SOCKET), pubSock_(INVALID_SOCKET) {}

Node::~Node() {
    if (sock_    != INVALID_SOCKET) closesocket(sock_);
    if (pubSock_ != INVALID_SOCKET) closesocket(pubSock_);
}

void Node::publish(std::string topic, std::string message) {
    // Lazily connect once and reuse the socket for all subsequent publishes.
    if (pubSock_ == INVALID_SOCKET) {
        pubSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (pubSock_ == INVALID_SOCKET)
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; return; }

        sockaddr_in brokerAddr{};
        brokerAddr.sin_family = AF_INET;
        brokerAddr.sin_port   = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &brokerAddr.sin_addr);

        if (connect(pubSock_, (sockaddr*)&brokerAddr, sizeof(brokerAddr)) == SOCKET_ERROR) {
            std::cerr << "connect() failed: " << WSAGetLastError() << "  Is broker running?\n";
            closesocket(pubSock_);
            pubSock_ = INVALID_SOCKET;
            return;
        }
    }

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
    if (send(pubSock_, buffer, totalLen, 0) == SOCKET_ERROR) {
        std::cerr << "send() failed: " << WSAGetLastError() << "\n";
        closesocket(pubSock_);
        pubSock_ = INVALID_SOCKET;
    }
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

    // Accumulation buffer handles the case where one recv() contains multiple
    // framed messages or a message split across two recv() calls.
    std::vector<char> rxbuf;

    while (true) {
        char chunk[BUFFER_SIZE] = {};
        int n = recv(sock_, chunk, BUFFER_SIZE - 1, 0);
        if (n <= 0) {
            std::cerr << "Broker disconnected or recv() failed.\n";
            break;
        }
        rxbuf.insert(rxbuf.end(), chunk, chunk + n);

        // Drain all complete [MSG_DATA][payloadLen][payload] frames.
        while (rxbuf.size() >= 2 && (uint8_t)rxbuf[0] == MSG_DATA) {
            uint8_t payLen = (uint8_t)rxbuf[1];
            if (rxbuf.size() < (size_t)(2 + payLen)) break;  // partial frame
            callback_(std::string(rxbuf.data() + 2, payLen));
            rxbuf.erase(rxbuf.begin(), rxbuf.begin() + 2 + payLen);
        }
    }

    if (sock_ != INVALID_SOCKET) { closesocket(sock_); sock_ = INVALID_SOCKET; }
}
