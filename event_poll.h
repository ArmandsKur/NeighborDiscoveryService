#ifndef NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H
#define NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H
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

#include "interface_manager.h"
#include "neighbor_manager.h"

class EventPoll {
    public:
        void add_to_pdfds(int new_fd);
        void del_from_pfds(int fd);
        void startup_netlink();
        void run_event_poll();

        //for some time made public
        /*
        std::vector<struct pollfd> pfds;
        nfds_t fd_count = 0;
        int netlink_fd;
        InterfaceManager if_mngr;
        */

    private:
        //Variables related to storing fds used in the poll
        std::vector<struct pollfd> pfds;
        nfds_t fd_count = 0;
        //Variables related to netlink
        InterfaceManager if_mngr;
        int netlink_fd;
        //Variables related to neighour management
        NeighborManager neighbor_mngr;

};
#endif //NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H