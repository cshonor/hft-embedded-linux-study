/* Set atime/mtime with futimens; ctime updates automatically.
 * cc -Wall -Wextra -o futimens_demo futimens_demo.c
 * ./futimens_demo [/tmp/tlpi_ts_demo.txt]
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void show_times(const char *tag, const struct stat *st)
{
    char a[64], m[64], c[64];
    strftime(a, sizeof(a), "%F %T", localtime(&st->st_atim.tv_sec));
    strftime(m, sizeof(m), "%F %T", localtime(&st->st_mtim.tv_sec));
    strftime(c, sizeof(c), "%F %T", localtime(&st->st_ctim.tv_sec));
    printf("%s\n  atime %s\n  mtime %s\n  ctime %s\n", tag, a, m, c);
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/tmp/tlpi_ts_demo.txt";
    struct timespec ts[2];
    struct stat before, after;
    int fd;

    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    if (write(fd, "hi\n", 3) != 3) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    if (fstat(fd, &before) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    show_times("before futimens", &before);

    /* Set atime/mtime to a fixed epoch-ish time; ctime will become "now" */
    ts[0].tv_sec = 1000000000;  /* ~2001-09-09 */
    ts[0].tv_nsec = 0;
    ts[1].tv_sec = 1100000000;  /* ~2004-11-09 */
    ts[1].tv_nsec = 0;

    if (futimens(fd, ts) == -1) {
        perror("futimens");
        close(fd);
        return EXIT_FAILURE;
    }

    if (fstat(fd, &after) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    show_times("after futimens (ctime should be ~now)", &after);
    close(fd);
    printf("file: %s\n", path);
    return 0;
}
