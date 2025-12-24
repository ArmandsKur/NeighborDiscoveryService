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

//For each connection, display(in this context store) the local interface name, neighbor's MAC address in that specific network, and neighbor's IP address in that network (or indicate if none exists)
/*struct Connection {

};*/

/*struct poll_fds {
    pollfd pfd

};*/

void add_to_pdfds(std::vector<pollfd> *pfds,int new_fd, nfds_t *fd_count) {
    pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    new_pollfd.events = POLLIN;
    new_pollfd.revents = 0;

    pfds->push_back(new_pollfd);

    (*fd_count) = pfds->size();
}

void del_from_pfds(std::vector<pollfd> *pfds, int fd, nfds_t *fd_count) {
    pfds->erase(
        std::remove_if(
            pfds->begin(),
            pfds->end(),
            [fd](const pollfd &pfd) {
                return pfd.fd == fd;
            }
        ),
        pfds->end()
    );
    (*fd_count) = pfds->size();
}

//void recieve_netlink_info()

int main() {
    int ready;
    struct sockaddr_nl sa;

    std::vector<pollfd> pfds;
    nfds_t fd_count = 0;

    int netlink_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    fcntl(netlink_fd, F_SETFL, O_NONBLOCK);
    if (netlink_fd != -1) {
        memset(&sa, 0, sizeof(sa));
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = RTMGRP_LINK |
                       RTMGRP_IPV4_IFADDR |
                       RTMGRP_IPV6_IFADDR;
        sa.nl_groups = RTMGRP_LINK;
        bind(netlink_fd, (struct sockaddr*)&sa, sizeof(sa));
    } else {
        perror("socket");
    }
    add_to_pdfds(&pfds,netlink_fd,&fd_count);

    std::cout << "fd_count: " << fd_count << std::endl;

    int size;
    char buffer[4096];
    struct iovec iov = { buffer, sizeof(buffer) };
    struct msghdr msg;
    struct nlmsghdr *nlh;

    nlh = (struct nlmsghdr*)buffer;
    while (fd_count > 0) {
        ready = poll(pfds.data(), fd_count, -1);
        if (ready == -1)
            perror("poll");
        printf("Ready: %d\n", ready);

        for (const pollfd& pfd : pfds) {
            if (pfd.revents != 0) {
                if (pfd.revents & POLLIN) {
                    std::cout << "Read" << std::endl;

                    while ((size = recv(pfd.fd, nlh, 4096, 0)) > 0) {
                        std::cout << "size: " << size << std::endl;
                        while ((NLMSG_OK(nlh, size)) && (nlh->nlmsg_type != NLMSG_DONE)) {
                            if (nlh->nlmsg_type == RTM_NEWLINK) {
                                std::cout << "RTM_NEWLINK" << std::endl;
                                struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
                                struct rtattr *rth = IFLA_RTA(ifi);
                                int rtl = IFLA_PAYLOAD(nlh);


                                while (rtl && RTA_OK(rth, rtl)) {
                                    if (rth->rta_type == IFLA_IFNAME) {
                                        std::cout << "IFLA_IFNAME" << std::endl;
                                        const char* ifname = static_cast<const char*>(RTA_DATA(rth));
                                        bool if_up = (ifi->ifi_flags & IFF_UP);
                                        int ifindex = ifi->ifi_index;

                                        printf("ifname: %s, status: %s\n", ifname, if_up ? "up" : "down");
                                        std::cout << "ifindex: " << ifindex << std::endl;
                                    }
                                    /*if (rth->rta_type == IFLA_OPERSTATE) {
                                        stdifi->ifi_flags
                                    }*/
                                    if (rth->rta_type == IFA_LOCAL) {
                                        char name[IFNAMSIZ];
                                        if_indextoname(ifi->ifi_index, name);
                                        char ip[INET_ADDRSTRLEN];
                                        inet_ntop(AF_INET, RTA_DATA(rth), ip, sizeof(ip));
                                        printf("interface %s ip: %s\n", name, ip);
                                    }
                                    rth = RTA_NEXT(rth, rtl);
                                }
                            } else if (nlh->nlmsg_type == RTM_DELLINK) {
                                std::cout << "RTM_DELLINK" << std::endl;
                            }


                            nlh = NLMSG_NEXT(nlh, size);
                        }
                    }
                }
            }
        }


    }


}

/*
int main() {
    int sockfd;
    struct ifaddrs *ifaddr;
    int family, s;


    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        printf("%-8s %s (%d)\n",
            ifa->ifa_name,
           (family == AF_PACKET) ? "AF_PACKET" :
           (family == AF_INET) ? "AF_INET" :
           (family == AF_INET6) ? "AF_INET6" : "???",
           family);
        }


    sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    return 0;
}*/