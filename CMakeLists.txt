cmake_minimum_required(VERSION 3.10)
project(HPCChatApp VERSION 1.0)

# Enforce modern C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Locate the locally built AWS SDK (SQS component)
find_package(AWSSDK REQUIRED COMPONENTS sqs)

# 1. Server Executable
add_executable(ChatServer main.cpp)
target_link_libraries(ChatServer ${AWSSDK_LINK_LIBRARIES} pthread)

# 2. Client Executable
add_executable(ChatClient client.cpp)
target_link_libraries(ChatClient pthread)