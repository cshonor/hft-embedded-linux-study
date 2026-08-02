/* Atomic-ish replace: write temp in same dir, then rename.
 * cc -Wall -Wextra -o rename_safe_write rename_safe_write.c
 * ./rename_safe_write /tmp/tlpi_safe.txt "payload"
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *target;
    const char *payload;
    char tmp[512];
    int fd;
    size_t len;

    if (argc < 3) {
        fprintf(stderr, "usage: %s TARGET TEXT\n", argv[0]);
        return EXIT_FAILURE;
    }
    target = argv[1];
    payload = argv[2];
    len = strlen(payload);

    /* Same directory as target so rename stays on one filesystem */
    if (snprintf(tmp, sizeof(tmp), "%s.tmp.%ld", target, (long)getpid())
        >= (int)sizeof(tmp)) {
        fprintf(stderr, "path too long\n");
        return EXIT_FAILURE;
    }

    fd = open(tmp, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0644);
    if (fd == -1) {
        perror("open tmp");
        return EXIT_FAILURE;
    }
    if (write(fd, payload, len) != (ssize_t)len ||
        write(fd, "\n", 1) != 1) {
        perror("write");
        close(fd);
        unlink(tmp);
        return EXIT_FAILURE;
    }
    if (fsync(fd) == -1) {
        perror("fsync");
        close(fd);
        unlink(tmp);
        return EXIT_FAILURE;
    }
    close(fd);

    if (rename(tmp, target) == -1) {
        perror("rename");
        unlink(tmp);
        return EXIT_FAILURE;
    }

    printf("wrote via rename: %s\n", target);
    return 0;
}
