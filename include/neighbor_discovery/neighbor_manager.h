#ifndef NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H
#include <algorithm>
#include <iostream>
#include <vector>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <chrono>

#include <array>
#include <map>
#include <unordered_map>

#include "neighbor_discovery/interface.h"

#define ETH_P_NEIGHBOR 0x88B5 //used one of the Local Experimental Ethertype
#define P_NEIGHBOR_SIZE 53 //define the size of total payload for neighbor protocol packet

#pragma pack(push, 1)
struct neighbor_payload {
    uint8_t  client_id[16];
    uint8_t  mac_addr[6];
    uint8_t  ip_family;       // AF_INET / AF_INET6 / 0
    union {
        struct in_addr  ipv4;
        struct in6_addr ipv6;
    };
};
#pragma pack(pop)

#pragma pack(push, 1)
struct cli_neighbor_payload {
    char local_ifname[IFNAMSIZ];
    uint8_t  client_id[16];
    uint8_t  mac_addr[6];
    uint8_t  ip_family;       // AF_INET / AF_INET6 / 0
    union {
        struct in_addr  ipv4;
        struct in6_addr ipv6;
    };
};
#pragma pack(pop)

//Store active connections for each neighbor ID
struct active_neighbor {
    std::map<int,struct neighbor_connection> active_connections;
};

struct neighbor_connection {
    std::array<uint8_t,16> neighbor_id;
    std::array<uint8_t,6> mac_addr;
    std::string local_ifname;
    std::chrono::time_point<std::chrono::steady_clock> last_seen;
    uint8_t  ip_family;       // AF_INET / AF_INET6 / 0
    union {
        struct in_addr  ipv4;
        struct in6_addr ipv6;
    };
};


class NeighborManager {
    public:
        bool init();
        int get_broadcast_recv_socket();
        int get_broadcast_send_socket();

        neighbor_payload construct_neighbor_payload(std::array<uint8_t,6> source_mac,ip_address ip_addr);
        cli_neighbor_payload construct_cli_neighbor_payload(neighbor_connection);
        void recv_broadcast(const std::unordered_map<int,ethernet_interface> &interface_list);
        void send_broadcast(int ifindex, std::array<uint8_t,6> source_mac, neighbor_payload payload);

        const std::map<std::array<uint8_t,16>, active_neighbor>& get_active_neighbors();

        void remove_unactive_connections();

        void cleanup();
    private:
        const std::array<uint8_t, 6> broadcast_mac = {0xff,0xff,0xff,0xff,0xff,0xff};
        //Socket fds
        int send_sockfd;
        int recv_sockfd;
        int create_broadcast_recv_socket();
        int create_broadcast_send_socket();

        std::map<std::array<uint8_t,16>, active_neighbor> neighbors;
        std::array<uint8_t,16> client_id;
        std::array<uint8_t,16> get_random_client_id();

        //helpers
        bool socket_set_nonblock(int sock_fd);
        ethhdr init_ethhdr(std::array<uint8_t,6> source_mac,std::array<uint8_t,6> dest_mac);
        sockaddr_ll init_sockaddr_ll(int ifindex, std::array<uint8_t,6> dest_mac);

};
#endif //NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H