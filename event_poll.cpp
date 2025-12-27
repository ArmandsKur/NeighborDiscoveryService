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

#include "event_poll.h"

void EventPoll::add_to_pdfds(int new_fd) {
    pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    new_pollfd.events = POLLIN;
    new_pollfd.revents = 0;

    pfds.push_back(new_pollfd);

    fd_count = pfds.size();
}

void EventPoll::del_from_pfds(int fd) {
    pfds.erase(
        std::remove_if(
            pfds.begin(),
            pfds.end(),
            [fd](const pollfd &pfd) {
                return pfd.fd == fd;
            }
        ),
        pfds.end()
    );
    fd_count = pfds.size();
}

void EventPoll::startup_netlink() {
    netlink_fd = if_mngr.open_netlink_socket();
    add_to_pdfds(netlink_fd);
    if_mngr.do_getlink_dump(netlink_fd);
    if_mngr.do_getaddr_dump(netlink_fd);
}