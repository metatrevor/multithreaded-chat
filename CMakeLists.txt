cmake_minimum_required(VERSION 2.8.4)
project(kubonga_chat_)

set(SERVER_INCLUDE_FILES server/src/server.h)
set(SERVER_SOURCE_FILES server/src/server.c)
find_package (Threads)
add_executable(chat_server ${SERVER_SOURCE_FILES} ${SERVER_INCLUDE_FILES})
target_link_libraries (chat_server ${CMAKE_THREAD_LIBS_INIT})

set(CLIENT_INCLUDE_FILES client/src/client.h)
set(CLIENT_SOURCE_FILES client/src/client.c )
add_executable(chat_client ${CLIENT_SOURCE_FILES} ${CLIENT_INCLUDE_FILES})
target_link_libraries (chat_client ${CMAKE_THREAD_LIBS_INIT})