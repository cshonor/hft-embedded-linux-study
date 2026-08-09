/* Print pid / pgid / sid for self (and optional child after fork).
 * cc -Wall -Wextra -o print_ids print_ids.c && ./print_ids
 */
#include <stdio.h>
#include <stdlib.h>
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

    show("parent");
    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        show("child ");
        _exit(0);
    }
    waitpid(pid, NULL, 0);
    return 0;
}
