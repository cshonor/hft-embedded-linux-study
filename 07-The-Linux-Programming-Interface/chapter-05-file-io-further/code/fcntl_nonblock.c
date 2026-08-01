/* Ch5: F_SETFL O_NONBLOCK on a pipe (disk files usually ignore it). */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    int pfd[2];
    if (pipe(pfd) < 0)
        die("pipe");

    int flags = fcntl(pfd[0], F_GETFL);
    if (flags < 0)
        die("F_GETFL");
    if (fcntl(pfd[0], F_SETFL, flags | O_NONBLOCK) < 0)
        die("F_SETFL O_NONBLOCK");

    char buf[8];
    ssize_t n = read(pfd[0], buf, sizeof(buf));
    if (n >= 0) {
        fprintf(stderr, "expected EAGAIN on empty nonblocking pipe\n");
        return 1;
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK)
        die("read");

    printf("empty nonblocking pipe: EAGAIN/EWOULDBLOCK as expected\n");
    close(pfd[0]);
    close(pfd[1]);
    return 0;
}
