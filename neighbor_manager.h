#ifndef NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H
#include <algorithm>
#include <iostream>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>

#include <array>
#include <unordered_map>
#include <unistd.h>
#include "interface.h"

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

//For each connection, display the local interface name, neighbor's MAC address in that specific network, and neighbor's IP address in that network (or indicate if none exists)
struct neighbor_connection {
    std::array<uint8_t, 16> neighbor_id;
    uint8_t  mac_addr[6];
    std::string local_ifname;
    std::time_t last_seen;
    uint8_t  ip_family;       // AF_INET / AF_INET6 / 0
    union {
        struct in_addr  ipv4;
        struct in6_addr ipv6;
    };
};



class NeighborManager {
    public:
        NeighborManager();
        int create_ethernet_socket();
        int create_broadcast_recv_socket(int ifindex);
        void recv_broadcast(int rcv_sockfd);
        void send_ethernet_msg(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> des_mac,struct neighbor_payload payload);
        void recv_ethernet_msg();
    private:
        int eth_socket;
        std::unordered_map<std::string,struct neighbor_connection> active_neighbors;
        std::array<uint8_t, 16> client_id;
        std::array<uint8_t, 16> get_random_client_id();
        const std::array<uint8_t, 6> broadcast_mac = {0xFF,0xFF,0xFF,0xFF,0xFF};
        struct ethhdr init_ethhdr(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> dest_mac);
        struct sockaddr_ll init_sockaddr_ll(int ifindex, std::array<uint8_t, 6> dest_mac);

};
#endif //NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H