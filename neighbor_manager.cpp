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

#include "neighbor_manager.h"
#include "interface.h"
#include <errno.h>
NeighborManager::NeighborManager() {
    client_id = get_random_client_id();
}


/*
 * I chose to use getentropy because:
 *  1)its "really" random compared to rand for example;
 *  2)does not need any additional library installments;
 *  3)using some system variables created upon install also does not work as the VMs were copied;
 *
 *  Currently each new run of the service new random number is generated, so each time
 *  when the service is restarted neighbor will be treated like a new one.
 *
 *  16 byte random number is selected as for 10K active neighbours its nearly impossible
 *  to get two same numbers.
 */
std::array<uint8_t, 16> NeighborManager::get_random_client_id() {
    const size_t length = 16;
    std::array<uint8_t, 16> id{};
    getentropy(client_id.data(), length);

    char buf[18];
    snprintf(buf, sizeof(buf),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             client_id[0], client_id[1], client_id[2],
             client_id[3], client_id[4], client_id[5]);
    std::cout << "Client id: " << buf << std::endl;
    return client_id;
}

int NeighborManager::create_ethernet_socket() {
    int eth_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    return eth_socket;
}

//Function used to return initialized ethhdr struct
struct ethhdr NeighborManager::init_ethhdr(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> dest_mac) {
    struct ethhdr eh{};
    std::memcpy(eh.h_source, source_mac.data(), ETH_ALEN);
    std::memcpy(eh.h_dest, dest_mac.data(), ETH_ALEN);
    eh.h_proto = htons(ETH_P_IP); //For this project use only one Protocol ID
    return eh;
}

//Function used to send message using raw socket
void NeighborManager::send_ethernet_msg(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> dest_mac) {
    struct ethhdr eh = init_ethhdr(source_mac,dest_mac);
    /*struct ethhdr eh = {
        .h_source = {source_mac[0], source_mac[1], source_mac[2], source_mac[3],source_mac[4],source_mac[5]},
        .h_dest = {dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]},
        .h_proto = htons(ETH_P_IP),
    };*/

    struct iphdr iph = {};
    struct udphdr uh = {};
    uint8_t buf[128];

    int s = create_ethernet_socket();

    int ifindex = 1;//for now hardcoded

    iph.ihl = 5;
    iph.version = 4;
    iph.tos = 16;
    iph.id = 1;
    iph.ttl = 64;
    iph.protocol = IPPROTO_UDP;
    iph.saddr = 1;
    iph.daddr = 2;
    iph.tot_len = htons(sizeof(buf) - ETH_HLEN);
    iph.check = 0;

    struct sockaddr_ll saddr_ll = {
        .sll_ifindex = ifindex,
        .sll_halen = ETH_ALEN,
        .sll_addr = {dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]},
    };

    /* construct a packet */
    memcpy(buf, &eh, sizeof(eh));
    memcpy(buf + sizeof(eh), &iph, sizeof(iph));
    memcpy(buf + sizeof(eh) + sizeof(iph), &uh, sizeof(uh));

    int errno;
    int n = sendto(s, buf, sizeof(buf), 0, (struct sockaddr *)&saddr_ll, sizeof(saddr_ll));

    std::cout << "Sendto result " << n << std::endl;
    std::cout << "buf size" << sizeof(buf) << std::endl;
    std::cout << "socket " << s << std::endl;
    std::cout << "errno " << errno << std::endl;
}

void NeighborManager::recv_ethernet_msg() {
    int len;
    len = recv(netlink_fd, &buffer, sizeof(buffer), 0);
}