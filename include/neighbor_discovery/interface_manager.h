#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H

#include "neighbor_discovery/interface.h"
#include <unordered_map>
class NeighborManager;

class InterfaceManager {
    public:
        int open_netlink_socket();
        void do_getlink_dump();
        void do_getaddr_dump();
        void socket_set_nonblock();
        void handle_netlink_event();
        void handle_newlink(nlmsghdr* nlh);
        void handle_dellink(nlmsghdr* nlh);
        void handle_newaddr(nlmsghdr* nlh);
        void handle_deladdr(nlmsghdr* nlh);
        void add_address(int ifindex, ip_address);
        void del_address(int ifindex, ip_address);
        ip_address get_ip_address(const ethernet_interface& interface);

        std::unordered_map<int,ethernet_interface> get_interface_list();

    private:
        std::unordered_map<int,ethernet_interface> interface_list;
        int netlink_fd;


};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H