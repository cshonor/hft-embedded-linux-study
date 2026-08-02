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
    struct sockaddr_un addr, peer;
    int sfd;
    const char *path;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    path = argv[2];

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(path) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "path too long\n");
        return 1;
    }
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        perror("socket");
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        char buf[256];
        socklen_t plen;
        ssize_t n;

        unlink(path);
        if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("bind");
            return 1;
        }
        printf("dgram recv on %s\n", path);
        plen = sizeof(peer);
        n = recvfrom(sfd, buf, sizeof(buf) - 1, 0,
                     (struct sockaddr *)&peer, &plen);
        if (n < 0) {
            perror("recvfrom");
            return 1;
        }
        buf[n] = '\0';
        printf("got (boundary preserved): %s\n", buf);
        close(sfd);
        unlink(path);
        return 0;
    }

    if (strcmp(argv[1], "client") == 0) {
        struct sockaddr_un local;
        const char *msg;
        const char *clipath = "/tmp/ch57-dgram-cli.sock";

        if (argc < 4) {
            usage(argv[0]);
            return 1;
        }
        msg = argv[3];

        /* Client should bind too for UNIX dgram identity / replies. */
        unlink(clipath);
        memset(&local, 0, sizeof(local));
        local.sun_family = AF_UNIX;
        strncpy(local.sun_path, clipath, sizeof(local.sun_path) - 1);
        if (bind(sfd, (struct sockaddr *)&local, sizeof(local)) == -1) {
            perror("bind client");
            return 1;
        }

        if (sendto(sfd, msg, strlen(msg), 0,
                   (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("sendto");
            return 1;
        }
        close(sfd);
        unlink(clipath);
        return 0;
    }

    usage(argv[0]);
    return 1;
}
