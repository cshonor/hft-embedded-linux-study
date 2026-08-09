#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

/*
 * Minimal Ch45 lab: key via ftok, exclusive create, then IPC_RMID.
 * Needs a stable pathname (file or directory) for ftok.
 */
int main(int argc, char *argv[])
{
    const char *path;
    key_t key;
    int msqid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <ftok-pathname>\n", argv[0]);
        return 1;
    }
    path = argv[1];

    key = ftok(path, 'A');
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }
    printf("ftok(%s, 'A') -> key=0x%x\n", path, (unsigned)key);

    msqid = msgget(key, IPC_CREAT | IPC_EXCL | 0600);
    if (msqid == -1) {
        if (errno == EEXIST)
            fprintf(stderr, "msgget: already exists (try ipcrm or other key)\n");
        else
            perror("msgget");
        return 1;
    }
    printf("created msqid=%d\n", msqid);

    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
        return 1;
    }
    printf("removed msqid=%d (IPC_RMID)\n", msqid);
    return 0;
}
