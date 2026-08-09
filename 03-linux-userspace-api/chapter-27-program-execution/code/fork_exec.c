/* Industrial pattern: fork → execvp → waitpid; child _exit on failure.
 * cc -Wall -Wextra -o fork_exec fork_exec.c
 * ./fork_exec [prog args...]     # default: /bin/echo Ch27-ok
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int st;
    char *default_argv[] = { "echo", "Ch27-ok", NULL };
    char **child_argv;
    const char *prog;

    if (argc > 1) {
        prog = argv[1];
        child_argv = &argv[1];
    } else {
        prog = "echo";
        child_argv = default_argv;
    }

    fflush(NULL);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* child: load new image in same PID */
        execvp(prog, child_argv);
        perror("execvp");
        _exit(127);
    }

    if (waitpid(pid, &st, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(st))
        printf("parent: child exit=%d (parent pid still %ld)\n",
               WEXITSTATUS(st), (long)getpid());
    else if (WIFSIGNALED(st))
        printf("parent: child killed by signal %d\n", WTERMSIG(st));
    return 0;
}
