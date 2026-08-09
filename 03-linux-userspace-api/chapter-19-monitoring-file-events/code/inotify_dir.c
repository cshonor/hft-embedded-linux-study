/* Blocking inotify on a directory.
 * cc -Wall -Wextra -o inotify_dir inotify_dir.c
 * ./inotify_dir DIR
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#define BUF_LEN (16 * (sizeof(struct inotify_event) + NAME_MAX + 1))

static void print_mask(uint32_t mask)
{
    if (mask & IN_ACCESS)        printf(" ACCESS");
    if (mask & IN_MODIFY)        printf(" MODIFY");
    if (mask & IN_ATTRIB)        printf(" ATTRIB");
    if (mask & IN_CLOSE_WRITE)   printf(" CLOSE_WRITE");
    if (mask & IN_CLOSE_NOWRITE) printf(" CLOSE_NOWRITE");
    if (mask & IN_OPEN)          printf(" OPEN");
    if (mask & IN_CREATE)        printf(" CREATE");
    if (mask & IN_DELETE)        printf(" DELETE");
    if (mask & IN_DELETE_SELF)   printf(" DELETE_SELF");
    if (mask & IN_MOVE_SELF)     printf(" MOVE_SELF");
    if (mask & IN_MOVED_FROM)    printf(" MOVED_FROM");
    if (mask & IN_MOVED_TO)      printf(" MOVED_TO");
    if (mask & IN_IGNORED)       printf(" IGNORED");
    if (mask & IN_Q_OVERFLOW)    printf(" Q_OVERFLOW");
    if (mask & IN_UNMOUNT)       printf(" UNMOUNT");
    if (mask & IN_ISDIR)         printf(" [DIR]");
}

int main(int argc, char *argv[])
{
    const char *dir;
    int fd, wd;
    char buf[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));

    if (argc != 2) {
        fprintf(stderr, "usage: %s DIR\n", argv[0]);
        return EXIT_FAILURE;
    }
    dir = argv[1];

    fd = inotify_init();
    if (fd == -1) {
        perror("inotify_init");
        return EXIT_FAILURE;
    }

    wd = inotify_add_watch(fd, dir,
                           IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB |
                           IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("watching %s (wd=%d); Ctrl-C to stop\n", dir, wd);

    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        char *p;

        if (n <= 0) {
            perror("read");
            break;
        }
        for (p = buf; p < buf + n; ) {
            struct inotify_event *ev = (struct inotify_event *)p;
            printf("wd=%d cookie=%u", ev->wd, ev->cookie);
            print_mask(ev->mask);
            if (ev->len > 0)
                printf(" name=\"%s\"", ev->name);
            printf("\n");
            if (ev->mask & IN_Q_OVERFLOW)
                fprintf(stderr, "WARNING: event queue overflow — state may be stale\n");
            p += sizeof(struct inotify_event) + ev->len;
        }
    }

    close(fd);
    return 0;
}
