/* Daemon with SIGHUP flag for "reload" (async-signal-safe pattern).
 * cc -Wall -Wextra -o daemon_sighup daemon_sighup.c become_daemon.c
 * ./daemon_sighup
 * kill -HUP $(cat /tmp/tlpi_daemon_sighup.pid)
 * kill $(cat /tmp/tlpi_daemon_sighup.pid)
 */
#include "become_daemon.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define PIDFILE "/tmp/tlpi_daemon_sighup.pid"

static volatile sig_atomic_t hup;
static volatile sig_atomic_t stop;
static int reload_count;

static void on_hup(int sig)
{
    (void)sig;
    hup = 1;
}

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

    if (becomeDaemon(0) == -1)
        return 1;

    openlog("tlpi-sighup", LOG_PID, LOG_DAEMON);

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_hup;
    sigaction(SIGHUP, &sa, NULL);
    sa.sa_handler = on_term;
    sigaction(SIGTERM, &sa, NULL);

    if (write_pid() == -1)
        return 1;

    syslog(LOG_INFO, "started");

    while (!stop) {
        if (hup) {
            hup = 0;
            reload_count++;
            syslog(LOG_NOTICE, "SIGHUP reload #%d", reload_count);
        }
        pause();
    }

    syslog(LOG_INFO, "stopping");
    closelog();
    unlink(PIDFILE);
    return 0;
}
