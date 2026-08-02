#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define NAME "/ch51_posix_demo"

int main(void)
{
    int fd;

    fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "exists; try: shm_unlink(\"%s\") or rm /dev/shm%s\n",
                    NAME, NAME);
            shm_unlink(NAME);
            fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
        }
        if (fd == -1) {
            perror("shm_open");
            return 1;
        }
    }

    printf("opened %s -> fd=%d (POSIX IPC name; fd-style handle)\n", NAME, fd);

    /* unlink removes the name; object lives until last close */
    if (shm_unlink(NAME) == -1) {
        perror("shm_unlink");
        close(fd);
        return 1;
    }
    printf("unlinked name; object still valid until close\n");

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }
    printf("closed: object destroyed (no remaining refs)\n");
    return 0;
}
