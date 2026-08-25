#include "parser.h"

#include <string.h>

/* 热路径：空格/成功读 token 几乎每次都有。EOF、行太长极少。 */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static char *skip_ws(char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    return s;
}

static int parse_stage(char *stage, struct cmd *c)
{
    memset(c, 0, sizeof *c);
    char *s = skip_ws(stage);
    while (likely(*s != '\0')) {
        if (*s == '<' || *s == '>') {
            int is_in = (*s == '<');
            s = skip_ws(s + 1);
            if (unlikely(*s == '\0'))
                return -1;
            char *start = s;
            while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '<' && *s != '>')
                s++;
            if (*s)
                *s++ = '\0';
            if (is_in)
                c->in_file = start;
            else
                c->out_file = start;
            s = skip_ws(s);
            continue;
        }
        if (unlikely(c->argc >= MAX_TOKENS - 1))
            return -1;
        c->argv[c->argc++] = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '<' && *s != '>')
            s++;
        if (*s) {
            char *end = s;
            s = skip_ws(s);
            *end = '\0';
        }
    }
    c->argv[c->argc] = NULL;
    return 0;
}

int parse_pipeline(char *line, struct cmd *cmds, int max_cmds, int *background)
{
    if (background)
        *background = 0;

    size_t nch = strlen(line);
    while (nch > 0 && (line[nch - 1] == ' ' || line[nch - 1] == '\t' || line[nch - 1] == '\n'))
        line[--nch] = '\0';
    if (nch > 0 && line[nch - 1] == '&' && (nch == 1 || line[nch - 2] == ' ' || line[nch - 2] == '\t')) {
        if (background)
            *background = 1;
        line[--nch] = '\0';
        while (nch > 0 && (line[nch - 1] == ' ' || line[nch - 1] == '\t'))
            line[--nch] = '\0';
    }

    int n = 0;
    char *start = line;
    for (char *p = line;; p++) {
        if (*p != '|' && *p != '\0')
            continue;
        int at_end = (*p == '\0');
        *p = '\0';
        if (n >= max_cmds)
            return -1;
        if (parse_stage(start, &cmds[n]) < 0)
            return -1;
        if (cmds[n].argc == 0)
            return -1;
        n++;
        if (at_end)
            break;
        start = p + 1;
    }
    return n;
}
