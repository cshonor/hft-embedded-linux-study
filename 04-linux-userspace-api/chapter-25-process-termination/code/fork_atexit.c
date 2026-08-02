/* Child calling exit() re-runs inherited atexit list; _exit() does not.
 * cc -Wall -Wextra -o fork_atexit fork_atexit.c
 * ./fork_atexit exit
 * ./fork_atexit _exit
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void bye(void)
{
    printf("atexit bye (pid=%ld)\n", (long)getpid());
}

int main(int argc, char *argv[])
{
    int child_use_exit = 1;
    pid_t pid;

    if (argc > 1 && strcmp(argv[1], "_exit") == 0)
        child_use_exit = 0;

    atexit(bye);
    printf("parent pid=%ld registering atexit, forking...\n", (long)getpid());
    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("child terminating via %s\n",
               child_use_exit ? "exit" : "_exit");
        fflush(stdout);
        if (child_use_exit)
            exit(0);
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    printf("parent exiting via exit()\n");
    return 0; /* triggers atexit in parent once */
}
