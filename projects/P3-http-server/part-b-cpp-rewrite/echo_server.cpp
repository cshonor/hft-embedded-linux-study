#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

constexpr int kPort = 8081;
constexpr int kBacklog = 16;

int main()
{
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(kPort);

    if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(listen_fd, kBacklog) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "C++ echo_server on " << kPort << '\n';

    for (;;) {
        int conn = accept(listen_fd, nullptr, nullptr);
        if (conn < 0) {
            perror("accept");
            continue;
        }
        char buf[1024];
        ssize_t n;
        while ((n = read(conn, buf, sizeof buf)) > 0) {
            if (write(conn, buf, static_cast<size_t>(n)) < 0)
                break;
        }
        close(conn);
    }
}
