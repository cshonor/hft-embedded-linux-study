#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define QNAME "/ch52_demo"

int main(void)
{
    mqd_t mq;
    struct mq_attr attr, gat;
    char buf[128];
    unsigned prio;
    ssize_t n;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 8;
    attr.mq_msgsize = sizeof(buf);
    attr.mq_curmsgs = 0;

    mq = mq_open(QNAME, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (mq == (mqd_t)-1) {
        if (errno == EEXIST) {
            mq_unlink(QNAME);
            mq = mq_open(QNAME, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
        }
        if (mq == (mqd_t)-1) {
            perror("mq_open");
            return 1;
        }
    }

    if (mq_send(mq, "low", 4, 1) == -1) {
        perror("mq_send low");
        return 1;
    }
    if (mq_send(mq, "high", 5, 10) == -1) {
        perror("mq_send high");
        return 1;
    }

    if (mq_getattr(mq, &gat) == 0)
        printf("curmsgs=%ld maxmsg=%ld msgsize=%ld\n",
               gat.mq_curmsgs, gat.mq_maxmsg, gat.mq_msgsize);

    /* Always receives highest priority first (not SysV mtype filter). */
    n = mq_receive(mq, buf, sizeof(buf), &prio);
    if (n == -1) {
        perror("mq_receive");
        return 1;
    }
    buf[n] = '\0';
    printf("first recv: prio=%u text=%s\n", prio, buf);

    n = mq_receive(mq, buf, sizeof(buf), &prio);
    if (n == -1) {
        perror("mq_receive");
        return 1;
    }
    buf[n] = '\0';
    printf("second recv: prio=%u text=%s\n", prio, buf);

    mq_close(mq);
    mq_unlink(QNAME);
    printf("closed+unlinked\n");
    return 0;
}
