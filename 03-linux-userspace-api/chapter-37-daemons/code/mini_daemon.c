/* Minimal daemon: becomeDaemon, write pid file, wait for SIGTERM.
 * cc -Wall -Wextra -o mini_daemon mini_daemon.c become_daemon.c
 * ./mini_daemon
 */
#include "become_daemon.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PIDFILE "/tmp/tlpi_mini_daemon.pid"

static volatile sig_atomic_t stop;

static void on_term(int sig)
{
    (void)sig;
    stop = 1;
}

static int write_pid(void)
{
    char buf[32];
    int fd, n;

    fd = open(PIDFILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1)
        return -1;
    n = snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
    if (write(fd, buf, (size_t)n) != n) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int main(void)
{
    struct sigaction sa;

    if (becomeDaemon(0) == -1) {
        perror("becomeDaemon");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    if (write_pid() == -1)
        return 1;

    while (!stop)
        pause();

    unlink(PIDFILE);
    return 0;
}
