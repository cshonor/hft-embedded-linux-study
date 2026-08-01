/* Ch5: pread uses offset without changing current file offset. */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    const char *path = "pread_demo.tmp";
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        die("open");
    if (write(fd, "0123456789", 10) != 10)
        die("write");
    if (lseek(fd, 0, SEEK_SET) < 0)
        die("lseek");

    char buf[4] = {0};
    if (pread(fd, buf, 3, 5) != 3)
        die("pread");

    off_t cur = lseek(fd, 0, SEEK_CUR);
    if (cur < 0)
        die("lseek CUR");

    printf("pread(offset=5) got \"%s\"; current offset still %lld (expect 0)\n",
           buf, (long long)cur);

    close(fd);
    unlink(path);
    return 0;
}
