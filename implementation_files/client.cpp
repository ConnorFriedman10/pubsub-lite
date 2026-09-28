#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char* SERVER_IP = "127.0.0.1";
const int   PORT      = 9999;

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    int result = 1;

    do {
        // Step 1: Turn on Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            { std::cerr << "WSAStartup() failed\n"; break; }

        // Step 2: Create the socket
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
            { std::cerr << "socket() failed: " << WSAGetLastError() << "\n"; break; }

        // Step 3: Fill in the server's address
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port   = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

        // Step 4: Connect to the server
        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
            { std::cerr << "connect() failed: " << WSAGetLastError() << "  Is server.exe running?\n"; break; }

        std::cout << "Connected to server!\n";

        // Step 5: Send a message
        const char* message = "Hello from the client!";
        int bytesSent = send(sock, message, (int)strlen(message), 0);
        if (bytesSent == SOCKET_ERROR)
            std::cerr << "send() failed: " << WSAGetLastError() << "\n";
        else
            std::cout << "Message sent.\n";

        result = 0;
    } while (false);

    // Step 6: Clean up — runs whether we succeeded or hit any error above
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();
    return result;
}
