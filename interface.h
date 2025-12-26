#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#include <string>

struct ip_address {

};

struct ethernet_interface {
    int ifindex;
    const char* ifname;
    const uint8_t* mac_addr;

    bool is_active;
};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_H