cmake_minimum_required(VERSION 3.10)
project(RdtpProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMPILER_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")

add_executable(rdt_sender sender.cpp)
add_executable(rdt_receiver receiver.cpp)