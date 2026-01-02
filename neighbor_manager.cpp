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
#include <bitset>
NeighborManager::NeighborManager() {
    client_id = get_random_client_id();
    create_ethernet_socket();
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
    eth_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));
    //eth_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    return eth_socket;
}

//Function used to return initialized ethhdr struct
struct ethhdr NeighborManager::init_ethhdr(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> dest_mac) {
    struct ethhdr eh{};
    std::memcpy(eh.h_source, source_mac.data(), ETH_ALEN);
    std::memcpy(eh.h_dest, dest_mac.data(), ETH_ALEN);
    eh.h_proto = htons(ETH_P_NEIGHBOR); //For this project use only one Protocol ID
    return eh;
}
//Function used to initialize sockaddr_ll struct.
struct sockaddr_ll NeighborManager::init_sockaddr_ll(int ifindex, std::array<uint8_t, 6> dest_mac) {
    sockaddr_ll saddr_ll{};
    saddr_ll.sll_family   = AF_PACKET;
    saddr_ll.sll_ifindex = ifindex;
    saddr_ll.sll_halen = ETH_ALEN;
    saddr_ll.sll_protocol = htons(ETH_P_NEIGHBOR);
    memcpy(saddr_ll.sll_addr, dest_mac.data(), ETH_ALEN);
    return saddr_ll;
}

//Function used to send message using raw socket
void NeighborManager::send_ethernet_msg(std::array<uint8_t, 6> source_mac,std::array<uint8_t, 6> dest_mac,struct neighbor_payload payload) {
    struct ethhdr eh = init_ethhdr(source_mac,dest_mac);
    uint8_t buf[128];

    int ifindex = 2;//for now hardcoded

    struct sockaddr_ll saddr_ll = {};
    saddr_ll.sll_family   = AF_PACKET;
    saddr_ll.sll_ifindex = ifindex;
    saddr_ll.sll_halen = ETH_ALEN;
    saddr_ll.sll_protocol = htons(ETH_P_NEIGHBOR);
    memcpy(saddr_ll.sll_addr, dest_mac.data(), ETH_ALEN);

    int s = create_ethernet_socket();

    /* construct a packet */
    memcpy(buf, &eh, sizeof(eh));
    memcpy(buf + sizeof(eh),&payload,sizeof(payload));

    int errno;
    int frame_size = sizeof(eh) + sizeof(payload);
    int n = sendto(s, buf, frame_size, 0, (struct sockaddr *)&saddr_ll, sizeof(saddr_ll));



    std::cout << "Sendto result " << n << std::endl;
    std::cout << "buf size" << sizeof(buf) << std::endl;
    std::cout<< "payload size" << sizeof(payload) << std::endl;
    std::cout << "socket " << s << std::endl;
    std::cout << "errno " << errno << std::endl;
}

int NeighborManager::create_broadcast_recv_socket(int ifindex) {
    int rcv_sockfd, ret;

    struct sockaddr_ll ll{};
    ll.sll_family = AF_PACKET;
    ll.sll_halen = ETH_ALEN;
    ll.sll_ifindex = ifindex;

    rcv_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));

    //Close the socket and return -1 if unable to bind.
    if (bind(rcv_sockfd, (struct sockaddr *) &ll, sizeof(ll)) == -1) {
        perror("bind");
        close(rcv_sockfd);
        return -1;
    }

    return rcv_sockfd;

}

void NeighborManager::recv_broadcast(int rcv_sockfd) {
    ssize_t len;
    uint8_t buf[128];

    len = recvfrom(rcv_sockfd, buf, sizeof(buf), 0, NULL, NULL);
    if (len < 0) {
        perror("recv");
    }
}

void NeighborManager::recv_ethernet_msg() {
    int len;
    int rcv_sock, ret;
    //uint8_t buf[128];
    uint8_t buf[53];

    struct sockaddr_ll ll{};
    ll.sll_family = AF_PACKET;
    ll.sll_halen = ETH_ALEN;
    ll.sll_ifindex = 2;

    rcv_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));
    std::cout << "recv ethernet msg socket" << rcv_sock << std::endl;

    ret = bind(rcv_sock, (struct sockaddr *) &ll, sizeof(ll));
    if (ret == -1) {
        perror("bind");
        exit(1);
    }

    while(1) {
        len = recvfrom(rcv_sock, buf, sizeof(buf), 0, NULL, NULL);
        if (len < 0)
        {
            perror("recv");
        }
        printf("received bytes = %d\n", len);
        break;


    }

    std::cout<<"Bits: "<<std::endl;
    for (auto b : buf) {
        std::cout << std::bitset<8>(b) << ' ';
    }
    std::cout << '\n';

    const ethhdr* eth = (ethhdr*)buf;
    if(eth->h_proto == htons(ETH_P_NEIGHBOR)) {
        std::cout << "received correct protocol" << std::endl;
    } else {
        std::cout << "received incorrect protocol" << std::endl;
    }

    const uint8_t* payload = buf + sizeof(struct ethhdr);
    size_t payload_len = len - sizeof(struct ethhdr);

    std::cout << "payload len: " << payload_len << std::endl;
    if (payload_len < sizeof(neighbor_payload)) {
        std::cerr << "payload too short\n";
        return;
    }
    std::cout << "proto raw: 0x"<< std::hex << ntohs(eth->h_proto) << std::dec << "\n";

    neighbor_payload pyld;
    std::memcpy(&pyld, buf + sizeof(ethhdr), sizeof(pyld));


    //const neighbor_payload* pyld = (neighbor_payload*)payload;
    std::cout << "pyld raw: 0x"<< std::hex << pyld.client_id << std::dec << "\n";
    std::cout << "pyld client_id[0]: " << pyld.client_id[0] << std::endl;

    std::cout << "client_id: ";
    for (int i = 0; i < 16; ++i) {
        printf("%02x", pyld.client_id[i]);
    }
    std::cout << "\n";

    std::cout << "recieved client_id" << std::endl;
    printf("mac_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
             pyld.client_id[0], pyld.client_id[1], pyld.client_id[2],
             pyld.client_id[3], pyld.client_id[4], pyld.client_id[5]);

    std::cout << "recieved mac_addr" << std::endl;
    printf("mac_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
             pyld.mac_addr[0], pyld.mac_addr[1], pyld.mac_addr[2],
             pyld.mac_addr[3], pyld.mac_addr[4], pyld.mac_addr[5]);

    printf("ip_family: %d\n", pyld.ip_family);
    //std::cout << "recieved ip_family " <<pyld.ip_family << std::endl;
}