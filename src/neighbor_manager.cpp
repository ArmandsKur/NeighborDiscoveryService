#include "neighbor_discovery/neighbor_manager.h"

bool NeighborManager::init() {
    client_id = get_random_client_id();
    if (create_broadcast_recv_socket() == -1) {
        std::cerr << "Failed to create broadcast recv socket" << std::endl;
        return false;
    }
    if (create_broadcast_send_socket() == -1) {
        std::cerr << "Failed to create broadcast send socket" << std::endl;
        return false;
    }
    return true;
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
    getentropy(id.data(), length);

    return id;
}

//Function used to return initialized ethhdr struct
ethhdr NeighborManager::init_ethhdr(std::array<uint8_t,6> source_mac, std::array<uint8_t,6> dest_mac) {
    struct ethhdr eh{};
    std::memcpy(eh.h_source, source_mac.data(), ETH_ALEN);
    std::memcpy(eh.h_dest, dest_mac.data(), ETH_ALEN);
    eh.h_proto = htons(ETH_P_NEIGHBOR); //For this project use only one Protocol ID
    return eh;
}
//Function used to initialize sockaddr_ll struct.
sockaddr_ll NeighborManager::init_sockaddr_ll(int ifindex, std::array<uint8_t,6> dest_mac) {
    sockaddr_ll saddr_ll{};
    saddr_ll.sll_family   = AF_PACKET;
    saddr_ll.sll_ifindex = ifindex;
    saddr_ll.sll_halen = ETH_ALEN;
    saddr_ll.sll_protocol = htons(ETH_P_NEIGHBOR);
    memcpy(saddr_ll.sll_addr, dest_mac.data(), ETH_ALEN);
    return saddr_ll;
}

bool NeighborManager::socket_set_nonblock(int sock_fd) {
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return false;
    }
    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return false;
    }
    return true;
}

//Function used to create a socket for broadcast receival
int NeighborManager::create_broadcast_recv_socket() {
    recv_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));
    if (recv_sockfd == -1) {
        perror("create_broadcast_recv_socket");
        return -1;
    }
    if (!socket_set_nonblock(recv_sockfd)) {
        perror("set nonblock recv socket");
        close(recv_sockfd);
        return -1;
    }
    return recv_sockfd;
}

int NeighborManager::create_broadcast_send_socket() {
    send_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_NEIGHBOR));
    if (send_sockfd == -1) {
        perror("create_broadcast_send_socket");
        return -1;
    }
    if (!socket_set_nonblock(send_sockfd)) {
        perror("set nonblock send socket");
        close(send_sockfd);
        return -1;
    }
    return send_sockfd;
}

