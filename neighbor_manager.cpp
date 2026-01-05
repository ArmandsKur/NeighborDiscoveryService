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
#include <ctime>
#include <array>
#include <unistd.h>
#include <errno.h>

#include "neighbor_manager.h"
#include <bitset>

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

void NeighborManager::socket_set_nonblock(int sock_fd) {
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
    }
    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
    }
}

//Function used to create a socket for broadcast recieval and bind it to specific interface
int NeighborManager::create_broadcast_recv_socket() {

    /*struct sockaddr_ll ll{};
    ll.sll_family = AF_PACKET;
    ll.sll_halen = ETH_ALEN;
    ll.sll_ifindex = ifindex;*/

    recv_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));
    socket_set_nonblock(recv_sockfd);
    /*
    //Close the socket and return -1 if unable to bind.
    if (bind(rcv_sockfd, (struct sockaddr *)&ll, sizeof(ll)) == -1) {
        perror("bind");
        close(rcv_sockfd);
        return -1;
    }
    */

    return recv_sockfd;

}

int NeighborManager::create_broadcast_send_socket() {
    send_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));
    socket_set_nonblock(send_sockfd);
    return send_sockfd;
}

neighbor_payload NeighborManager::construct_neighbor_payload(std::array<uint8_t,6>  source_mac,ip_address ip_addr) {
    neighbor_payload payload{};
    memcpy(payload.client_id, client_id.data(), client_id.size());
    memcpy(payload.mac_addr, source_mac.data(), ETH_ALEN);
    //Fill IP address in case if there is any
    payload.ip_family = ip_addr.family;
    if (payload.ip_family == AF_INET) {
        payload.ipv4 = ip_addr.ipv4;
    } else if (payload.ip_family == AF_INET6) {
        payload.ipv6 = ip_addr.ipv6;
    }

    return payload;
}

//Function used to send message using raw socket
void NeighborManager::send_broadcast(int ifindex, std::array<uint8_t, 6> source_mac, struct neighbor_payload payload) {
    uint8_t buf[128];
    struct ethhdr eh = init_ethhdr(source_mac,broadcast_mac);
    struct sockaddr_ll saddr_ll = init_sockaddr_ll(ifindex,broadcast_mac);

    //construct a packet
    memcpy(buf, &eh, sizeof(eh));
    memcpy(buf + sizeof(eh),&payload,sizeof(payload));
    const int frame_size = sizeof(eh) + sizeof(payload);
    /*
    std::cout<<"ifindex: "<<ifindex<<std::endl;

    printf("broadcast_mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
             broadcast_mac[0], broadcast_mac[1], broadcast_mac[2],
             broadcast_mac[3], broadcast_mac[4], broadcast_mac[5]);

    printf("source_mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
             source_mac[0], source_mac[1], source_mac[2],
             source_mac[3], source_mac[4], source_mac[5]);

    printf("payload mac_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
             payload.mac_addr[0], payload.mac_addr[1], payload.mac_addr[2],
             payload.mac_addr[3], payload.mac_addr[4], payload.mac_addr[5]);

    std::cout<<"ip_address "<<inet_ntoa(payload.ipv4)<<std::endl;
    */
    //send msg
    const ssize_t n = sendto(send_sockfd, buf, frame_size, 0, (struct sockaddr *)&saddr_ll, sizeof(saddr_ll));
    if (n == -1) {
        //In case if kernel buffer is full
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cerr<<"Kernel buffer full\n";
            return;
        }
        perror("sendto");
    }
    if (n != frame_size) {
        std::cerr<<"Neighbor frame not sent completely\n";
    }
}

