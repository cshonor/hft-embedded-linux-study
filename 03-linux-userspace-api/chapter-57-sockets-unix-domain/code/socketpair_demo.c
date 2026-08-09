#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int sv[2];
    pid_t pid;
    char buf[64];
    ssize_t n;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(sv[0]);
        n = read(sv[1], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("child got: %s\n", buf);
            write(sv[1], "pong", 4);
        }
        close(sv[1]);
        _exit(0);
    }

    close(sv[1]);
    write(sv[0], "ping", 4);
    n = read(sv[0], buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("parent got: %s\n", buf);
    }
    close(sv[0]);
    wait(NULL);
    return 0;
}
