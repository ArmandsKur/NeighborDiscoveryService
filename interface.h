#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#include <string>
#include <vector>

struct ip_address {
    sa_family_t family;

    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    };
};

struct ethernet_interface {
    int ifindex;
    const char* ifname;
    const uint8_t* mac_addr;
    std::vector<ip_address> ip_addresses;
    bool is_active;
};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_H