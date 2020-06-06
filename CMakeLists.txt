cmake_minimum_required(VERSION 3.16)
project(radio)

set(CMAKE_CXX_STANDARD 14)

add_executable(radio main.cpp radio.h radio.cpp err.cpp err.h speaker.h speaker.cpp protocol.h protocol.cpp)
add_executable(client client_main.cpp err.cpp err.h telnet_controller.h telnet_controller.cpp client.h client.cpp protocol.h protocol.cpp)