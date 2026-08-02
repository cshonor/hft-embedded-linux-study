#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* Linux abstract name: sun_path[0] == '\0', no filesystem inode. */
#define ANAME "ch57_abstract"

static socklen_t fill_abstract(struct sockaddr_un *addr)
{
    size_t nlen;

    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    addr->sun_path[0] = '\0';
    nlen = strlen(ANAME);
    memcpy(addr->sun_path + 1, ANAME, nlen);
    return (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + nlen);
}

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s server\n", argv0);
    fprintf(stderr, "       %s client <msg>\n", argv0);
}

int main(int argc, char *argv[])
{
    struct sockaddr_un addr;
    socklen_t alen;
    int sfd;

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    alen = fill_abstract(&addr);
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket");
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        int cfd;
        char buf[256];
        ssize_t n;

        if (bind(sfd, (struct sockaddr *)&addr, alen) == -1) {
            perror("bind");
            return 1;
        }
        if (listen(sfd, 5) == -1) {
            perror("listen");
            return 1;
        }
        printf("abstract listen @\\0%s\n", ANAME);
        cfd = accept(sfd, NULL, NULL);
        if (cfd == -1) {
            perror("accept");
            return 1;
        }
        n = read(cfd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("server got: %s\n", buf);
        }
        close(cfd);
        close(sfd);
        return 0;
    }

    if (strcmp(argv[1], "client") == 0) {
        const char *msg;

        if (argc < 3) {
            usage(argv[0]);
            return 1;
        }
        msg = argv[2];
        if (connect(sfd, (struct sockaddr *)&addr, alen) == -1) {
            perror("connect");
            return 1;
        }
        write(sfd, msg, strlen(msg));
        close(sfd);
        return 0;
    }

    usage(argv[0]);
    return 1;
}
