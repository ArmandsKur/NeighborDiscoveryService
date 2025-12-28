#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_H
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
#include <vector>

struct ip_address {
    sa_family_t family;

    union {
        in_addr ipv4;
        in6_addr ipv6;
    };
};

struct ethernet_interface {
    int ifindex;
    //const char* ifname;
    std::string ifname;
    //const uint8_t* mac_addr;
    std::array<uint8_t, 6> mac_addr;
    std::vector<ip_address> ip_addresses;
    bool is_active;
};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_H