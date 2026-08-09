/* Do not use system()/popen(); use absolute-path exec* instead.
 * cc -Wall -Wextra -o no_system_exec no_system_exec.c && ./no_system_exec
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid;
    int st;
    char *const argv[] = { "echo", "ok-via-execve", NULL };
    char *const envp[] = { "PATH=/usr/bin:/bin", NULL };

    fflush(NULL);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        /* Absolute path — not execlp/execvp (PATH hijack surface) */
        execve("/bin/echo", argv, envp);
        perror("execve");
        _exit(127);
    }
    if (waitpid(pid, &st, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    printf("child exit=%d\n", WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    return 0;
}
