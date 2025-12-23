cmake_minimum_required(VERSION 3.10)
project(UdpPinger)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SERVER_TARGET udp_pinger_server)
set(CLIENT_TARGET udp_pinger_client)

add_executable(${SERVER_TARGET} udp_pinger_server.cpp)
add_executable(${CLIENT_TARGET} udp_pinger_client.cpp)