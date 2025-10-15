cmake_minimum_required(VERSION 3.10)

project(TcpClientServer CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(client Client.cpp
	common.h
)
add_executable(server Server.cpp
	common.h
)