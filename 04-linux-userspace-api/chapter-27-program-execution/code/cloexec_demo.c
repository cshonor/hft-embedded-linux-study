/* O_CLOEXEC: fd survives fork, closed across exec.
 * cc -Wall -Wextra -o cloexec_demo cloexec_demo.c && ./cloexec_demo
 *
 * Child execs a tiny helper via /bin/sh that checks whether fd 3 is open.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int run_check(int with_cloexec)
{
    int fd, st;
    pid_t pid;
    char *argv[] = {
        "sh", "-c",
        "if [ -e /proc/self/fd/3 ]; then echo 'fd3=OPEN'; else echo 'fd3=CLOSED'; fi",
        NULL
    };

    fd = open("/tmp/tlpi_cloexec_probe",
              O_CREAT | O_RDWR | O_TRUNC | (with_cloexec ? O_CLOEXEC : 0),
              0600);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    /* Force descriptor number 3 for the probe */
    if (fd != 3) {
        if (dup2(fd, 3) == -1) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    printf("%s O_CLOEXEC: ", with_cloexec ? "with" : "without");
    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        close(3);
        return -1;
    }
    if (pid == 0) {
        execv("/bin/sh", argv);
        _exit(127);
    }
    waitpid(pid, &st, 0);
    close(3);
    return 0;
}

int main(void)
{
    run_check(0);
    run_check(1);
    return 0;
}
