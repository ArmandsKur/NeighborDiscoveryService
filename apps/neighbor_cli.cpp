#include <sys/socket.h>
#include <sys/un.h>

int main() {
    int unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
}
