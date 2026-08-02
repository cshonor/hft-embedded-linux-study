#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *rp;
    int lfd, cfd, s;
    char buf[256];
    ssize_t n;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <bind-host> <port>\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    /* host may be "127.0.0.1"; NULL would mean INADDR_ANY */

    s = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }

    lfd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1)
            continue;
        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(lfd);
        lfd = -1;
    }
    freeaddrinfo(res);
    if (lfd == -1) {
        fprintf(stderr, "bind failed\n");
        return 1;
    }

    if (listen(lfd, 5) == -1) {
        perror("listen");
        return 1;
    }
    printf("TCP listen %s:%s\n", argv[1], argv[2]);

    for (;;) {
        cfd = accept(lfd, NULL, NULL);
        if (cfd == -1) {
            perror("accept");
            continue;
        }
        n = read(cfd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("got: %s\n", buf);
            dprintf(cfd, "echo:%s", buf);
        }
        close(cfd); /* iterative: one client at a time */
    }
}
