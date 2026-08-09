#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define NAME "/ch53_named"

int main(void)
{
    sem_t *sem;
    int val;

    sem = sem_open(NAME, O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            sem_unlink(NAME);
            sem = sem_open(NAME, O_CREAT | O_EXCL, 0600, 1);
        }
        if (sem == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }
    }

    if (sem_wait(sem) == -1) {
        perror("sem_wait");
        return 1;
    }
    printf("in critical section\n");
    if (sem_post(sem) == -1) {
        perror("sem_post");
        return 1;
    }

    if (sem_getvalue(sem, &val) == 0)
        printf("getvalue snapshot=%d\n", val);

    sem_close(sem);
    sem_unlink(NAME);
    printf("closed+unlinked\n");
    return 0;
}
