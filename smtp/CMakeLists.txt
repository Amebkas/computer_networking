cmake_minimum_required(VERSION 3.10)

project(SmtpClientProject)

set(CXX_STANDARD_REQUIRED ON)
set(COMPILER_FLAGS "-Wall -Wextra")

set(targetName smtp_client)
set(sourceFiles main.cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")

add_executable(${targetName} ${sourceFiles})
