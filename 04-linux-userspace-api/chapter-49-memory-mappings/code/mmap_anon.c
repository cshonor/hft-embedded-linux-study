#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    char *p;
    pid_t pid;

    p = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    strcpy(p, "from-parent");

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("child sees: %s\n", p);
        strcpy(p, "from-child");
        munmap(p, 4096);
        _exit(0);
    }

    wait(NULL);
    printf("parent after child: %s\n", p);
    munmap(p, 4096);
    return 0;
}
