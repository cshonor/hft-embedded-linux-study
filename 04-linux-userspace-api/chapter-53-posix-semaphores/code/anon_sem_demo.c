#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    sem_t *sem;
    pid_t pid;

    sem = mmap(NULL, sizeof(*sem), PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* pshared=1: process-shared; must live in shared memory */
    if (sem_init(sem, 1, 0) == -1) {
        perror("sem_init");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("child: waiting\n");
        if (sem_wait(sem) == -1) {
            perror("child wait");
            _exit(1);
        }
        printf("child: got post\n");
        munmap(sem, sizeof(*sem));
        _exit(0);
    }

    usleep(50000);
    printf("parent: posting\n");
    if (sem_post(sem) == -1) {
        perror("sem_post");
        return 1;
    }
    wait(NULL);
    sem_destroy(sem);
    munmap(sem, sizeof(*sem));
    return 0;
}
