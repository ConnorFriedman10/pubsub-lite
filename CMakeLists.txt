cmake_minimum_required(VERSION 3.10)
project(pubsub-lite VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_executable(pubsub-lite testing-files/playfile.cpp)
target_link_libraries(pubsub-lite ws2_32)

add_executable(server implementation_files/server.cpp)
target_link_libraries(server ws2_32)

add_executable(client implementation_files/client.cpp)
target_link_libraries(client ws2_32)
