#include "builtin.h"
#include "executor.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[MAX_LINE];
    struct cmd cmds[MAX_CMDS];

    for (;;) {
        fputs("mysh> ", stdout);
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            putchar('\n');
            break;
        }
        if (line[0] == '\n')
            continue;

        int n = parse_pipeline(line, cmds, MAX_CMDS);
        if (n <= 0)
            continue;

        if (n == 1 && try_builtin(cmds[0].argv, cmds[0].argc))
            continue;

        run_pipeline(cmds, n);
    }
    return 0;
}
