/* kill(pid, 0): existence / permission probe, no signal delivered.
 * cc -Wall -Wextra -o kill_probe kill_probe.c
 * ./kill_probe PID
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    pid_t pid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PID\n", argv[0]);
        return EXIT_FAILURE;
    }
    pid = (pid_t)atol(argv[1]);

    if (kill(pid, 0) == 0) {
        printf("pid %ld: exists and we may signal it\n", (long)pid);
        return 0;
    }

    if (errno == ESRCH)
        printf("pid %ld: no such process (ESRCH)\n", (long)pid);
    else if (errno == EPERM)
        printf("pid %ld: exists but permission denied (EPERM)\n", (long)pid);
    else
        printf("pid %ld: kill(0) failed: %s\n", (long)pid, strerror(errno));
    return 1;
}
