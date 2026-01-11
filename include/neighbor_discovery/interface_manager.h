#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H


#include <unordered_map>
#include <linux/netlink.h>

#include "neighbor_discovery/interface.h"



class InterfaceManager {
    public:
        bool init();
        void handle_netlink_event();

        int get_netlink_socket();
        ip_address get_ip_address(const ethernet_interface& interface);
        const std::unordered_map<int,ethernet_interface>& get_interface_list();

        void cleanup();

    private:
        std::unordered_map<int,ethernet_interface> interface_list;
        int netlink_fd = -1;
        //init steps
        int open_netlink_socket();
        bool do_getlink_dump();
        bool do_getaddr_dump();
        bool socket_set_nonblock();
        //functions for processing netlink messages
        void handle_newlink(nlmsghdr* nlh);
        void handle_dellink(nlmsghdr* nlh);
        void handle_newaddr(nlmsghdr* nlh);
        void handle_deladdr(struct nlmsghdr* nlh);
        void add_address(int ifindex, ip_address);
        void del_address(int ifindex, ip_address);



};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H