#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 16
#define BUFSZ 1024

int main(void)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    printf("echo_server listening on %d (Ctrl-C to stop)\n", PORT);

    for (;;) {
        int conn = accept(listen_fd, NULL, NULL);
        if (conn < 0) {
            perror("accept");
            continue;
        }
        char buf[BUFSZ];
        ssize_t n;
        while ((n = read(conn, buf, sizeof buf)) > 0) {
            if (write(conn, buf, (size_t)n) < 0) {
                perror("write");
                break;
            }
        }
        close(conn);
    }
}
