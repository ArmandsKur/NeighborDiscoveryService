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

void process_rtm_newlink(nlmsghdr* nlh) {
    struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
    struct rtattr *rta = IFLA_RTA(ifi);
    int rtl = IFLA_PAYLOAD(nlh);
    while (RTA_OK(rta, rtl)) {
        if (rta->rta_type == IFLA_IFNAME) {
            const char* ifname = (char*)(RTA_DATA(rta));
            bool if_up = (ifi->ifi_flags & IFF_UP);
            bool if_running = (ifi->ifi_flags & IFF_RUNNING);
            int ifindex = ifi->ifi_index;

            printf(
                "ifname: %s, status: %s, i_runnning: %s, ifindex: %i\n",
                ifname,
                if_up ? "up" : "down",
                if_running ? "running" : "stopped",
                ifindex
            );
            //std::cout << "ifindex: " << ifindex << std::endl;
        }
        if (rta->rta_type == IFLA_ADDRESS) {
            const uint8_t* mac_addr = (uint8_t*)RTA_DATA(rta);
            char buf[18];
            snprintf(buf, sizeof(buf),
                     "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac_addr[0], mac_addr[1], mac_addr[2],
                     mac_addr[3], mac_addr[4], mac_addr[5]);
            std::cout << "Interface MAC address: " << buf << std::endl;
        }
        if (rta->rta_type == IFLA_OPERSTATE) {
            const char* operstate = (char*)(RTA_DATA(rta));
            std::cout << "operstate: " << operstate << std::endl;
        }
        rta = RTA_NEXT(rta, rtl);
    }

}

void process_rtm_dellink(nlmsghdr* nlh) {
    struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
    int ifindex = ifi->ifi_index;
    printf("interface No.%i deleted\n", ifindex);
}

void process_rtm_newaddr(nlmsghdr* nlh) {
    struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
    int ifindex = ifa->ifa_index;
    sa_family_t ifa_family = ifa->ifa_family;
    std::cout<<"CHECK"<<std::endl;
    printf("ifa_family: %i, ifindex: %i\n", ifa_family, ifindex);
    struct rtattr *rta = IFA_RTA(ifa);
    int rtl = IFA_PAYLOAD(nlh);
    while (RTA_OK(rta, rtl)) {
        if (rta->rta_type == IFA_ADDRESS) {
            char ip[ifa_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            inet_ntop(ifa_family,RTA_DATA(rta),ip,sizeof(ip));
            printf("Interface No.%i ip address: %s addded\n",ifindex,ip);
        }
        rta = RTA_NEXT(rta, rtl);
    }
}
void process_rtm_deladdr(nlmsghdr* nlh) {
    struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
    int ifindex = ifa->ifa_index;
    sa_family_t ifa_family = ifa->ifa_family;
    std::cout<<"CHECK"<<std::endl;
    printf("ifa_family: %i, ifindex: %i\n", ifa_family, ifindex);
    struct rtattr *rta = IFA_RTA(ifa);
    int rtl = IFA_PAYLOAD(nlh);
    while (RTA_OK(rta, rtl)) {
        if (rta->rta_type == IFA_ADDRESS) {
            char ip[ifa_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            inet_ntop(ifa_family,RTA_DATA(rta),ip,sizeof(ip));
            printf("Interface No.%i ip address: %s deleted\n",ifindex,ip);
        }
        rta = RTA_NEXT(rta, rtl);
    }
}

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
        //sa.nl_groups = RTMGRP_LINK;
        bind(netlink_fd, (struct sockaddr*)&sa, sizeof(sa));
    } else {
        perror("socket");
    }
    add_to_pdfds(&pfds,netlink_fd,&fd_count);

    std::cout << "fd_count: " << fd_count << std::endl;

    InterfaceManager if_mngr;

    while (fd_count > 0) {
        ready = poll(pfds.data(), fd_count, -1);
        if (ready == -1)
            perror("poll");
        printf("Ready: %d\n", ready);

        for (const pollfd& pfd : pfds) {
            if (pfd.revents != 0) {
                if (pfd.revents & POLLIN) {
                    std::cout << "Read" << std::endl;
                    char buffer[8192];
                    struct iovec iov = {buffer,sizeof(buffer)};
                    struct sockaddr_nl sa;
                    struct msghdr msg = {&sa,sizeof(sa),&iov,1};

                    ssize_t len = recvmsg(pfd.fd, &msg, 0);
                    if (len < 0) {
                        perror("recvmsg");
                        return -1;
                    }

                    /* Kernel messages only */
                    if (sa.nl_pid != 0)
                        continue;

                    for (struct nlmsghdr* nlh = (struct nlmsghdr*)buffer;NLMSG_OK(nlh, len);nlh = NLMSG_NEXT(nlh, len)) {
                        if (nlh->nlmsg_type == NLMSG_DONE)
                            break;

                        if (nlh->nlmsg_type == NLMSG_ERROR) {
                            std::cerr << "Netlink error\n";
                            continue;
                        }

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