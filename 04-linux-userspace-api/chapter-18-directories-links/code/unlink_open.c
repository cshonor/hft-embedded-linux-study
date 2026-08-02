/* open → unlink → still read via fd (anonymous temp pattern).
 * cc -Wall -Wextra -o unlink_open unlink_open.c
 * ./unlink_open [/tmp/tlpi_unlink_open.txt]
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/tmp/tlpi_unlink_open.txt";
    char buf[64];
    ssize_t n;
    int fd;

    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    if (write(fd, "still here\n", 11) != 11) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    if (unlink(path) == -1) {
        perror("unlink");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("unlinked %s (name gone from directory)\n", path);

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return EXIT_FAILURE;
    }
    n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }
    buf[n] = '\0';
    printf("read via open fd after unlink: %s", buf);

    close(fd);
    printf("closed fd — storage released\n");
    return 0;
}
