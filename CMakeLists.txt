cmake_minimum_required(VERSION 3.10)
project(pubsub-lite VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_executable(pubsub-lite testing-files/playfile.cpp) #tells CMake to compile this .cpp file into a runnable .exe
target_link_libraries(pubsub-lite ws2_32) #windows socket, without this link network functions wouldn't work
#a socket is a software abstraction that lets two programs communicate, either on the same machine or across a network
#one side calls the client, and the other side picks up, and once connected both sides are able to communicate with one another

#this project uses a TCP socket, which guarantees that data arrives in order and without corruption, i.e. how web browsers
#load pages.

add_executable(server implementation_files/server.cpp)
target_link_libraries(server ws2_32)

add_executable(client implementation_files/client.cpp)
target_link_libraries(client ws2_32)

add_executable(broker Infra/broker.cpp)
target_link_libraries(broker ws2_32)
