cmake_minimum_required(VERSION 3.10)
project(DiskUsageVisualizer VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(DiskUsageVisualizer TestingFiles/playfile.cpp)

set_property(TARGET DiskUsageVisualizer PROPERTY CXX_STANDARD 17)