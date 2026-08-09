#include "parser.h"

#include <string.h>

int parse_line(char *line, char **argv, int max_argv)
{
    int argc = 0;
    char *tok = strtok(line, " \t\n");

    while (tok != NULL && argc < max_argv - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}
