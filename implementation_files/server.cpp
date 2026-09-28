#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") //address lack of cmake

const int PORT = 9999;
const int BUFFER_SIZE = 512;

int main() {
    WSADATA wsaData;
    SOCKET listenSock = INVALID_SOCKET, clientSock = INVALID_SOCKET;
    int result = 1;

    do {
        // Step 1: Turn on Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) //turns on the networking library, requests version 2.2
            { std::cerr << "WSAStartup() failed\n"; break; } //line for failure to connect library 2.2

        // Step 2: Create the listening socket
        listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSock == INVALID_SOCKET) //check for if socket isn't valid
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 3: Bind to port 9999 on all interfaces
        sockaddr_in serverAddr{};
        serverAddr.sin_family      = AF_INET;
        serverAddr.sin_port        = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
            { std::cerr << "bind() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 4: Start listening (queue up to 1 pending connection)
        if (listen(listenSock, 1) == SOCKET_ERROR) //queues up the server to listen for incoming requests
            { std::cerr << "listen() failed: " << WSAGetLastError() << "\n"; break; }

        std::cout << "Server is listening on port " << PORT << "...\n";

        // Step 5: Block until a client connects
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);
        clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock == INVALID_SOCKET)
            { std::cerr << "accept() failed: " << WSAGetLastError() << "\n"; break; }

        std::cout << "A client connected!\n";

        // Step 6: Read the message the client sends
        char buffer[BUFFER_SIZE] = {};
        int bytesReceived = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived > 0)
            std::cout << "Received message: " << buffer << "\n";
        else
            std::cerr << "recv() failed or connection closed: " << WSAGetLastError() << "\n";

        result = 0;
    } while (false);

    // Step 7: Clean up — runs whether we succeeded or hit any error above
    if (clientSock != INVALID_SOCKET) closesocket(clientSock);
    if (listenSock != INVALID_SOCKET) closesocket(listenSock);
    WSACleanup();
    return result;
}
