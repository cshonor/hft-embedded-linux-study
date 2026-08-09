#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *rp;
    struct sockaddr_storage peer;
    socklen_t plen;
    int fd, s;
    char buf[256];
    ssize_t n;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <bind-host> <port>\n", argv[0]);
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }

    fd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1)
            continue;
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd == -1) {
        fprintf(stderr, "bind failed\n");
        return 1;
    }

    printf("UDP bind %s:%s\n", argv[1], argv[2]);
    for (;;) {
        plen = sizeof(peer);
        n = recvfrom(fd, buf, sizeof(buf), 0,
                     (struct sockaddr *)&peer, &plen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        sendto(fd, buf, (size_t)n, 0, (struct sockaddr *)&peer, plen);
    }
}
