#include "neighbor_discovery/client_manager.h"

bool ClientManager::init() {
    if (open_listen_socket() == -1) {
        std::cerr << "Failed to open unix listen socket\n";
        return false;
    }
    return true;
}


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
        perror("unix_listen_socket socket()");
        return -1;
    }

    unlink(NAME);

    struct sockaddr_un name = init_sockaddr_un();
    ret = bind(listen_socket, (const struct sockaddr *) &name,sizeof(name));
    if (ret == -1) {
        perror("listen_socket bind()");
        return -1;
    }
    ret = listen(listen_socket, 5);
    if (ret == -1) {
        perror("unix_listen_socket_listen()");
        return -1;
    }

    //socket_set_nonblock(listen_socket);

    return listen_socket;

}

int ClientManager::get_listen_socket() {
    return listen_socket;
}


int ClientManager::open_data_socket() {
    data_socket = accept(listen_socket, NULL, NULL);
    if (data_socket == -1) {
        perror("accept");
        return -1;
    }
    std::cout<<"Unix socket opened"<<std::endl;
    connected = true;

    return data_socket;
}

bool ClientManager::get_conn_status() {
    return connected;
}

void ClientManager::write_message(neighbor_payload payload) {
    int n;
    uint8_t buf[128];
    memcpy(buf,&payload,sizeof(payload));

    n = write(data_socket, buf, sizeof(buf));
    if (n == -1) {
        perror("client_manager::write_message()");
        return;
    }

}

void ClientManager::end_message() {
    int n;
    char buf[128];
    strcpy(buf, "END");

    n = write(data_socket, buf, strlen(buf) + 1);
    if (n == -1) {
        perror("ClientManager::end_message()");
        return;
    }
}

void ClientManager::close_data_socket() {
    close(data_socket);
    connected = false;
}



