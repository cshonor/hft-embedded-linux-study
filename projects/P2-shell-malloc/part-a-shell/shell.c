#include "builtin.h"
#include "executor.h"
#include "parser.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

static void on_sigchld(int sig)
{
    (void)sig;
    /* 只收割、不处理逻辑：handler 里只能调 async-signal-safe 的函数。 */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(void)
{
    char line[MAX_LINE];
    struct cmd cmds[MAX_CMDS];

    /* Ctrl-C 杀掉前台作业，不要把 shell 一起带走。 */
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, on_sigchld);

    for (;;) {
        fputs("mysh> ", stdout);
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            putchar('\n');
            break;
        }
        if (line[0] == '\n')
            continue;

        int bg = 0;
        int n = parse_pipeline(line, cmds, MAX_CMDS, &bg);
        if (n <= 0)
            continue;

        if (n == 1 && try_builtin(cmds[0].argv, cmds[0].argc))
            continue;

        run_pipeline(cmds, n, bg);
    }
    return 0;
}
