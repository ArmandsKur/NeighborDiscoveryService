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
    if_mngr.socket_set_nonblock(netlink_fd);
}

void EventPoll::run_event_poll() {
    int ready;

    while (fd_count > 0) {
        ready = poll(pfds.data(), fd_count, -1);
        if (ready == -1)
            perror("poll");

        for (const pollfd& pfd : pfds) {
            if (pfd.revents == 0) {
                continue;
            }
            if (pfd.revents & POLLIN) {
                char buffer[8192];
                struct iovec iov = {buffer,sizeof(buffer)};
                struct sockaddr_nl sa;
                struct msghdr msg = {&sa,sizeof(sa),&iov,1};

                ssize_t len = recvmsg(pfd.fd, &msg, 0);
                //Stop reading if error on recvmsg
                if (len < 0) {
                    perror("recvmsg");
                    break;
                }

                //Handle only kernel messages
                if (sa.nl_pid != 0)
                    continue;

                for (struct nlmsghdr* nlh = (struct nlmsghdr*)buffer;NLMSG_OK(nlh, len);nlh = NLMSG_NEXT(nlh, len)) {
                    if (nlh->nlmsg_type == NLMSG_DONE)
                        break;

                    if (nlh->nlmsg_type == NLMSG_ERROR) {
                        std::cerr << "Netlink error\n";
                        continue;
                    }

                    if (nlh->nlmsg_seq != 0)
                        continue;

                    switch (nlh->nlmsg_type) {
                        case RTM_NEWLINK:
                            std::cout << "RTM_NEWLINK" << std::endl;
                            if_mngr.handle_newlink(nlh);
                            break;
                        case RTM_DELLINK:
                            std::cout << "RTM_DELLINK" << std::endl;
                            if_mngr.handle_dellink(nlh);
                            break;
                        case RTM_NEWADDR:
                            std::cout << "RTM_NEWADDR" << std::endl;
                            if_mngr.handle_newaddr(nlh);
                            break;
                        case RTM_DELADDR:
                            std::cout << "RTM_DELADDR" << std::endl;
                            if_mngr.handle_deladdr(nlh);
                            break;
                    }
                }
            }
        }
    }
}