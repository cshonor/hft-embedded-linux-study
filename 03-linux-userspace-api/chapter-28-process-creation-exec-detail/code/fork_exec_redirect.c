/* fork → redirect stdout to file → execvp; failure uses _exit.
 * cc -Wall -Wextra -o fork_exec_redirect fork_exec_redirect.c
 * ./fork_exec_redirect [/tmp/tlpi_exec_out.txt]
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *out = (argc > 1) ? argv[1] : "/tmp/tlpi_exec_out.txt";
    char *child_argv[] = { "echo", "hello-from-exec", NULL };
    pid_t pid;
    int fd, st;

    fflush(NULL);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        fd = open(out, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644);
        if (fd == -1) {
            perror("open");
            _exit(127);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(127);
        }
        /* fd has O_CLOEXEC; STDOUT keep open for the new image */
        close(fd);

        execvp(child_argv[0], child_argv);
        perror("execvp");
        _exit(127);
    }

    if (waitpid(pid, &st, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (WIFEXITED(st))
        printf("parent: child exit=%d, output file=%s\n",
               WEXITSTATUS(st), out);
    return 0;
}
