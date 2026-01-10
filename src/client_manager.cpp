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
    connected = true;

    return data_socket;
}

bool ClientManager::get_conn_status() {
    return connected;
}


void ClientManager::write_message(cli_neighbor_payload payload) {
    size_t n = write(data_socket, &payload, sizeof(payload));
    if (n == -1) {
        perror("client_manager::write_message()");
    }

}

void ClientManager::end_message() {
    int n;
    const char end_mssg[] = "END";

    n = write(data_socket, end_mssg, 3);
    if (n == -1) {
        perror("ClientManager::end_message()");
        return;
    }
    if (n < 3) {
        std::cerr<<"Partial write of END mssg\n";
    }
}

void ClientManager::close_data_socket() {
    close(data_socket);
    connected = false;
}

void ClientManager::cleanup() {
    if (data_socket != -1) {
        close(data_socket);
        data_socket = -1;
    }
    if (listen_socket != -1) {
        close(listen_socket);
        listen_socket = -1;
    }
    unlink(NAME);
}



