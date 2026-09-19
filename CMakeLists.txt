cmake_minimum_required(VERSION 3.10)
project(pubsub-lite VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(pubsub-lite TestingFiles/playfile.cpp)

set_property(TARGET pubsub-lite PROPERTY CXX_STANDARD 17)

target_link_libraries(pubsub-lite ws2_32)