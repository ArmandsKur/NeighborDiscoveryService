#ifndef NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H
#define NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H
#include <sys/un.h>

#include "neighbor_discovery/neighbor_manager.h"

#define NAME "/tmp/NeighborDiscoveryService.sock"



class ClientManager {
    public:
        bool init();
        int get_listen_socket();
        int open_data_socket();
        bool get_conn_status();
        void write_message(cli_neighbor_payload payload);
        void end_message();
        void close_data_socket();

        void cleanup();
    private:
        int listen_socket = -1;
        int data_socket = -1;
        bool connected = false;
        sockaddr_un init_sockaddr_un();
        int open_listen_socket();
};
#endif //NEIGHBORDISCOVERYSERVICE_CLI_MANAGER_H