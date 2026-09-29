#include <iostream>
#include <fstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <functional>

using namespace std;

const char* SERVER_IP = "127.0.0.1";
const int   PORT      = 9999;
const int BUFFER_SIZE = 512;

class Node {
    public:
        Node(std::string name);
        void publish(std::string topic, std::string message);
        void subscribe(std::string topic, std::function<void(const std::string&)> callback);
        void spin();

    private:
        std::string name_;
        SOCKET sock_;
        SOCKET listenSock_;
        std::string subscribedTopic_;
        std::string publishTopic_;
        std::function<void(const std::string&)> callback_;
};