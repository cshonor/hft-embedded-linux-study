/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2026.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* cpu_burner.c

   Usage: cpu_burner [period]
                     (def=1)

   This program consumes CPU time, displaying the rate of CPU
   consumption during each 'period'.

   For some experiments, it is useful to confine multiple burner processes to
   the same CPU, using taskset(1). For example:

        taskset 0x1 ./cpu_burner

   See also cpu_multi_burner.c and cpu_multithread_burner.c.
*/
#define _GNU_SOURCE
#include <time.h>
#include <fcntl.h>
#include "tlpi_hdr.h"

#define NANO 1000000000L

static long
timespecDiff(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) * NANO + b.tv_nsec - a.tv_nsec;
}

/* Return pointer to a statically allocated buffer containingg the pathname
   of the cgroup of which this process is a member. */

static
const char *procCgroup(pid_t pid)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%ld/cgroup", (long)pid);

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        errExit("open: %s", path);

    static char buf[4096];
    ssize_t nread = read(fd, buf, (sizeof(buf) - 1));
    if (nread < 0)
        errExit("read: %s", path);

    close(fd);

    buf[nread - 1] = '\0';      // Replace trailing \n with '\0'

    /* The cgroup pathname is in the third column-delimited field */

    char *p = buf;
    while (*p != ':')
        p++;
    p++;
    while (*p != ':')
        p++;

    return p + 1;
}

static void
burnCPU(float period, int sleepPercent, bool showCgroup)
{
    long step_size = NANO * period;
    const int RUN_UNIT_NSEC = NANO / 100;       // 10ms
    long prev_step = 0;

    struct timespec intvl = {
        .tv_sec = 0,
        .tv_nsec = RUN_UNIT_NSEC * sleepPercent / 100
    };

    long run_length_nsec = RUN_UNIT_NSEC * (100 - sleepPercent) / 100;

    struct timespec base_real;
    if (clock_gettime(CLOCK_REALTIME, &base_real) == -1)
        errExit("clock_gettime");

    struct timespec prev_real = base_real;

    struct timespec prev_cpu;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &prev_cpu) == -1)
        errExit("clock_gettime");

    /* CPU burning is performed in intervals of RUN_UNIT_NSEC. During each
       interval, we may sleep for a certain percentage of the interval if
       sleepPercent is greater than zero. The algorithm here is not perfect:
       small oversleeps mean that in practice we may get up to around 1% less
       CPU time than we were aiming for. However, in the context of the
       experiments this feature is intended for (mainly: monitoring per-cgroup
       pressure stall information in cpu.pressure), this is not significant. */

    while (1) {
        if (sleepPercent > 0)
            nanosleep(&intvl, NULL);

        struct timespec curr_real;
        if (clock_gettime(CLOCK_REALTIME, &curr_real) == -1)
            errExit("clock_gettime");

        struct timespec run_start = curr_real;

        long elapsed_real_nsec, elapsed_real_steps;
        long run_time_nsec;

        do {
            if (clock_gettime(CLOCK_REALTIME, &curr_real) == -1)
                errExit("clock_gettime");
            run_time_nsec = timespecDiff(run_start, curr_real);
            elapsed_real_nsec = timespecDiff(base_real, curr_real);
            elapsed_real_steps = elapsed_real_nsec / step_size;
        } while (!(elapsed_real_steps > prev_step) &&
                (run_time_nsec < run_length_nsec)) ;

        if (elapsed_real_steps > prev_step) {
            struct timespec curr_cpu;
            if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &curr_cpu) == -1)
                errExit("clock_gettime");

            long diff_real_nsec = timespecDiff(prev_real, curr_real);
            long diff_cpu_nsec = timespecDiff(prev_cpu, curr_cpu);

            printf("%ld [t=%.2f] %%CPU: %5.2f", (long) getpid(),
                    (double) elapsed_real_nsec / NANO,
                    (double) diff_cpu_nsec / diff_real_nsec * 100);

            if (showCgroup)
                printf("  %s", procCgroup(getpid()));

            printf("\n");

            prev_cpu = curr_cpu;
            prev_real = curr_real;
            prev_step = elapsed_real_steps;
        }
    }
}

int
main(int argc, char *argv[])
{

    int opt;
    int sleepPercent = 0;
    bool showCgroup = false;
    while ((opt = getopt(argc, argv, "ci:")) != -1) {
        switch (opt) {
        case 'c':
            showCgroup = true;
            break;
        case 'i':
            sleepPercent = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr,
                    "Usage: %s [-c] [-i <idle-percent> [period]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (sleepPercent < 0 || sleepPercent > 99)
        fatal("idle-percent must be in the 0-99");

    float period = 1.0;

    if (argc > optind)
        sscanf(argv[optind], "%f", &period);

    burnCPU(period, sleepPercent, showCgroup);

    exit(EXIT_SUCCESS);
}
