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

#include "interface.h"
#include "interface_manager.h"
/*
InterfaceManager::InterfaceManager() {
    struct ifaddrs *ifaddr;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        std::cout<< "addr" << std::endl;
        if (ifa->ifa_addr == NULL)
            continue;
        std::string ifname = ifa->ifa_name;
        std::cout<< "ifname: " << ifname << std::endl;
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            // = inet_ntoa(addr->sin_addr);
            //addr->sin_addr
            char ip[family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            inet_ntop(family,&addr->sin_addr,ip,sizeof(ip));
            std::cout<< "AF_INET " << ip << std::endl;
        } else if (family == AF_INET6) {
            char ip[family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            inet_ntop(family,ifa->ifa_addr->sa_data,ip,sizeof(ip));
            std::cout<< "AF_INET6 " << ip << std::endl;
        } else { //No Ip address for interface
            std::cout<< "no address" << std::endl;
        }

        bool if_up = (ifa->ifa_flags & IFF_UP);
        bool if_running = (ifa->ifa_flags & IFF_RUNNING);
        bool is_loopback = (ifa->ifa_flags & IFF_LOOPBACK);
        bool is_active = (if_up && if_running);


    }
}
*/
void InterfaceManager::handle_newlink(struct nlmsghdr *nlh) {
    auto *ifi = static_cast<struct ifinfomsg *>(NLMSG_DATA(nlh));

    //Get interface index and status
    int ifindex = ifi->ifi_index;
    bool if_up = (ifi->ifi_flags & IFF_UP);
    bool if_running = (ifi->ifi_flags & IFF_RUNNING);
    bool is_active = (if_up && if_running);
    interface_list[ifindex].ifindex = ifindex;
    interface_list[ifindex].is_active = is_active;

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
            //interface_list[ifindex].mac_addr = mac_addr;
            std::memcpy(interface_list[ifindex].mac_addr.data(), RTA_DATA(rta), 6);
            char buf[18];
            snprintf(buf, sizeof(buf),
                     "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac_addr[0], mac_addr[1], mac_addr[2],
                     mac_addr[3], mac_addr[4], mac_addr[5]);
            std::cout << "ip address: " << buf << std::endl;
        }
        rta = RTA_NEXT(rta, rtl);
    }
}

void InterfaceManager::handle_dellink(struct nlmsghdr *nlh) {
    auto ifi = static_cast<struct ifinfomsg *>(NLMSG_DATA(nlh));
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
            if (ifa_family == AF_INET) {
                std::memcpy(&ipaddress.ipv4, RTA_DATA(rta), sizeof(struct in_addr));
            } else {
                std::memcpy(&ipaddress.ipv6, RTA_DATA(rta), sizeof(struct in6_addr));
            }

            add_address(ifindex,ipaddress);
            //interface_list[ifindex]
            char ip[ifa_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
            inet_ntop(ifa_family,RTA_DATA(rta),ip,sizeof(ip));
            printf("Interface No.%i ip address: %s addded\n",ifindex,ip);
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