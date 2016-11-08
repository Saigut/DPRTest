cmake_minimum_required(VERSION 3.2)
project(DPRTest C)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-error=unused-variable -Wno-xxxxx")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-unused-variable")

set(SERVER_SRC server.c common.c)
set(CHECK_SERVER_SRC check_server.c common.c)
set(CLIENT_SRC pr_client.c common.c)

add_executable(server ${SERVER_SRC})
add_executable(check_server ${CHECK_SERVER_SRC})
add_executable(pr_client ${CLIENT_SRC})

target_link_libraries(server pthread)
target_link_libraries(pr_client pthread)