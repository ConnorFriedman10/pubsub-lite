#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char* SERVER_IP = "127.0.0.1";
const int   PORT      = 9999;

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    Node pub("talker");
    pub.publish("chatter", "Hello from the publisher!");

    WSACleanup();
    return 0;
}
