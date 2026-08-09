/* O_DIRECT alignment: unaligned buffer → EINVAL; aligned → ok (Linux).
 *   cc -Wall -Wextra -D_GNU_SOURCE -o odirect_align odirect_align.c
 *   ./odirect_align /tmp/odirect_test.bin
 *
 * Some FS/kernels may reject O_DIRECT; then open fails — that is also a lesson.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef O_DIRECT
#error "O_DIRECT not available on this platform"
#endif

enum { BLK = 4096 };

static int try_write(int fd, void *buf, size_t len, const char *tag)
{
    ssize_t n = pwrite(fd, buf, len, 0);
    if (n < 0) {
        printf("%s: FAIL %s\n", tag, strerror(errno));
        return -1;
    }
    printf("%s: wrote %zd bytes\n", tag, n);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/tmp/odirect_test.bin";

    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open O_DIRECT");
        return 1;
    }

    void *aligned = NULL;
    if (posix_memalign(&aligned, BLK, BLK) != 0) {
        perror("posix_memalign");
        close(fd);
        return 1;
    }
    memset(aligned, 0xab, BLK);

    /* Misaligned pointer: +1 inside page — should EINVAL on Linux */
    unsigned char *bad = (unsigned char *)aligned + 1;
    try_write(fd, bad, BLK, "misaligned addr");

    /* Aligned */
    try_write(fd, aligned, BLK, "aligned 4096");

    if (fsync(fd) == -1)
        perror("fsync");

    free(aligned);
    close(fd);
    return 0;
}
