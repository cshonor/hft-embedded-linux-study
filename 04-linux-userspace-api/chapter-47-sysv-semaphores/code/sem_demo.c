#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>

/* Not defined by some libc headers — TLPI requires app to declare it. */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int open_binary_sem(key_t key)
{
    int semid;
    union semun arg;

    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (semid >= 0) {
        arg.val = 1;
        if (semctl(semid, 0, SETVAL, arg) == -1) {
            perror("semctl SETVAL");
            semctl(semid, 0, IPC_RMID);
            return -1;
        }
        printf("created+init semid=%d val=1\n", semid);
        return semid;
    }
    if (errno != EEXIST) {
        perror("semget create");
        return -1;
    }
    semid = semget(key, 1, 0600);
    if (semid == -1) {
        perror("semget open");
        return -1;
    }
    printf("opened existing semid=%d\n", semid);
    return semid;
}

static int sem_p(int semid)
{
    struct sembuf op = { 0, -1, SEM_UNDO };
    return semop(semid, &op, 1);
}

static int sem_v(int semid)
{
    struct sembuf op = { 0, 1, SEM_UNDO };
    return semop(semid, &op, 1);
}

int main(int argc, char *argv[])
{
    key_t key;
    int semid;
    union semun arg;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <ftok-pathname>\n", argv[0]);
        return 1;
    }

    key = ftok(argv[1], 'C');
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }

    semid = open_binary_sem(key);
    if (semid == -1)
        return 1;

    if (sem_p(semid) == -1) {
        perror("P");
        return 1;
    }
    printf("pid=%d in critical section\n", (int)getpid());
    if (sem_v(semid) == -1) {
        perror("V");
        return 1;
    }

    arg.val = 0;
    printf("GETVAL=%d\n", semctl(semid, 0, GETVAL, arg));

    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("IPC_RMID");
        return 1;
    }
    printf("semaphore set removed\n");
    return 0;
}
