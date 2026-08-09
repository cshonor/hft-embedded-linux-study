#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s server <path>\n", argv0);
    fprintf(stderr, "       %s client <path> <msg>\n", argv0);
}

int main(int argc, char *argv[])
{
    const char *path;
    int fd;
    char buf[256];
    ssize_t n;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    path = argv[2];

    if (strcmp(argv[1], "server") == 0) {
        if (mkfifo(path, 0666) == -1 && errno != EEXIST) {
            perror("mkfifo");
            return 1;
        }
        fd = open(path, O_RDONLY);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        n = read(fd, buf, sizeof(buf) - 1);
        if (n < 0) {
            perror("read");
            close(fd);
            return 1;
        }
        buf[n] = '\0';
        printf("server got: %s\n", buf);
        close(fd);
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
        fd = open(path, O_WRONLY);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        if (write(fd, msg, strlen(msg)) < 0)
            perror("write");
        close(fd);
        return 0;
    }

    usage(argv[0]);
    return 1;
}
