/* POSIX timer: CLOCK_MONOTONIC + SIGEV_THREAD callback (no signal handler).
 * cc -Wall -Wextra -o posix_timer_thread posix_timer_thread.c -lrt
 * ./posix_timer_thread
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile int ticks;

static void timer_cb(union sigval sv)
{
    int *p = sv.sival_ptr;
    (*p)++;
    /* ordinary thread context — printf OK */
    printf("tick %d\n", *p);
}

int main(void)
{
    timer_t tid;
    struct sigevent sev;
    struct itimerspec its;
    int i;

    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_cb;
    sev.sigev_value.sival_ptr = (void *)&ticks;

    if (timer_create(CLOCK_MONOTONIC, &sev, &tid) == -1) {
        perror("timer_create");
        return EXIT_FAILURE;
    }

    /* first fire after 200ms, then every 200ms */
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 200000000L;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 200000000L;

    if (timer_settime(tid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        timer_delete(tid);
        return EXIT_FAILURE;
    }

    for (i = 0; i < 50 && ticks < 5; i++) {
        struct timespec pause = { .tv_sec = 0, .tv_nsec = 100000000L };
        nanosleep(&pause, NULL);
    }

    timer_delete(tid);
    printf("total ticks=%d\n", ticks);
    return 0;
}
