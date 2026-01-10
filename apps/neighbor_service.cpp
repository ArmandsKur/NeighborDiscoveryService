#include <csignal>
#include "neighbor_discovery/event_poll.h"



static volatile sig_atomic_t keep_running = 1;
// Signal handler function
void signalHandler(int sig) {
    std::cout << "\nShutdown signal recieved. Signal: " << sig << std::endl;
    keep_running = 0;
    //exit(sig);
}
/*
 * NeighborDiscovery service must be ran with root privileges
 * otherwise won't be able to create raw sockets
 */
int main() {
    // Handle signal

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    EventPoll event_poll;

    if (!event_poll.startup()) {
        std::cout<<"event_poll startup failed\n";
        std::cout<<"Ending program execution\n";
        return -1;
    }
    event_poll.run_event_poll(keep_running);
    event_poll.shutdown();

    return 0;
}
