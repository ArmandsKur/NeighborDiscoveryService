#include <algorithm>
#include <iostream>
#include <vector>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>

#include "neighbor_discovery/event_poll.h"

//Function to insert new fds to pollfd vector and store role of fd in the pfd_role map
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
    if_mngr.do_getlink_dump();
    if_mngr.do_getaddr_dump();
    if_mngr.socket_set_nonblock();
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
                    //Send broadcast message trough each active interface
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
        }
    }
}