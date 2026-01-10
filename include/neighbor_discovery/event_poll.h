#ifndef NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H
#define NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H
#include <vector>
#include <signal.h>

#include "neighbor_discovery/neighbor_manager.h"
#include "neighbor_discovery/interface_manager.h"
#include "neighbor_discovery/client_manager.h"

enum class PollFdRole {
    Netlink,
    PacketRecv,
    PacketSend,
    UnixListen
};

class EventPoll{
    public:
        bool startup();
        void shutdown();
        void run_event_poll(volatile const sig_atomic_t& keep_running);

        void add_to_pfds(int new_fd, short events, PollFdRole role);
        void del_from_pfds(int fd);


    private:
        //Variables related to storing fds used in the poll
        std::vector<pollfd> pfds;
        std::unordered_map<int,PollFdRole> pfd_role;
        nfds_t fd_count = 0;

        //Manager classes
        NeighborManager neighbor_mngr;
        InterfaceManager if_mngr;
        ClientManager client_mngr;



};
#endif //NEIGHBORDISCOVERYSERVICE_EVENT_POLL_H