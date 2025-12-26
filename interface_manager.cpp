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


void InterfaceManager::handle_newlink(struct nlmsghdr *nlh) {
    struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);

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
            const char* ifname = (char*)(RTA_DATA(rta));
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
            const uint8_t* mac_addr = (uint8_t*)RTA_DATA(rta);
            interface_list[ifindex].mac_addr = mac_addr;
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
    struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
    int ifindex = ifi->ifi_index;
    interface_list.erase(ifindex);
    printf("interface No.%i deleted\n", ifindex);
}

void InterfaceManager::add_address(int ifindex, struct ip_address ip_addr) {
    interface_list[ifindex].ip_addresses.push_back(ip_addr);
}

void InterfaceManager::delete_address(int ifindex, struct ip_address) {
    /*
    * Since on add_address I just push back without checking if it already exists
    * We have to delete all the same addresses from vector in one go
    * Probably not needed if RTM_NEWADDR comes only for unexistand addresses
    */
}

void InterfaceManager::handle_newaddr(struct nlmsghdr *nlh) {
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

            //interface_list[ifindex]
            printf("Interface No.%i ip address: %s addded\n",ifindex,ip);
        }
        rta = RTA_NEXT(rta, rtl);
    }
}

void InterfaceManager::handle_deladdr(struct nlmsghdr *nlh) {
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


/*
class InterfaceManager {
    public:
        //On netlink message RTM_NEWLINK update the interface info.
        void handle_newlink(struct nlmsghdr* nlh) {
            struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);

            //Get interface index and status
            int ifindex = ifi->ifi_index;
            bool if_up = (ifi->ifi_flags & IFF_UP);
            bool if_running = (ifi->ifi_flags & IFF_RUNNING);
            bool is_usable = (if_up && if_running);
            interface_list[ifindex].ifindex = ifindex;
            interface_list[ifindex].is_usable = is_usable;

            struct rtattr *rta = IFLA_RTA(ifi);
            int rtl = IFLA_PAYLOAD(nlh);
            while (RTA_OK(rta, rtl)) {
                //Get interface name
                if (rta->rta_type == IFLA_IFNAME) {
                    const char* ifname = (char*)(RTA_DATA(rta));
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
                    const uint8_t* mac_addr = (uint8_t*)RTA_DATA(rta);
                    interface_list[ifindex].mac_addr = mac_addr;
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
        //on netlink message RTM_DELLINK delete the
        void handle_dellink(struct nlmsghdr* nlh) {
            struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(nlh);
            int ifindex = ifi->ifi_index;
            printf("interface No.%i deleted\n", ifindex);
        }
    private:
        std::unordered_map<int,ethernet_interface> interface_list;
};*/