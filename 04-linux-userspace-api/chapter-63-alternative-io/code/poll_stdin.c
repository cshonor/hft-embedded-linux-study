#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    struct pollfd pfd;
    char buf[128];
    ssize_t n;
    int ready;

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;

    printf("poll STDIN for 5s (type a line)...\n");
    ready = poll(&pfd, 1, 5000);
    if (ready == -1) {
        perror("poll");
        return 1;
    }
    if (ready == 0) {
        printf("timeout — no input\n");
        return 0;
    }

    if (pfd.revents & (POLLERR | POLLHUP)) {
        printf("err/hup revents=0x%x\n", pfd.revents);
        return 1;
    }
    if (pfd.revents & POLLIN) {
        n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("ready, got: %s", buf);
        }
    }
    return 0;
}
