#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Node/node.h"#pragma comment(lib, "ws2_32.lib") //address lack of cmake

const int PORT = 9999;
const int BUFFER_SIZE = 512;

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    Node sub("listener");
    sub.subscribe("chatter", [](const std::string& msg) {
        std::cout << "Received: " << msg << "\n";
    });
    sub.spin();

    WSACleanup();
    return 0;
}