neighbor_payload NeighborManager::construct_neighbor_payload(std::array<uint8_t,6> source_mac, ip_address ip_addr) {
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
void NeighborManager::send_broadcast(int ifindex, std::array<uint8_t,6> source_mac, neighbor_payload payload) {
    uint8_t buf[sizeof(ethhdr)+sizeof(neighbor_payload)];
    struct ethhdr eh = init_ethhdr(source_mac,broadcast_mac);
    struct sockaddr_ll saddr_ll = init_sockaddr_ll(ifindex,broadcast_mac);

    //construct a packet
    memcpy(buf, &eh, sizeof(eh));
    memcpy(buf + sizeof(eh),&payload,sizeof(payload));
    const int frame_size = sizeof(eh) + sizeof(payload);

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

void NeighborManager::recv_broadcast(const std::unordered_map<int,ethernet_interface> &interface_list) {
    ssize_t len;
    uint8_t buf[sizeof(ethhdr)+sizeof(neighbor_payload)];
    struct sockaddr_ll ll{};
    socklen_t sockaddr_len = sizeof(ll);

    std::chrono::time_point<std::chrono::steady_clock> rcv_time = std::chrono::steady_clock::now();
    len = recvfrom(recv_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&ll, &sockaddr_len);
    if (len < 0) {
        //In case if nothing to receive don't throw errors
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

    /*
     * Check if interface to which broadcast is received is active
     * If not then ignore the payload.
     * Packets which get queued in the virtual switch during the downtime of interface
     * have to be ignored.
     */
    auto it = interface_list.find(ifindex);
    if (it == interface_list.end()) {
        std::cerr<<"Interface not found\n";
        return;
    } else {
        if (!it->second.is_active)
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

    //copy the neighbor payload received to the neighbor_payload struct variable
    neighbor_payload neighbor_pyld;
    std::memcpy(&neighbor_pyld, payload, sizeof(neighbor_pyld));

    //Create neighbor_connection struct and fill it with information
    neighbor_connection neigh_conn{};
    std::memcpy(&neigh_conn.neighbor_id, neighbor_pyld.client_id,sizeof(neighbor_pyld.client_id));
    std::memcpy(&neigh_conn.mac_addr, neighbor_pyld.mac_addr,sizeof(neighbor_pyld.mac_addr));
    std::memcpy(&neigh_conn.local_ifname, ifname,IFNAMSIZ);
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
    neighbors[neigh_conn.neighbor_id].active_connections[ifindex] = neigh_conn;

    //Print received neighbor info
    printf("neigh_conn.local_ifname: %s\n",neigh_conn.local_ifname);
    printf("neighbor_pyld.client_id: %02x:%02x:%02x:%02x:%02x:%02x\n",
             neighbor_pyld.client_id[0], neighbor_pyld.client_id[1], neighbor_pyld.client_id[2],
             neighbor_pyld.client_id[3], neighbor_pyld.client_id[4], neighbor_pyld.client_id[5]);

    printf("neighbor_pyld.mac_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
             neighbor_pyld.mac_addr[0], neighbor_pyld.mac_addr[1], neighbor_pyld.mac_addr[2],
             neighbor_pyld.mac_addr[3], neighbor_pyld.mac_addr[4], neighbor_pyld.mac_addr[5]);


    char ip[neigh_conn.ip_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
    if (neigh_conn.ip_family == AF_INET) {
        inet_ntop(AF_INET,&neigh_conn.ipv4,ip,sizeof(ip));
        printf("ip_address: %s\n",ip);
    } else if (neigh_conn.ip_family == AF_INET6) {
        inet_ntop(AF_INET6,&neigh_conn.ipv6,ip,sizeof(ip));
        printf("ip_address: %s\n",ip);
    }
}

cli_neighbor_payload NeighborManager::construct_cli_neighbor_payload(neighbor_connection conn) {
    cli_neighbor_payload payload{};
    memcpy(payload.local_ifname, conn.local_ifname, IFNAMSIZ);
    memcpy(payload.client_id, conn.neighbor_id.data(), conn.neighbor_id.size());
    memcpy(payload.mac_addr, conn.mac_addr.data(), ETH_ALEN);
    //Fill IP address in case if there is any
    payload.ip_family = conn.ip_family;
    if (payload.ip_family == AF_INET) {
        payload.ipv4 = conn.ipv4;
    } else if (payload.ip_family == AF_INET6) {
        payload.ipv6 = conn.ipv6;
    }

    return payload;
}

int NeighborManager::get_broadcast_recv_socket() {
    return recv_sockfd;
}
int NeighborManager::get_broadcast_send_socket() {
    return send_sockfd;
}

const std::map<std::array<uint8_t, 16>, active_neighbor>& NeighborManager::get_active_neighbors() {
    return neighbors;
}

//Erase inactive connections
void NeighborManager::remove_inactive_connections() {
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();

    for (auto neighbor_it = neighbors.begin(); neighbor_it != neighbors.end(); ) {
        //Get reference to neighbor_connection map
        auto& active_neigh_conn = neighbor_it->second.active_connections;
        //Delete connections not seen in last 30 seconds
        for (auto conn_it = active_neigh_conn.begin(); conn_it != active_neigh_conn.end(); ) {
            if (now - conn_it->second.last_seen > std::chrono::seconds(30)) {
                conn_it = active_neigh_conn.erase(conn_it);
            } else {
                ++conn_it;
            }
        }
        //If after connection deletion map becomes empty, neighbor has to be deleted
        if (active_neigh_conn.empty()) {
            neighbor_it = neighbors.erase(neighbor_it);
        } else {
            ++neighbor_it;
        }
    }
}


void NeighborManager::cleanup() {
    if (send_sockfd != -1) {
        close(send_sockfd);
        send_sockfd = -1;
    }
    if (recv_sockfd != -1) {
        close(recv_sockfd);
        recv_sockfd = -1;
    }
}

