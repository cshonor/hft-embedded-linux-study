#include "builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int try_builtin(char **argv, int argc)
{
    (void)argc;
    if (argv[0] == NULL)
        return 1;

    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = (argv[1] != NULL) ? argv[1] : getenv("HOME");
        if (dir == NULL || chdir(dir) != 0)
            perror("cd");
        return 1;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char buf[4096];
        if (getcwd(buf, sizeof buf) != NULL)
            puts(buf);
        else
            perror("pwd");
        return 1;
    }
    return 0;
}
