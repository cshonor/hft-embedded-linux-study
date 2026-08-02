#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static void reap_children(int sig)
{
    int saved = errno;

    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;
    errno = saved;
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *rp;
    struct sigaction sa;
    int lfd = -1, cfd, opt, s;
    pid_t pid;
    char buf[256];
    ssize_t n;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <bind-host> <port>\n", argv[0]);
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = reap_children;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1)
            continue;
        opt = 1;
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

    if (listen(lfd, 16) == -1) {
        perror("listen");
        return 1;
    }
    printf("fork server on %s:%s\n", argv[1], argv[2]);

    for (;;) {
        cfd = accept(lfd, NULL, NULL);
        if (cfd == -1) {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;
        }

        pid = fork();
        if (pid == -1) {
            perror("fork");
            close(cfd);
            continue;
        }

        if (pid == 0) {
            close(lfd); /* child: drop listen fd */
            n = read(cfd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                dprintf(cfd, "pid=%ld echo:%s", (long)getpid(), buf);
            }
            close(cfd);
            _exit(0);
        }

        close(cfd); /* parent: drop conn fd copy */
    }
}
