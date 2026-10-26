cmake_minimum_required(VERSION 3.10)
project(pubsub-lite VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_executable(broker Infra/broker.cpp)
target_link_libraries(broker ws2_32)

add_executable(bench Infra/bench.cpp Node/node.cpp)
target_link_libraries(bench ws2_32)
