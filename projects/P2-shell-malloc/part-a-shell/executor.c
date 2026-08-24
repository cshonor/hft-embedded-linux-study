#include "executor.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void apply_redir(const struct cmd *c)
{
    if (c->in_file) {
        int fd = open(c->in_file, O_RDONLY);
        if (fd < 0) {
            perror(c->in_file);
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (c->out_file) {
        int fd = open(c->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror(c->out_file);
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

static void exec_one(const struct cmd *c)
{
    apply_redir(c);
    execvp(c->argv[0], c->argv);
    perror(c->argv[0]);
    _exit(127);
}

void run_pipeline(struct cmd *cmds, int n)
{
    if (n <= 0)
        return;

    int prev_read = -1;
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < n; i++) {
        int pipefd[2] = {-1, -1};
        if (i != n - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }
        if (pid == 0) {
            if (prev_read >= 0) {
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (pipefd[1] >= 0) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                close(pipefd[0]);
            }
            exec_one(&cmds[i]);
        }
        pids[i] = pid;
        if (prev_read >= 0)
            close(prev_read);
        if (pipefd[1] >= 0)
            close(pipefd[1]);
        prev_read = pipefd[0];
    }

    for (int i = 0; i < n; i++) {
        if (waitpid(pids[i], NULL, 0) < 0)
            perror("waitpid");
    }
}
