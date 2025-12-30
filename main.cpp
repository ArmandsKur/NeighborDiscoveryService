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

#include "event_poll.h"
#include "interface_manager.h"
#include "neighbor_manager.h"


/*
 * NeighborDiscovery service must be ran as superuser, in other creation of
 */
int main() {

    NeighborManager neighbor_mngr;
    std::array<uint8_t, 6> source_mac = {0xce,0x97,0xe5,0xd8,0x48,0x26};
    std::array<uint8_t, 6> dest_mac = {0xff,0xff,0xff,0xff,0xff,0xff};
    neighbor_mngr.send_ethernet_msg(source_mac,dest_mac);
    /*
    EventPoll event_poll;
    event_poll.startup_netlink();
    event_poll.run_event_poll();
    */
    return 0;
}
