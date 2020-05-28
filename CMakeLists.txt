cmake_minimum_required(VERSION 3.16)
project(radio)

set(CMAKE_CXX_STANDARD 14)

add_executable(radio main.cpp radio.h radio.cpp err.cpp err.h)