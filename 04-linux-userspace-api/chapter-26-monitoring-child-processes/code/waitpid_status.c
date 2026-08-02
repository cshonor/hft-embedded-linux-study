/* waitpid + WIF* macros.
 * cc -Wall -Wextra -o waitpid_status waitpid_status.c
 * ./waitpid_status           # child _exit(42)
 * ./waitpid_status signal    # child raise(SIGTERM)
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_status(pid_t pid, int st)
{
    printf("reaped pid=%ld\n", (long)pid);
    if (WIFEXITED(st)) {
        printf("  WIFEXITED: exit status=%d\n", WEXITSTATUS(st));
    } else if (WIFSIGNALED(st)) {
        printf("  WIFSIGNALED: signal=%d", WTERMSIG(st));
#ifdef WCOREDUMP
        if (WCOREDUMP(st))
            printf(" (core dumped)");
#endif
        printf("\n");
    } else {
        printf("  other status raw=0x%x\n", (unsigned)st);
    }
}

int main(int argc, char *argv[])
{
    int by_signal = (argc > 1 && strcmp(argv[1], "signal") == 0);
    pid_t pid;
    int st;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (pid == 0) {
        if (by_signal)
            raise(SIGTERM);
        _exit(42);
    }

    if (waitpid(pid, &st, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    print_status(pid, st);
    return 0;
}
