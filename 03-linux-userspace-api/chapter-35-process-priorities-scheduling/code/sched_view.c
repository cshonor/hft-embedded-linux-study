/* Show scheduler policy; optionally try SCHED_FIFO (needs privilege).
 * cc -Wall -Wextra -o sched_view sched_view.c
 * ./sched_view
 * sudo ./sched_view fifo 10
 */
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *policy_name(int p)
{
    switch (p) {
    case SCHED_OTHER: return "SCHED_OTHER";
    case SCHED_FIFO:  return "SCHED_FIFO";
    case SCHED_RR:    return "SCHED_RR";
#ifdef SCHED_BATCH
    case SCHED_BATCH: return "SCHED_BATCH";
#endif
#ifdef SCHED_IDLE
    case SCHED_IDLE:  return "SCHED_IDLE";
#endif
    default:          return "?";
    }
}

static void show(void)
{
    int pol;
    struct sched_param sp;

    pol = sched_getscheduler(0);
    if (pol == -1) {
        perror("sched_getscheduler");
        return;
    }
    if (sched_getparam(0, &sp) == -1) {
        perror("sched_getparam");
        return;
    }
    printf("pid=%ld policy=%s(%d) priority=%d\n",
           (long)getpid(), policy_name(pol), pol, sp.sched_priority);

    if (pol == SCHED_RR) {
        struct timespec ts;
        if (sched_rr_get_interval(0, &ts) == 0)
            printf("RR quantum: %ld.%09ld s\n",
                   (long)ts.tv_sec, ts.tv_nsec);
    }
}

int main(int argc, char *argv[])
{
    show();

    if (argc >= 2 && strcmp(argv[1], "fifo") == 0) {
        struct sched_param sp;
        int prio = (argc >= 3) ? atoi(argv[2]) : 10;

        memset(&sp, 0, sizeof(sp));
        sp.sched_priority = prio;
        if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {
            fprintf(stderr, "sched_setscheduler(FIFO): %s\n", strerror(errno));
            fprintf(stderr, "need CAP_SYS_NICE / root, and valid RT priority\n");
            return 1;
        }
        printf("switched to SCHED_FIFO\n");
        show();
        /* drop back so we don't leave a RT shell leftover if run interactively */
        sp.sched_priority = 0;
        sched_setscheduler(0, SCHED_OTHER, &sp);
        printf("restored SCHED_OTHER\n");
    }
    return 0;
}
