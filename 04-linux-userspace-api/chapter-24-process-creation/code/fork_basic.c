/* fork returns twice; COW makes globals independent after write.
 * cc -Wall -Wextra -o fork_basic fork_basic.c && ./fork_basic
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int g = 100;

int main(void)
{
    pid_t pid;

    printf("before fork: pid=%ld g=%d\n", (long)getpid(), g);
    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        g = 200;
        printf("child:  pid=%ld ppid=%ld fork_ret=0 g=%d\n",
               (long)getpid(), (long)getppid(), g);
        _exit(0);
    }

    /* parent */
    printf("parent: pid=%ld child=%ld g=%d (still)\n",
           (long)getpid(), (long)pid, g);
    if (waitpid(pid, NULL, 0) == -1)
        perror("waitpid");
    printf("parent after wait: g=%d\n", g);
    return 0;
}
