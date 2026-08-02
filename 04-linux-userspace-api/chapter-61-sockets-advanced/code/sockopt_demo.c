#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void)
{
    int fd, opt, got;
    socklen_t olen;
    struct sockaddr_in addr, local;
    char ip[INET_ADDRSTRLEN];

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    /* Must set before bind. */
    opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("SO_REUSEADDR");
        return 1;
    }

    opt = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1) {
        perror("TCP_NODELAY");
        return 1;
    }

    olen = sizeof(got);
    if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &got, &olen) == 0)
        printf("TCP_NODELAY=%d\n", got);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0); /* ephemeral */

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    olen = sizeof(local);
    if (getsockname(fd, (struct sockaddr *)&local, &olen) == -1) {
        perror("getsockname");
        return 1;
    }
    inet_ntop(AF_INET, &local.sin_addr, ip, sizeof(ip));
    printf("getsockname: %s:%u (kernel-assigned port)\n",
           ip, (unsigned)ntohs(local.sin_port));

    close(fd);
    return 0;
}
