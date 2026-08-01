/* Ch5: dup shares open-file description (and thus file offset). */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    const char *path = "dup_demo.tmp";
    int fd1 = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd1 < 0)
        die("open");

    if (write(fd1, "ABCDEFGH", 8) != 8)
        die("write");
    if (lseek(fd1, 0, SEEK_SET) < 0)
        die("lseek");

    int fd2 = dup(fd1);
    if (fd2 < 0)
        die("dup");

    char a, b;
    if (read(fd1, &a, 1) != 1)
        die("read fd1");
    if (read(fd2, &b, 1) != 1)
        die("read fd2");

    printf("after dup: fd1 read '%c', fd2 read '%c' (shared offset -> consecutive)\n", a, b);
    printf("expected: 'A' then 'B'\n");

    close(fd1);
    close(fd2);
    unlink(path);
    return 0;
}
