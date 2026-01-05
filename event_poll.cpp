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

#include "event_poll.h"

//Function to insert new fds to pollfd vectoe and store role of fd in the pfd_role map
void EventPoll::add_to_pfds(int new_fd, short events, PollFdRole role) {
    pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    new_pollfd.events = events; //POLLIN
    new_pollfd.revents = 0;

    pfds.push_back(new_pollfd);
    pfd_role[new_fd] = role;

    fd_count = pfds.size();
}
//Erase specific fd from pfds vector and pfd_role unordered_map
void EventPoll::del_from_pfds(int fd) {
    //erase fd from vector
    pfds.erase(
        std::remove_if(
            pfds.begin(),
            pfds.end(),
            [fd](const pollfd &pfd) {
                return pfd.fd == fd;
            }
        ),
        pfds.end()
    );
    fd_count = pfds.size();
    //erase fd from unordered map
    pfd_role.erase(fd);
}
//Function used to initialize netlink activities
void EventPoll::startup_netlink() {
    int netlink_fd = if_mngr.open_netlink_socket();
    if (netlink_fd == -1) {
        std::cerr << "Failed to open netlink socket\n";
    }
    add_to_pfds(netlink_fd, POLLIN, PollFdRole::Netlink);
    if_mngr.do_getlink_dump(netlink_fd);
    if_mngr.do_getaddr_dump(netlink_fd);
    if_mngr.socket_set_nonblock(netlink_fd);
}

//Function to create sockets for neighbor manager and store their fds in pdfs vector
void EventPoll::startup_neighbor_manager() {
    int recv_fd = neighbor_mngr.create_broadcast_recv_socket();
    if (recv_fd == -1) {
        std::cerr << "Failed to create broadcast recv socket\n";
        return;
    }
    add_to_pfds(recv_fd, POLLIN, PollFdRole::PacketRecv);
    int send_fd = neighbor_mngr.create_broadcast_send_socket();
    if (send_fd == -1) {
        std::cerr << "Failed to create broadcast send socket\n";
        return;
    }
    add_to_pfds(send_fd, POLLOUT, PollFdRole::PacketSend);
}
//Function used to run main event poll which will handle all the activities
void EventPoll::run_event_poll() {
    int ready;

    //Init time_point variables
    std::chrono::time_point<std::chrono::steady_clock> last_packet_time{};
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::cout<<"fd count: "<<fd_count<<std::endl;
    while (fd_count > 0) {
        ready = poll(pfds.data(), fd_count, -1);
        if (ready == -1)
            perror("poll");

        for (const pollfd& pfd : pfds) {
            if (pfd.revents == 0) {
                continue;
            }
            if (pfd_role[pfd.fd] == PollFdRole::Netlink) {
                if (pfd.revents & POLLIN) {
                    if_mngr.handle_netlink_event();
                }
            } else if (pfd_role[pfd.fd] == PollFdRole::PacketRecv) {
                if (pfd.revents & POLLIN) {
                    std::cout<<"Broadcast recieved\n";
                    neighbor_mngr.recv_broadcast();
                }
            } else if (pfd_role[pfd.fd] == PollFdRole::PacketSend) {
                now = std::chrono::steady_clock::now();
                //Send broadcast only each 5 seconds
                if (now - last_packet_time >= std::chrono::seconds(5)) {

                    //std::cout<<"did stuff each 5 seconds\n";
                    for (auto& it: if_mngr.get_interface_list()) {
                        auto interface = it.second;
                        neighbor_payload payload = neighbor_mngr.construct_neighbor_payload(
                            interface.mac_addr,
                            if_mngr.get_ip_address(interface)
                        );
                        if (interface.is_active && !interface.is_loopback) {
                            neighbor_mngr.send_broadcast(interface.ifindex,interface.mac_addr,payload);
                        }
                    }
                    last_packet_time = now;
                }
            }
            /*
            if (pfd.revents & POLLIN) {
                char buffer[8192];
                struct iovec iov = {buffer,sizeof(buffer)};
                struct sockaddr_nl sa;
                struct msghdr msg = {&sa,sizeof(sa),&iov,1};

                ssize_t len = recvmsg(pfd.fd, &msg, 0);
                //Stop reading if error on recvmsg
                if (len < 0) {
                    perror("recvmsg");
                    break;
                }

                //Handle only kernel messages
                if (sa.nl_pid != 0)
                    continue;

                for (struct nlmsghdr* nlh = (struct nlmsghdr*)buffer;NLMSG_OK(nlh, len);nlh = NLMSG_NEXT(nlh, len)) {
                    if (nlh->nlmsg_type == NLMSG_DONE)
                        break;

                    if (nlh->nlmsg_type == NLMSG_ERROR) {
                        std::cerr << "Netlink error\n";
                        continue;
                    }

                    if (nlh->nlmsg_seq != 0)
                        continue;

                    switch (nlh->nlmsg_type) {
                        case RTM_NEWLINK:
                            std::cout << "RTM_NEWLINK" << std::endl;
                            if_mngr.handle_newlink(nlh);
                            break;
                        case RTM_DELLINK:
                            std::cout << "RTM_DELLINK" << std::endl;
                            if_mngr.handle_dellink(nlh);
                            break;
                        case RTM_NEWADDR:
                            std::cout << "RTM_NEWADDR" << std::endl;
                            if_mngr.handle_newaddr(nlh);
                            break;
                        case RTM_DELADDR:
                            std::cout << "RTM_DELADDR" << std::endl;
                            if_mngr.handle_deladdr(nlh);
                            break;
                    }
                }
            }*/
        }
    }
}