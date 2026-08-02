#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define NAME "/ch54_demo"
#define LEN  4096

int main(void)
{
    int fd;
    char *p;
    pid_t pid;

    fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        if (errno == EEXIST) {
            shm_unlink(NAME);
            fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
        }
        if (fd == -1) {
            perror("shm_open");
            return 1;
        }
    }

    /* New object size is 0 — must grow before MAP_SHARED access. */
    if (ftruncate(fd, LEN) == -1) {
        perror("ftruncate");
        return 1;
    }

    p = mmap(NULL, LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        usleep(50000);
        printf("child sees: %s\n", p);
        munmap(p, LEN);
        _exit(0);
    }

    strcpy(p, "hello-posix-shm");
    wait(NULL);

    munmap(p, LEN);
    shm_unlink(NAME);
    printf("parent: unmapped+unlinked\n");
    return 0;
}
