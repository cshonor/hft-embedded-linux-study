#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *path;
    int fd;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }
    path = argv[1];

    fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK)
            fprintf(stderr, "flock: already locked\n");
        else
            perror("flock");
        return 1;
    }

    printf("flock LOCK_EX held on %s — sleeping\n", path);
    sleep(5);
    flock(fd, LOCK_UN);
    close(fd);
    printf("unlocked\n");
    return 0;
}
