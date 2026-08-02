/* Demonstrate stdio buffer duplication across fork.
 * cc -Wall -Wextra -o fork_stdio_buf fork_stdio_buf.c
 * ./fork_stdio_buf           # often prints "before fork" twice when redirected
 * ./fork_stdio_buf fflush    # fix
 *
 * Tip: ./fork_stdio_buf > /tmp/out.txt  makes stdout fully buffered.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int do_fflush = (argc > 1 && strcmp(argv[1], "fflush") == 0);
    pid_t pid;

    /* No trailing newline — stays in user buffer when fully buffered */
    printf("before fork");
    if (do_fflush)
        fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf(" [child]\n");
        _exit(0);
    }

    printf(" [parent]\n");
    waitpid(pid, NULL, 0);
    return 0;
}
