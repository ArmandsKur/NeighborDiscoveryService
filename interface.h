#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_H
#include <string>

struct ethernet_interface {
    int ifindex;
    int is_usable;
    const char* ifname;
    const uint8_t* mac_addr;
};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_H