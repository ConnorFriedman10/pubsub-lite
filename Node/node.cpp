#include <iostream>
#include <fstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <functional>
#include "node.h"

void Node::publish(std::string topic, std::string message) { //used to be client.cpp
    do {
        // Step 2: Create the socket
        sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_ == INVALID_SOCKET)
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 3: Fill in the server's address
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port   = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

        // Step 4: Connect to the server
        if (connect(sock_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
            { std::cerr << "connect() failed: " << WSAGetLastError() << "  Is server.exe running?\n"; break; }

        std::cout << "Connected to server!\n";

        // Step 5: Send a message
        const char* message = "Hello from the client!";
        int bytesSent = send(sock_, message, (int)strlen(message), 0);
        if (bytesSent == SOCKET_ERROR)
            std::cerr << "send() failed: " << WSAGetLastError() << "\n";
        else
            std::cout << "Message sent.\n";

    } while (false);
};

void Node::subscribe(std::string topic, std::function<void(const std::string&)> callback) {
    do {
        // Step 2: Create the listening socket
        listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSock_ == INVALID_SOCKET) //check for if socket isn't valid
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 3: Bind to port 9999 on all interfaces
        sockaddr_in serverAddr{};
        serverAddr.sin_family      = AF_INET;
        serverAddr.sin_port        = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        if (::bind(listenSock_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
            { std::cerr << "bind() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 4: Start listening (queue up to 1 pending connection)
        if (listen(listenSock_, 1) == SOCKET_ERROR) //queues up the server to listen for incoming requests
            { std::cerr << "listen() failed: " << WSAGetLastError() << "\n"; break; }

        std::cout << "Server is listening on port " << PORT << "...\n";

        // Step 5: Block until a client connects
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);
        listenSock_ = accept(listenSock_, (sockaddr*)&clientAddr, &clientAddrLen);
        if (sock_ == INVALID_SOCKET)
            { std::cerr << "accept() failed: " << WSAGetLastError() << "\n"; break; }

        std::cout << "A client connected!\n";

        // Step 6: Read the message the client sends
        char buffer[BUFFER_SIZE] = {};
        int bytesReceived = recv(sock_, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived > 0)
            std::cout << "Received message: " << buffer << "\n";
        else
            std::cerr << "recv() failed or connection closed: " << WSAGetLastError() << "\n";

    } while (false);
};

void Node::spin() {
    do {
        // Step 2: Create the listening socket
        listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSock_ == INVALID_SOCKET) //check for if socket isn't valid
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 3: Bind to port 9999 on all interfaces
        sockaddr_in serverAddr{};
        serverAddr.sin_family      = AF_INET;
        serverAddr.sin_port        = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        if (::bind(listenSock_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
            { std::cerr << "bind() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 4: Start listening (queue up to 1 pending connection)
        if (listen(listenSock_, 1) == SOCKET_ERROR) //queues up the server to listen for incoming requests
            { std::cerr << "listen() failed: " << WSAGetLastError() << "\n"; break; }

        std::cout << "Node \"" << name_ << "\" listening on port " << PORT << "...\n";

        // Step 5: Loop forever — accept, receive, fire callback
        while (true) {
            sockaddr_in clientAddr{};
            int clientAddrLen = sizeof(clientAddr);
            SOCKET clientSock = accept(listenSock_, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSock == INVALID_SOCKET)
                { std::cerr << "accept() failed: " << WSAGetLastError() << "\n"; break; }

            std::cout << "A client connected!\n";

            // Step 6: Read the message and fire the callback
            char buffer[BUFFER_SIZE] = {};
            int bytesReceived = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived > 0)
                callback_(buffer);
            else
                std::cerr << "recv() failed or connection closed: " << WSAGetLastError() << "\n";

            closesocket(clientSock);
        }

    } while (false);

    if (listenSock_ != INVALID_SOCKET) closesocket(listenSock_);
}