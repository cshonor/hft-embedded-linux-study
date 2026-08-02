/* Process-group leader cannot setsid; child after fork can.
 * cc -Wall -Wextra -o setsid_demo setsid_demo.c && ./setsid_demo
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void show(const char *who)
{
    printf("%s: pid=%ld pgid=%ld sid=%ld\n",
           who,
           (long)getpid(),
           (long)getpgrp(),
           (long)getsid(0));
}

int main(void)
{
    pid_t pid;

    show("before");
    /* If we are already a session/group leader (common in some shells),
     * setsid fails — that is the point of forking first. */
    if (setsid() == -1)
        printf("setsid in current process: %s (often EPERM if already leader)\n",
               strerror(errno));
    else
        show("setsid-ok-unexpected");

    fflush(stdout);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (setsid() == -1) {
            perror("child setsid");
            _exit(1);
        }
        show("child-after-setsid");
        /* New session: no controlling terminal typically */
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    show("parent-unchanged");
    return 0;
}
