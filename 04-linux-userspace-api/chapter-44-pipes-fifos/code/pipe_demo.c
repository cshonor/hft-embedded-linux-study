#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int fd[2];
    pid_t pid;
    char buf[64];
    ssize_t n;
    const char *msg = "hello-from-parent\n";

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(fd[1]); /* child: read end only */
        n = read(fd[0], buf, sizeof(buf) - 1);
        if (n < 0) {
            perror("read");
            _exit(1);
        }
        buf[n] = '\0';
        printf("child got: %s", buf);
        close(fd[0]);
        _exit(0);
    }

    close(fd[0]); /* parent: write end only */
    if (write(fd[1], msg, strlen(msg)) != (ssize_t)strlen(msg)) {
        perror("write");
        return 1;
    }
    close(fd[1]); /* EOF for child */
    wait(NULL);
    return 0;
}
