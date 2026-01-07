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

#include "neighbor_discovery/event_poll.h"
#include "neighbor_discovery/interface_manager.h"
#include "neighbor_discovery/neighbor_manager.h"


/*
 * NeighborDiscovery service must be ran with root privileges
 * otherwise won't be able to create raw sockets
 */

int main() {
    //clock_gettime(CLOCK_MONOTONIC,);
    std::cout<<"main"<<std::endl;
    int work = 0;

    EventPoll event_poll;
    //event_poll.startup_netlink();
    event_poll.startup();
    //event_poll.startup_neighbor_manager();
    event_poll.run_event_poll();

    /*
    NeighborManager neighbor_mngr;
    std::array<uint8_t, 6> source_mac = {0xce,0x97,0xe5,0xd8,0x48,0x26};
    std::array<uint8_t, 6> dest_mac = {0xff,0xff,0xff,0xff,0xff,0xff};
    struct neighbor_payload test_payload{};
    test_payload.ip_family = 0;
    memcpy(test_payload.mac_addr, source_mac.data(), ETH_ALEN);
    std::array<uint8_t, 16>  test_payload_client_id= {0xe8,0x06,0xfd,0x84,0xa9,0xb0,0xe8,0x06,0xfd,0x84,0xa9,0xb0,0xe8,0x06,0xfd,0x84};
    memcpy(test_payload.client_id, test_payload_client_id.data(), 16);



    std::cout<<"Sending msg"<<std::endl;
    neighbor_mngr.send_ethernet_msg(source_mac,dest_mac,test_payload);

    std::cout<<"Recv msg"<<std::endl;
    neighbor_mngr.recv_ethernet_msg();

    */
    return 0;
}
