#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const int PORT = 9999;
const int BUFFER_SIZE = 512;

int main() {
    // Step 1: Turn on Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup() failed\n";
        return 1;
    }

    // Step 2: Create the listening socket
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Step 3: Bind to port 9999 on all interfaces
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    // Step 4: Start listening (queue up to 1 pending connection)
    if (listen(listenSock, 1) == SOCKET_ERROR) {
        std::cerr << "listen() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << "...\n";

    // Step 5: Block until a client connects
    sockaddr_in clientAddr{};
    int clientAddrLen = sizeof(clientAddr);
    SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "accept() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "A client connected!\n";

    // Step 6: Read the message the client sends
    char buffer[BUFFER_SIZE] = {};
    int bytesReceived = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
    if (bytesReceived > 0) {
        std::cout << "Received message: " << buffer << "\n";
    } else {
        std::cerr << "recv() failed or connection closed: " << WSAGetLastError() << "\n";
    }

    // Step 7: Clean up
    closesocket(clientSock);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}
