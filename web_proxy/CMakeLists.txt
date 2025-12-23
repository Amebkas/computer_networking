cmake_minimum_required(VERSION 3.10)
project(HttpProxyServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_NAME http_proxy)
set(SOURCE_FILES main.cpp)
set(COMPILER_FLAGS "-Wall -Wextra")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")

add_executable(${TARGET_NAME} ${SOURCE_FILES})