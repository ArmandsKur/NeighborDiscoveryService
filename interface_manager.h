#ifndef NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H
#include "interface.h"
#include <unordered_map>
class InterfaceManager {
    public:
        void handle_newlink(struct nlmsghdr* nlh);
        void handle_dellink(struct nlmsghdr* nlh);
        void handle_newaddr(struct nlmsghdr* nlh);
        void handle_deladdr(struct nlmsghdr* nlh);
    private:
        std::unordered_map<int,ethernet_interface> interface_list;
};
#endif //NEIGHBORDISCOVERYSERVICE_INTERFACE_MANAGER_H