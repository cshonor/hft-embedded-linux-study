/* Prefer open + fstat over stat + open (TOCTOU / symlink races).
 * cc -Wall -Wextra -o open_fstat_safe open_fstat_safe.c
 * ./open_fstat_safe PATH
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    struct stat st;
    int fd;
    const char *path;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PATH\n", argv[0]);
        return 1;
    }
    path = argv[1];

    /* O_NOFOLLOW: fail if final component is a symlink (where supported) */
    fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd == -1) {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    printf("fd=%d mode=%04o uid=%u gid=%u size=%lld\n",
           fd,
           (unsigned)(st.st_mode & 07777),
           (unsigned)st.st_uid,
           (unsigned)st.st_gid,
           (long long)st.st_size);

    /* Further checks (owner, regular file, etc.) go here before use */
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "not a regular file\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
