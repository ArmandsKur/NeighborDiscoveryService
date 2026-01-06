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

#include "client_manager.h"

#define NAME "NeighborDiscoveryService"

class ClientManager {
    public:
        int open_listen_socket();
        int open_data_socket();
    private:
        int listen_socket;
        int data_socket;
        sockaddr_un init_sockaddr_un();
        void socket_set_nonblock(int sock_fd);
};
#endif //NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H