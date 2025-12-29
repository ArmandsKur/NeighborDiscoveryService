cmake_minimum_required(VERSION 3.28.3)
project(NeighborDiscoveryService)

set(CMAKE_CXX_STANDARD 17)

add_executable(NeighborDiscoveryService main.cpp
        interface_manager.cpp
        interface_manager.h
        event_poll.cpp
        event_poll.h
        interface.h
        neighbor_manager.cpp
        neighbor_manager.h)
