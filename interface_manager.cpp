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
#include <unordered_map>
#include <errno.h>

#include "interface.h"
#include "interface_manager.h"
#include "neighbor_manager.h"

//Function used to create netlink socket and return its fd
int InterfaceManager::open_netlink_socket() {
    struct sockaddr_nl sa;
    netlink_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (netlink_fd != -1) {
        memset(&sa, 0, sizeof(sa));
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = RTMGRP_LINK |
                       RTMGRP_IPV4_IFADDR |
                       RTMGRP_IPV6_IFADDR;
        bind(netlink_fd, (struct sockaddr*)&sa, sizeof(sa));

        return netlink_fd;
    } else {
        perror("socket");
        return -1;
    }
}

/*
 *Function used to perform initial getlink dump which
 *Done so the interface_list can be filled with interface data
 */
void InterfaceManager::do_getlink_dump(int netlink_fd) {
    int len;
    char buffer[8192];
    int getlink_dump_seq = 1;

    //Create struct to send getlink dump request
    struct {
        struct nlmsghdr nlh;
        struct ifinfomsg ifm;
    } req;
    req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nlh.nlmsg_type = RTM_GETLINK;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_seq = getlink_dump_seq;
    req.nlh.nlmsg_pid = getpid();
    req.ifm.ifi_family = AF_UNSPEC;


    send(netlink_fd, &req, sizeof(req), 0);

    bool is_dump_done = false;
    while (!is_dump_done) {

        len = recv(netlink_fd, &buffer, sizeof(buffer), 0);
        for (struct nlmsghdr* nlh = (struct nlmsghdr*)buffer;NLMSG_OK(nlh, len);nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_seq != getlink_dump_seq)
                continue;
            if (nlh->nlmsg_type == NLMSG_DONE) {
                is_dump_done = true;
                break;
            }
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                std::cerr << "Netlink error\n";
                is_dump_done = true;
                break;
            }
            if (nlh->nlmsg_type == RTM_NEWLINK) {
                std::cout<<"RTM_NEWLINK"<<std::endl;
                handle_newlink(nlh);
            }

        }
    }
}
//Same idea as with getlink, just for IP addresses
void InterfaceManager::do_getaddr_dump(int netlink_fd) {
    int len;
    char buffer[8192];
    int getaddr_dump_seq = 2;
    //Create struct to send getaddr dump request
    struct {
        struct nlmsghdr nlh;
        struct ifinfomsg ifm;
    } req;
    req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nlh.nlmsg_type = RTM_GETADDR;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_seq = getaddr_dump_seq;
    req.nlh.nlmsg_pid = getpid();
    req.ifm.ifi_family = AF_UNSPEC;


    send(netlink_fd, &req, sizeof(req), 0);

    bool is_dump_done = false;
    while (!is_dump_done) {

        len = recv(netlink_fd, &buffer, sizeof(buffer), 0);
        for (struct nlmsghdr* nlh = (struct nlmsghdr*)buffer;NLMSG_OK(nlh, len);nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_seq != getaddr_dump_seq)
                continue;
            if (nlh->nlmsg_type == NLMSG_DONE) {
                is_dump_done = true;
                break;
            }
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                std::cerr << "Netlink error\n";
                is_dump_done = true;
                break;
            }
            if (nlh->nlmsg_type == RTM_NEWADDR) {
                std::cout<<"RTM_NEWADDR"<<std::endl;
                handle_newaddr(nlh);
            }

        }
    }
}

void InterfaceManager::socket_set_nonblock(int netlink_fd) {
    int flags = fcntl(netlink_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
    }
    if (fcntl(netlink_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
    }
}

void InterfaceManager::handle_netlink_event(pollfd pfd) {
    if (pfd.revents & POLLIN) {
        char buffer[8192];
        struct iovec iov = {buffer,sizeof(buffer)};
        struct sockaddr_nl sa;
        struct msghdr msg = {&sa,sizeof(sa),&iov,1};

        ssize_t len = recvmsg(pfd.fd, &msg, 0);
        //Stop reading if error on recvmsg
        if (len < 0) {
            //In case if nothing to read then don't print any errors
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            perror("recvmsg");
            return;
        }

        //Handle only kernel messages
        if (sa.nl_pid != 0)
            return;

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
                    handle_newlink(nlh);
                    break;
                case RTM_DELLINK:
                    std::cout << "RTM_DELLINK" << std::endl;
                    handle_dellink(nlh);
                    break;
                case RTM_NEWADDR:
                    std::cout << "RTM_NEWADDR" << std::endl;
                    handle_newaddr(nlh);
                    break;
                case RTM_DELADDR:
                    std::cout << "RTM_DELADDR" << std::endl;
                    handle_deladdr(nlh);
                    break;
            }
        }
    }
}

