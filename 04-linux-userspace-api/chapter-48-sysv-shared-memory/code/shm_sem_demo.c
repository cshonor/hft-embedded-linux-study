#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/* Layout in shared segment — no absolute pointers. */
struct slot {
    int ready; /* 0 empty, 1 filled (protected by sem) */
    char msg[64];
};

static int binary_sem_init(key_t key)
{
    int semid;
    union semun arg;

    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (semid >= 0) {
        arg.val = 1;
        if (semctl(semid, 0, SETVAL, arg) == -1) {
            perror("SETVAL");
            semctl(semid, 0, IPC_RMID);
            return -1;
        }
        return semid;
    }
    if (errno != EEXIST) {
        perror("semget");
        return -1;
    }
    return semget(key, 1, 0600);
}

static int sem_op1(int semid, short op)
{
    struct sembuf sb = { 0, op, SEM_UNDO };
    return semop(semid, &sb, 1);
}

int main(int argc, char *argv[])
{
    key_t key;
    int shmid, semid;
    struct slot *mem;
    pid_t pid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <ftok-pathname>\n", argv[0]);
        return 1;
    }

    key = ftok(argv[1], 'D');
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }

    shmid = shmget(key, sizeof(struct slot), IPC_CREAT | IPC_EXCL | 0600);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    semid = binary_sem_init(key);
    if (semid == -1) {
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    mem = shmat(shmid, NULL, 0);
    if (mem == (void *)-1) {
        perror("shmat");
        return 1;
    }
    mem->ready = 0;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* child: wait until ready, then read */
        for (;;) {
            if (sem_op1(semid, -1) == -1) {
                perror("child P");
                _exit(1);
            }
            if (mem->ready) {
                printf("child read: %s\n", mem->msg);
                mem->ready = 0;
                sem_op1(semid, 1);
                break;
            }
            sem_op1(semid, 1);
            usleep(1000);
        }
        shmdt(mem);
        _exit(0);
    }

    /* parent: write message */
    if (sem_op1(semid, -1) == -1) {
        perror("parent P");
        return 1;
    }
    strcpy(mem->msg, "hello-via-shm");
    mem->ready = 1;
    sem_op1(semid, 1);

    wait(NULL);

    /* Mark for deletion; freed after last detach. */
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    shmdt(mem);
    printf("parent: RMID marked, detached\n");
    return 0;
}
