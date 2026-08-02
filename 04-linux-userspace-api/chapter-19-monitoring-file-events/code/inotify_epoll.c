/* Nonblocking inotify + epoll loop.
 * cc -Wall -Wextra -o inotify_epoll inotify_epoll.c
 * ./inotify_epoll DIR
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>

#define BUF_LEN (16 * (sizeof(struct inotify_event) + NAME_MAX + 1))

static void handle_events(int ifd)
{
    char buf[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));
    char *p;

    for (;;) {
        ssize_t n = read(ifd, buf, sizeof(buf));
        if (n == -1) {
            if (errno == EAGAIN)
                return;
            perror("read");
            return;
        }
        if (n == 0)
            return;

        for (p = buf; p < buf + n; ) {
            struct inotify_event *ev = (struct inotify_event *)p;
            printf("event mask=0x%x", ev->mask);
            if (ev->len)
                printf(" name=%s", ev->name);
            if (ev->mask & (IN_MOVED_FROM | IN_MOVED_TO))
                printf(" cookie=%u", ev->cookie);
            if (ev->mask & IN_Q_OVERFLOW)
                printf(" <<OVERFLOW>>");
            printf("\n");
            p += sizeof(struct inotify_event) + ev->len;
        }
    }
}

int main(int argc, char *argv[])
{
    const char *dir;
    int ifd, efd, wd;
    struct epoll_event ev, events[1];

    if (argc != 2) {
        fprintf(stderr, "usage: %s DIR\n", argv[0]);
        return EXIT_FAILURE;
    }
    dir = argv[1];

    ifd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (ifd == -1) {
        perror("inotify_init1");
        return EXIT_FAILURE;
    }

    wd = inotify_add_watch(ifd, dir,
                           IN_CREATE | IN_DELETE | IN_MODIFY |
                           IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(ifd);
        return EXIT_FAILURE;
    }

    efd = epoll_create1(EPOLL_CLOEXEC);
    if (efd == -1) {
        perror("epoll_create1");
        close(ifd);
        return EXIT_FAILURE;
    }

    ev.events = EPOLLIN;
    ev.data.fd = ifd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, ifd, &ev) == -1) {
        perror("epoll_ctl");
        close(efd);
        close(ifd);
        return EXIT_FAILURE;
    }

    printf("epoll+inotify on %s; Ctrl-C to stop\n", dir);
    for (;;) {
        int n = epoll_wait(efd, events, 1, -1);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }
        if (events[0].data.fd == ifd)
            handle_events(ifd);
    }

    close(efd);
    close(ifd);
    return 0;
}