void InterfaceManager::handle_newlink(struct nlmsghdr *nlh) {
    auto *ifi = static_cast<struct ifinfomsg *>(NLMSG_DATA(nlh));

    //Get interface index and status
    int ifindex = ifi->ifi_index;
    bool if_up = (ifi->ifi_flags & IFF_UP);
    bool if_running = (ifi->ifi_flags & IFF_RUNNING);
    bool is_active = (if_up && if_running);
    bool is_loopback = (ifi->ifi_flags & IFF_LOOPBACK);
    interface_list[ifindex].ifindex = ifindex;
    interface_list[ifindex].is_active = is_active;
    interface_list[ifindex].is_loopback = is_loopback;



    struct rtattr *rta = IFLA_RTA(ifi);
    int rtl = IFLA_PAYLOAD(nlh);
    while (RTA_OK(rta, rtl)) {
        //Get interface name
        if (rta->rta_type == IFLA_IFNAME) {
            auto ifname = static_cast<char *>((RTA_DATA(rta)));
            interface_list[ifindex].ifname = ifname;
            printf(
                "ifname: %s, status: %s, i_runnning: %s, ifindex: %i\n",
                ifname,
                if_up ? "up" : "down",
                if_running ? "running" : "stopped",
                ifindex
            );
        }
        //Get interface mac address
        if (rta->rta_type == IFLA_ADDRESS) {
            auto mac_addr = static_cast<uint8_t *>(RTA_DATA(rta));
            std::memcpy(interface_list[ifindex].mac_addr.data(), RTA_DATA(rta), 6);
            char buf[18];
            snprintf(buf, sizeof(buf),
                     "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac_addr[0], mac_addr[1], mac_addr[2],
                     mac_addr[3], mac_addr[4], mac_addr[5]);
            std::cout << "MAC address: " << buf << std::endl;
        }
        rta = RTA_NEXT(rta, rtl);
    }
}

void InterfaceManager::handle_dellink(struct nlmsghdr *nlh) {
    auto ifi = static_cast<struct ifinfomsg *>(NLMSG_DATA(nlh));

    //Ignore loopback interface
    if (ifi->ifi_flags & IFF_LOOPBACK) {
        return;
    }

    int ifindex = ifi->ifi_index;
    interface_list.erase(ifindex);
    printf("interface No.%i deleted\n", ifindex);
}

void InterfaceManager::add_address(int ifindex, ip_address ip_addr) {
    interface_list[ifindex].ip_addresses.push_back(ip_addr);
}

void InterfaceManager::del_address(int ifindex, ip_address ip_addr) {
    /*
    * Since on add_address I just push back without checking if it already exists
    * We have to delete all the same addresses from vector in one go
    * Probably not needed if RTM_NEWADDR comes only for unexistent addresses
    */
    auto& ips = interface_list[ifindex].ip_addresses;
    ips.erase(
        std::remove_if(
            ips.begin(),
            ips.end(),
            [ip_addr](const ip_address& ipaddr) {
                if (ip_addr.family == AF_INET) {
                    return ipaddr.ipv4.s_addr == ip_addr.ipv4.s_addr;
                } else {
                    return std::memcmp(ipaddr.ipv6.s6_addr, ip_addr.ipv6.s6_addr, 16) == 0; //raw byte comparison
                    //return ipaddr.ipv6.s6_addr == ip_addr.ipv6.s6_addr;
                }
                //return ipaddr.ipv4.s_addr == ip_addr.ipv4.s_addr; //Remove all ip addresses with similar s_addr from vector
            }
        ),
        ips.end()
    );
}

void InterfaceManager::handle_newaddr(struct nlmsghdr *nlh) {
    const auto ifa = static_cast<struct ifaddrmsg *>(NLMSG_DATA(nlh));
    const int ifindex = ifa->ifa_index;
    const sa_family_t ifa_family = ifa->ifa_family;
    std::cout<<"CHECK"<<std::endl;
    printf("ifa_family: %i, ifindex: %i\n", ifa_family, ifindex);
    struct rtattr *rta = IFA_RTA(ifa);
    int rtl = IFA_PAYLOAD(nlh);
    while (RTA_OK(rta, rtl)) {
        if (rta->rta_type == IFA_ADDRESS) {
            ip_address ipaddress = {};
            ipaddress.family = ifa_family;
            sockaddr_in6 testip;
            char ip[ifa_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            if (ifa_family == AF_INET) {
                std::memcpy(&ipaddress.ipv4, RTA_DATA(rta), sizeof(struct in_addr));
                inet_ntop(ifa_family,&ipaddress.ipv4,ip,sizeof(ip));
                printf("Interface No.%i ip address: %s addded\n",ifindex,ip);
            } else {
                std::memcpy(&ipaddress.ipv6, RTA_DATA(rta), sizeof(struct in6_addr));
                inet_ntop(ifa_family,&ipaddress.ipv6,ip,sizeof(ip));
                printf("Interface No.%i ip address: %s addded\n",ifindex,ip);
            }

            add_address(ifindex,ipaddress);
        }
        rta = RTA_NEXT(rta, rtl);
    }
}

void InterfaceManager::handle_deladdr(struct nlmsghdr *nlh) {
    auto *ifa = static_cast<struct ifaddrmsg *>(NLMSG_DATA(nlh));
    const int ifindex = ifa->ifa_index;
    const sa_family_t ifa_family = ifa->ifa_family;
    std::cout<<"CHECK"<<std::endl;
    printf("ifa_family: %i, ifindex: %i\n", ifa_family, ifindex);
    struct rtattr *rta = IFA_RTA(ifa);
    int rtl = IFA_PAYLOAD(nlh);
    while (RTA_OK(rta, rtl)) {
        if (rta->rta_type == IFA_ADDRESS) {
            ip_address ipaddress = {};
            ipaddress.family = ifa_family;
            if (ifa_family == AF_INET) {
                std::memcpy(&ipaddress.ipv4, RTA_DATA(rta), sizeof(struct in_addr));
            } else {
                std::memcpy(&ipaddress.ipv6, RTA_DATA(rta), sizeof(struct in6_addr));
            }
            del_address(ifindex,ipaddress);

            char ip[ifa_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            inet_ntop(ifa_family,RTA_DATA(rta),ip,sizeof(ip));
            printf("Interface No.%i ip address: %s deleted\n",ifindex,ip);
        }
        rta = RTA_NEXT(rta, rtl);
    }
}