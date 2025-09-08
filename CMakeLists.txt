cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(server src/server.cpp src/logger.cpp src/protocol.cpp src/thread_pool.cpp)
add_executable(client src/client.cpp src/logger.cpp src/protocol.cpp src/thread_pool.cpp)
