#include <algorithm>
#include <iostream>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>

#include "neighbor_discovery/neighbor_manager.h"

#define NAME "/tmp/NeighborDiscoveryService.sock"

int main() {
    int ret;
    int data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("unix_socket socket()");
        return -1;
    }

    //connect socket to the address
    sockaddr_un  name;
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, NAME, sizeof(name.sun_path) - 1);

    ret = connect(data_socket, (const sockaddr *) &name,sizeof(name));
    if (ret == -1) {
        perror("data_socket connect()");
        return -1;
    }

    ssize_t len;
    char buf[sizeof(cli_neighbor_payload)];
    uint8_t prev_client_id[16] = {};
    int conn_num = 1;
    while (true) {

        /* Wait for next data packet. */

        len = read(data_socket, buf, sizeof(buf));
        if (len == -1) {
            perror("read unix socket stream");
            return -1;
        }

        if (!strncmp(buf, "END", 3)) {
            break;
        }

        if (len < sizeof(cli_neighbor_payload)) {
            std::cerr<<"received payload is less than expected\n";
            return -1;
        }

        cli_neighbor_payload neighbor_pyld;
        std::memcpy(&neighbor_pyld, buf, sizeof(neighbor_pyld));

        //Print neighbor ID only once per neighbor
        if (memcmp(prev_client_id,neighbor_pyld.client_id,16) != 0) {
            //Print received neighbor info
            printf("NeighborID: %02x:%02x:%02x:%02x:%02x:%02x\n",
                             neighbor_pyld.client_id[0], neighbor_pyld.client_id[1], neighbor_pyld.client_id[2],
                             neighbor_pyld.client_id[3], neighbor_pyld.client_id[4], neighbor_pyld.client_id[5]);
            //Refresh connection number for each new neighbor
            conn_num = 1;
        }

        printf("%d. ",conn_num);
        printf("ifname: %s, ", neighbor_pyld.local_ifname);
        printf("neighbor_pyld.mac_addr: %02x:%02x:%02x:%02x:%02x:%02x, ",
                 neighbor_pyld.mac_addr[0], neighbor_pyld.mac_addr[1], neighbor_pyld.mac_addr[2],
                 neighbor_pyld.mac_addr[3], neighbor_pyld.mac_addr[4], neighbor_pyld.mac_addr[5]);

        char ip[neighbor_pyld.ip_family == AF_INET6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
        if (neighbor_pyld.ip_family == AF_INET) {
            inet_ntop(AF_INET,&neighbor_pyld.ipv4,ip,sizeof(ip));
            printf("ip_address: %s\n",ip);
        } else if (neighbor_pyld.ip_family == AF_INET6) {
            inet_ntop(AF_INET6,&neighbor_pyld.ipv6,ip,sizeof(ip));
            printf("ip_address: %s\n",ip);
        } else {
            printf("ip_address: None\n");
        }

        //Update prev_client_id value
        std::memcpy(&prev_client_id,neighbor_pyld.client_id,16);
        //Increment conn_num for neighbors with multiple connections
        conn_num++;
    }

    close(data_socket);
}
