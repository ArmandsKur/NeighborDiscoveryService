#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H
#include "interface.h"
#include <unordered_map>
class InterfaceManager {
    public:
        //InterfaceManager();
        int open_netlink_socket();
        void do_getlink_dump(int netlink_fd);
        void do_getaddr_dump(int netlink_fd);
        void socket_set_nonblock(int netlink_fd);
        void handle_netlink_event(pollfd pfd);
        void handle_newlink(struct nlmsghdr* nlh);
        void handle_dellink(struct nlmsghdr* nlh);
        void handle_newaddr(struct nlmsghdr* nlh);
        void handle_deladdr(struct nlmsghdr* nlh);
        void add_address(int ifindex, struct ip_address);
        void del_address(int ifindex, struct ip_address);

    private:
        std::unordered_map<int,ethernet_interface> interface_list;
        int netlink_fd;

};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H