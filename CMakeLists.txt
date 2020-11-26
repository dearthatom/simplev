cmake_minimum_required(VERSION 3.8)
project(simplev)

set(CMAKE_CXX_STANDARD 11)

include_directories(common simplev/data )

add_library(simplev SHARED

        simplev/poller.cpp
        simplev/tcpconn.cpp
        simplev/tcpserver.cpp
        simplev/eventloop.cpp
        simplev/channel.cpp
        simplev/data/net.cpp
        common/util.cpp)
target_include_directories(simplev PUBLIC common simplev/data)
#target_link_libraries(simplev ld)