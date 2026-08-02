#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

struct mymsg {
    long mtype;
    char mtext[64];
};

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    key_t key;
    int msqid;
    struct mymsg out, in;
    ssize_t n;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <ftok-pathname>\n", argv[0]);
        return 1;
    }

    key = ftok(argv[1], 'B');
    if (key == (key_t)-1)
        die("ftok");

    msqid = msgget(key, IPC_CREAT | IPC_EXCL | 0600);
    if (msqid == -1)
        die("msgget");

    out.mtype = 100;
    strcpy(out.mtext, "type-100");
    if (msgsnd(msqid, &out, strlen(out.mtext) + 1, 0) == -1)
        die("msgsnd 100");

    out.mtype = 300;
    strcpy(out.mtext, "type-300");
    if (msgsnd(msqid, &out, strlen(out.mtext) + 1, 0) == -1)
        die("msgsnd 300");

    out.mtype = 200;
    strcpy(out.mtext, "type-200");
    if (msgsnd(msqid, &out, strlen(out.mtext) + 1, 0) == -1)
        die("msgsnd 200");

    /* msgtyp < 0: smallest type among those <= |msgtyp| */
    memset(&in, 0, sizeof(in));
    n = msgrcv(msqid, &in, sizeof(in.mtext), -250, 0);
    if (n == -1)
        die("msgrcv");
    printf("recv mtype=%ld text=%s (nbytes=%zd)\n", in.mtype, in.mtext, n);

    if (msgctl(msqid, IPC_RMID, NULL) == -1)
        die("msgctl IPC_RMID");
    printf("queue removed\n");
    return 0;
}
