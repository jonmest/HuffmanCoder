cmake_minimum_required(VERSION 3.30)
project(HuffmanCoding)

set(CMAKE_CXX_STANDARD 20)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -fno-rtti -fno-exceptions")
endif()

add_executable(HuffmanCoding main.cpp
)
