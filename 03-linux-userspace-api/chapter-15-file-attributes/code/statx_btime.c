/* Linux statx: try to print birth time (btime).
 * cc -Wall -Wextra -D_GNU_SOURCE -o statx_btime statx_btime.c
 * ./statx_btime PATH
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#ifndef STATX_BTIME
#error "statx / STATX_BTIME not available — need recent glibc + Linux headers"
#endif

int main(int argc, char *argv[])
{
    struct statx stx;
    const char *path;
    char buf[64];

    if (argc != 2) {
        fprintf(stderr, "usage: %s PATH\n", argv[0]);
        return EXIT_FAILURE;
    }
    path = argv[1];

    if (statx(AT_FDCWD, path, AT_SYMLINK_NOFOLLOW,
              STATX_BASIC_STATS | STATX_BTIME, &stx) == -1) {
        perror("statx");
        return EXIT_FAILURE;
    }

    printf("%s\n", path);
    printf("  size=%llu mode=%04o ino=%llu\n",
           (unsigned long long)stx.stx_size,
           (unsigned)(stx.stx_mode & 07777),
           (unsigned long long)stx.stx_ino);

    if (stx.stx_mask & STATX_BTIME) {
        time_t sec = (time_t)stx.stx_btime.tv_sec;
        strftime(buf, sizeof(buf), "%F %T", localtime(&sec));
        printf("  btime %s.%09u  (birth)\n", buf, stx.stx_btime.tv_nsec);
    } else {
        printf("  btime: not provided by this filesystem/kernel\n");
    }
    return 0;
}
