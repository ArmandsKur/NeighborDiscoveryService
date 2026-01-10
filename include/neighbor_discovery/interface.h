#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

#include <array>

struct ip_address {
    sa_family_t family = 0;

    union {
        in_addr ipv4;
        in6_addr ipv6;
    };
};

struct ethernet_interface {
    int ifindex;
    std::string ifname;
    std::array<uint8_t, 6> mac_addr;
    std::vector<ip_address> ip_addresses;
    bool is_active;
    bool is_loopback;
};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_H