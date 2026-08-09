/* Parent and child share the same open-file offset after fork.
 * cc -Wall -Wextra -o fork_fd_offset fork_fd_offset.c
 * ./fork_fd_offset [/tmp/tlpi_fork_fd.txt]
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/tmp/tlpi_fork_fd.txt";
    int fd;
    pid_t pid;

    fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (write(fd, "AAAA", 4) != 4) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    fflush(NULL);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        close(fd);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Continues at offset 4 — same open file description */
        if (write(fd, "CCCC", 4) != 4)
            _exit(1);
        close(fd);
        _exit(0);
    }

    if (write(fd, "BBBB", 4) != 4)
        perror("write parent");
    waitpid(pid, NULL, 0);
    close(fd);

    printf("wrote to %s — expect mixed AAAA + BBBB/CCCC (order races)\n", path);
    return 0;
}
