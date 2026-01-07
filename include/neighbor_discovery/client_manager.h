#ifndef NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H
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
#include <sys/un.h>

#include "neighbor_discovery/neighbor_manager.h"

#define NAME "/tmp/NeighborDiscoveryService.sock"

class ClientManager {
    public:
        bool init();
        int get_listen_socket();
        int open_data_socket();
        bool get_conn_status();
        void write_message(neighbor_payload payload);
        void end_message();
        void close_data_socket();
    private:
        int listen_socket;
        int data_socket;
        bool connected = false;
        sockaddr_un init_sockaddr_un();
        int open_listen_socket();
        void socket_set_nonblock(int sock_fd);
};
#endif //NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H