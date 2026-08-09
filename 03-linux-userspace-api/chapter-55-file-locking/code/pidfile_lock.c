#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Single-instance pattern: exclusive record lock on the pid file.
 * Keep fd open for the process lifetime (closing releases all fcntl locks).
 */
int main(int argc, char *argv[])
{
    const char *path;
    int fd;
    struct flock fl;
    char buf[32];
    int n;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <pidfile>\n", argv[0]);
        return 1;
    }
    path = argv[1];

    fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; /* whole file through EOF */

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            fprintf(stderr, "another instance holds the lock\n");
            return 1;
        }
        perror("fcntl F_SETLK");
        return 1;
    }

    if (ftruncate(fd, 0) == -1) {
        perror("ftruncate");
        return 1;
    }
    n = snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
    if (write(fd, buf, (size_t)n) != n) {
        perror("write");
        return 1;
    }

    printf("locked %s pid=%ld — sleeping (Ctrl-C to exit)\n",
           path, (long)getpid());
    for (;;)
        pause();

    /* unreachable: process exit releases locks */
    return 0;
}