void NeighborManager::recv_broadcast() {
    ssize_t len;
    uint8_t buf[P_NEIGHBOR_SIZE];
    struct sockaddr_ll ll{};
    socklen_t sockaddr_len = sizeof(ll);

    std::chrono::time_point<std::chrono::steady_clock> rcv_time = std::chrono::steady_clock::now();
    len = recvfrom(recv_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&ll, &sockaddr_len);
    if (len < 0) {
        //In case if nothing to recieve don't throw errors
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("recv");
        return;
    }

    //Get interface name from sockaddr received in recvfrom
    int ifindex = ll.sll_ifindex;
    char ifname[IFNAMSIZ];
    if (!if_indextoname(ifindex, ifname)) {
        perror("if_indextoname");
        return;
    }

    //Check that correct protocol packet was received
    const ethhdr* eth = (ethhdr*)buf;
    if(eth->h_proto != htons(ETH_P_NEIGHBOR)) {
        std::cerr << "received incorrect protocol\n";
        return;
    }

    //Check that the payload is of correct size for neighbor_payload struct
    const uint8_t* payload = buf + sizeof(struct ethhdr);
    size_t payload_len = len - sizeof(struct ethhdr);
    if (payload_len < sizeof(neighbor_payload)) {
        std::cerr << "payload too short\n";
        return;
    }

    //copy the neighbor payload recieved to the neighbor_payload struct variable
    neighbor_payload neighbor_pyld;
    std::memcpy(&neighbor_pyld, payload, sizeof(neighbor_pyld));

    //Create neighbor_connection struct and fill it with information
    neighbor_connection neigh_conn{};
    std::memcpy(&neigh_conn.neighbor_id,neighbor_pyld.client_id,sizeof(neighbor_pyld.client_id));
    std::memcpy(&neigh_conn.mac_addr,neighbor_pyld.mac_addr,sizeof(neighbor_pyld.mac_addr));
    neigh_conn.local_ifname = ifname;
    neigh_conn.last_seen = rcv_time;
    if (neighbor_pyld.ip_family == AF_INET) {
        neigh_conn.ip_family = AF_INET;
        neigh_conn.ipv4 = neighbor_pyld.ipv4;
    } else if (neighbor_pyld.ip_family == AF_INET6) {
        neigh_conn.ip_family = AF_INET6;
        neigh_conn.ipv6 = neighbor_pyld.ipv6;
    } else {
        neigh_conn.ip_family = 0;
    }

    //store the connection
    active_neighbors[neigh_conn.neighbor_id].active_connections[ifindex] = neigh_conn;
    //active_neighbors[ifname] = neigh_conn;

    std::cout<<"neigh_conn.local_ifname: "<<neigh_conn.local_ifname<<std::endl;

    printf("neighbor_pyld.client_id: %02x:%02x:%02x:%02x:%02x:%02x\n",
             neighbor_pyld.client_id[0], neighbor_pyld.client_id[1], neighbor_pyld.client_id[2],
             neighbor_pyld.client_id[3], neighbor_pyld.client_id[4], neighbor_pyld.client_id[5]);

    printf("neighbor_pyld.mac_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
             neighbor_pyld.mac_addr[0], neighbor_pyld.mac_addr[1], neighbor_pyld.mac_addr[2],
             neighbor_pyld.mac_addr[3], neighbor_pyld.mac_addr[4], neighbor_pyld.mac_addr[5]);

    if (neigh_conn.ip_family == AF_INET) {
        std::cout<<"ip_address_v4 "<<inet_ntoa(neigh_conn.ipv4)<<std::endl;
    } else if (neigh_conn.ip_family == AF_INET6) {
        printf("ip_address_v6: %02x:%02x:%02x:%02x:%02x:%02x\n",
             neigh_conn.ipv6.s6_addr[0], neigh_conn.ipv6.s6_addr[1], neigh_conn.ipv6.s6_addr[2],
             neigh_conn.ipv6.s6_addr[3], neigh_conn.ipv6.s6_addr[4], neigh_conn.ipv6.s6_addr[5]);
    }


}
/*
void NeighborManager::recv_ethernet_msg() {
    int len;
    int rcv_sock, ret;
    uint8_t buf[P_NEIGHBOR_SIZE];

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
    std::memcpy(&pyld, payload, sizeof(pyld));


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
*/