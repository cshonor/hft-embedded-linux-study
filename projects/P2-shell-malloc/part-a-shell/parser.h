#ifndef PARSER_H
#define PARSER_H

#define MAX_TOKENS 64
#define MAX_LINE   1024

/* Split line into argv-style tokens. Returns argc. Mutates line. */
int parse_line(char *line, char **argv, int max_argv);

#endif
