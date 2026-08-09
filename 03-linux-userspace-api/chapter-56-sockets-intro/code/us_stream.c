#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s server <path>\n", argv0);
    fprintf(stderr, "       %s client <path> <msg>\n", argv0);
}

int main(int argc, char *argv[])
{
    struct sockaddr_un addr;
    int sfd;
    const char *path;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    path = argv[2];

    if (strlen(path) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "path too long\n");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (strcmp(argv[1], "server") == 0) {
        int cfd;
        char buf[256];
        ssize_t n;

        unlink(path);
        sfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sfd == -1) {
            perror("socket");
            return 1;
        }
        if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("bind");
            return 1;
        }
        if (listen(sfd, 5) == -1) {
            perror("listen");
            return 1;
        }
        printf("listening on %s\n", path);
        cfd = accept(sfd, NULL, NULL);
        if (cfd == -1) {
            perror("accept");
            return 1;
        }
        n = read(cfd, buf, sizeof(buf) - 1);
        if (n < 0) {
            perror("read");
            return 1;
        }
        buf[n] = '\0';
        printf("server got: %s\n", buf);
        close(cfd);
        close(sfd);
        unlink(path);
        return 0;
    }

    if (strcmp(argv[1], "client") == 0) {
        const char *msg;

        if (argc < 4) {
            usage(argv[0]);
            return 1;
        }
        msg = argv[3];
        sfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sfd == -1) {
            perror("socket");
            return 1;
        }
        if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("connect");
            return 1;
        }
        if (write(sfd, msg, strlen(msg)) < 0)
            perror("write");
        close(sfd);
        return 0;
    }

    usage(argv[0]);
    return 1;
}
