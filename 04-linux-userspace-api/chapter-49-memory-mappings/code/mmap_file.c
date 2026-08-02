#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *path;
    int fd;
    char *p;
    pid_t pid;
    const size_t len = 4096;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <path>\n", argv[0]);
        return 1;
    }
    path = argv[1];

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    if (ftruncate(fd, (off_t)len) == -1) {
        perror("ftruncate");
        return 1;
    }

    p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd); /* mapping keeps reference */

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* wait briefly for parent write (demo only) */
        usleep(50000);
        printf("child mapped: %s\n", p);
        munmap(p, len);
        _exit(0);
    }

    strcpy(p, "hello-mmap-file");
    if (msync(p, len, MS_SYNC) == -1)
        perror("msync");

    wait(NULL);
    munmap(p, len);
    return 0;
}
