cmake_minimum_required(VERSION 3.28.3)
project(NeighborDiscoveryService)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(neighbor_lib
        src/interface_manager.cpp
        src/event_poll.cpp
        src/neighbor_manager.cpp
        include/neighbor_discovery/interface_manager.h
        include/neighbor_discovery/event_poll.h
        include/neighbor_discovery/interface.h
        include/neighbor_discovery/neighbor_manager.h
)

add_executable(NeighborDiscoveryService
        apps/neighbor_service.cpp
)

target_link_libraries(NeighborDiscoveryService neighbor_lib)

add_executable(NeighborCli apps/neighbor_cli.cpp)

target_link_libraries(NeighborCli neighbor_lib)

# Grant raw socket capabilities after build
add_custom_command(TARGET NeighborDiscoveryService POST_BUILD
        COMMAND sudo setcap cap_net_raw+ep $<TARGET_FILE:NeighborDiscoveryService>
        COMMENT "Setting CAP_NET_RAW capability"
)

