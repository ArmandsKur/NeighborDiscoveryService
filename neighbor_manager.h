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

#include <array>
#include <unistd.h>
#include "interface.h"

struct neighbor {
    std::array<uint8_t, 16> neighbor_id;
};



class NeighborManager {
    public:
        NeighborManager();
        int create_ethernet_socket();
        void send_ethernet_msg(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> des_mac);
    private:
        std::array<uint8_t, 16> client_id;
        std::array<uint8_t, 16> get_random_client_id();
        const std::array<uint8_t, 6> broadcast_mac = {0xFF,0xFF,0xFF,0xFF,0xFF};
        struct ethhdr init_ethhdr(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> dest_mac);

};
#endif //NEIGHBORDISCOVERYSERVICE_NEIGHBOR_MANAGER_H