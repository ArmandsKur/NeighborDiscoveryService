#include "neighbor_discovery/event_poll.h"


/*
 * NeighborDiscovery service must be ran with root privileges
 * otherwise won't be able to create raw sockets
 */

int main() {
    EventPoll event_poll;
    if (!event_poll.startup()) {
        std::cout<<"event_poll startup failed\n";
        std::cout<<"Ending program execution\n";
        return -1;
    }
    event_poll.run_event_poll();

    return 0;
}
