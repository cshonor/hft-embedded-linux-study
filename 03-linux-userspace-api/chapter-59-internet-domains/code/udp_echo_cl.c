#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *rp;
    int fd, s;
    char buf[256];
    ssize_t n;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <host> <port> <msg>\n", argv[0]);
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

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
        if (sendto(fd, argv[3], strlen(argv[3]), 0,
                   rp->ai_addr, rp->ai_addrlen) >= 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd == -1) {
        fprintf(stderr, "sendto failed\n");
        return 1;
    }

    n = recvfrom(fd, buf, sizeof(buf) - 1, 0, NULL, NULL);
    if (n > 0) {
        buf[n] = '\0';
        printf("echo: %s\n", buf);
    }
    close(fd);
    return 0;
}
