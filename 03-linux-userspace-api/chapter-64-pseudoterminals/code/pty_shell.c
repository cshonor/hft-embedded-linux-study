#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int mfd, sfd;
    char *slave;
    pid_t pid;
    char buf[256];
    ssize_t n;

    mfd = posix_openpt(O_RDWR | O_NOCTTY);
    if (mfd == -1) {
        perror("posix_openpt");
        return 1;
    }
    if (grantpt(mfd) == -1 || unlockpt(mfd) == -1) {
        perror("grantpt/unlockpt");
        return 1;
    }
    slave = ptsname(mfd);
    if (slave == NULL) {
        perror("ptsname");
        return 1;
    }
    printf("slave=%s\n", slave);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (setsid() == -1) {
            perror("setsid");
            _exit(1);
        }
        sfd = open(slave, O_RDWR);
        if (sfd == -1) {
            perror("open slave");
            _exit(1);
        }
#ifdef TIOCSCTTY
        ioctl(sfd, TIOCSCTTY, 0);
#endif
        close(mfd);
        dup2(sfd, STDIN_FILENO);
        dup2(sfd, STDOUT_FILENO);
        dup2(sfd, STDERR_FILENO);
        if (sfd > STDERR_FILENO)
            close(sfd);
        execlp("sh", "sh", "-c", "tty; echo hello-from-pty", (char *)NULL);
        perror("exec");
        _exit(1);
    }

    /* parent: keep master, drain output from child shell */
    while ((n = read(mfd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        fputs(buf, stdout);
    }
    close(mfd);
    wait(NULL);
    return 0;
}
