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
#include <ctime>
#include <array>
#include <unistd.h>
#include <errno.h>
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
    struct sockaddr_un  name;
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, NAME, sizeof(name.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *) &name,sizeof(name));
    if (ret == -1) {
        perror("data_socket connect()");
    }

    ssize_t len;
    char buf[128];
    while (true) {
        /* Wait for next data packet. */

        len = read(data_socket, buf, sizeof(buf));
        if (len == -1) {
            perror("read unix socket stream");
            return -1;
        }
        if (len > 0) {
            std::cout << "mssg len" << len << std::endl;
        }

        //Ensure buffer is 0-terminated
        buf[sizeof(buf) - 1] = 0;

        if (!strncmp(buf, "END", sizeof(buf))) {
            std::cout<<"End signal recieved\n";
            break;
        }

        cli_neighbor_payload neighbor_pyld;
        std::memcpy(&neighbor_pyld, buf, sizeof(neighbor_pyld));

        //Print recieved neighbor info
        std::cout<<"neigh_conn.local_ifname: "<<neighbor_pyld.local_ifname<<std::endl;
        printf("neighbor_pyld.client_id: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 neighbor_pyld.client_id[0], neighbor_pyld.client_id[1], neighbor_pyld.client_id[2],
                 neighbor_pyld.client_id[3], neighbor_pyld.client_id[4], neighbor_pyld.client_id[5]);

        printf("neighbor_pyld.mac_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 neighbor_pyld.mac_addr[0], neighbor_pyld.mac_addr[1], neighbor_pyld.mac_addr[2],
                 neighbor_pyld.mac_addr[3], neighbor_pyld.mac_addr[4], neighbor_pyld.mac_addr[5]);

        if (neighbor_pyld.ip_family == AF_INET) {
            std::cout<<"ip_address_v4 "<<inet_ntoa(neighbor_pyld.ipv4)<<std::endl;
        } else if (neighbor_pyld.ip_family == AF_INET6) {
            printf("ip_address_v6: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 neighbor_pyld.ipv6.s6_addr[0], neighbor_pyld.ipv6.s6_addr[1], neighbor_pyld.ipv6.s6_addr[2],
                 neighbor_pyld.ipv6.s6_addr[3], neighbor_pyld.ipv6.s6_addr[4], neighbor_pyld.ipv6.s6_addr[5]);
        }
        /*
        if (down_flag) {
            continue;
        }
        */
        /* Add received summand. */

        //result += atoi(buffer);
    }

}
