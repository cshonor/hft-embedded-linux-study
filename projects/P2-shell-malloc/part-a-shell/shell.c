#include "builtin.h"
#include "executor.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[MAX_LINE];
    char *argv[MAX_TOKENS];

    for (;;) {
        fputs("mysh> ", stdout);
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            putchar('\n');
            break;
        }
        if (line[0] == '\n')
            continue;

        int argc = parse_line(line, argv, MAX_TOKENS);
        if (argc == 0)
            continue;

        if (try_builtin(argv, argc))
            continue;

        run_external(argv);
    }
    return 0;
}
