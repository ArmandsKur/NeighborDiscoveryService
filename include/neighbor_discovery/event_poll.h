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

#include "neighbor_discovery/neighbor_manager.h"
#include "neighbor_discovery/interface_manager.h"
#include "neighbor_discovery/client_manager.h"

enum class PollFdRole {
    Netlink,
    PacketRecv,
    PacketSend,
    UnixListen
};

class EventPoll{
    public:
        bool startup();
        void startup_netlink();
        void startup_neighbor_manager();
        void run_event_poll();

        void add_to_pfds(int new_fd, short events, PollFdRole role);
        void del_from_pfds(int fd);


    private:
        //Variables related to storing fds used in the poll
        std::vector<pollfd> pfds;
        std::unordered_map<int,PollFdRole> pfd_role;
        nfds_t fd_count = 0;

        //Manager classes
        NeighborManager neighbor_mngr;
        InterfaceManager if_mngr;
        ClientManager client_mngr;



};
#endif //NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H