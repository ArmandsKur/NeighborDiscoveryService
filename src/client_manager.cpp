

#include "neighbor_discovery/client_manager.h"

//Initialize the unix domain sock address
sockaddr_un ClientManager::init_sockaddr_un() {
    sockaddr_un name;
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, NAME, sizeof(name.sun_path) - 1);
    return name;

}

void ClientManager::socket_set_nonblock(int sock_fd) {
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
    }
    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
    }
}

//Create an unix socket and listen on the sun_path NAME
int ClientManager::open_listen_socket() {
    int ret;
    listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        perror("unix_socket");
        return -1;
    }
    struct sockaddr_un name = init_sockaddr_un();
    ret = bind(listen_socket, (const struct sockaddr *) &name,sizeof(name));
    if (ret == -1) {
        perror("bind");
        return -1;
    }
    ret = listen(listen_socket, 1);
    if (ret == -1) {
        perror("listen");
        return -1;
    }

    socket_set_nonblock(listen_socket);

    return listen_socket;

}

int ClientManager::open_data_socket() {
    data_socket = accept(listen_socket, NULL, NULL);
    if (data_socket == -1) {
        perror("accept");
        return -1;
    }
}



