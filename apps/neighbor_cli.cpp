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
        /*
        if (down_flag) {
            continue;
        }
        */
        /* Add received summand. */

        //result += atoi(buffer);
    }

}
