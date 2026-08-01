/* Ch5: O_CREAT|O_EXCL is atomic create-or-fail. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    const char *path = "o_excl_demo.tmp";
    unlink(path);

    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0)
        die("first open O_EXCL");
    close(fd);
    printf("first create: ok\n");

    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd >= 0) {
        fprintf(stderr, "second create should have failed\n");
        close(fd);
        unlink(path);
        return 1;
    }
    if (errno != EEXIST)
        die("second open unexpected errno");
    printf("second create: EEXIST as expected\n");

    unlink(path);
    return 0;
}
