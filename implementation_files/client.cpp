#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char* SERVER_IP = "127.0.0.1";
const int   PORT      = 9999;

int main() {
    // Step 1: Turn on Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup() failed\n";
        return 1;
    }

    // Step 2: Create the socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Step 3: Fill in the server's address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // Step 4: Connect to the server
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect() failed: " << WSAGetLastError()
                  << "  Is server.exe running?\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!\n";

    // Step 5: Send a message
    const char* message = "Hello from the client!";
    int bytesSent = send(sock, message, (int)strlen(message), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "send() failed: " << WSAGetLastError() << "\n";
    } else {
        std::cout << "Message sent.\n";
    }

    // Step 6: Clean up
    closesocket(sock);
    WSACleanup();
    return 0;
}
