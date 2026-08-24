#ifndef PARSER_H
#define PARSER_H

#define MAX_TOKENS 64
#define MAX_LINE   1024
#define MAX_CMDS   8

struct cmd {
    char *argv[MAX_TOKENS];
    int argc;
    char *in_file;  /* `< file`，没有则为 NULL */
    char *out_file; /* `> file` */
};

/* 把一行拆成管道级命令。会改写 line。返回命令个数，失败返回 -1。 */
int parse_pipeline(char *line, struct cmd *cmds, int max_cmds);

#endif
