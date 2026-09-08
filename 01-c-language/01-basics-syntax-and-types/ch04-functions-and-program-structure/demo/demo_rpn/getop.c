#include <stdio.h>
#include <ctype.h>
#include "calc.h"

int getop(char buf[])
{
    int ch, idx = 0;

    while ((ch = getchar()) == ' ' || ch == '\t')
        ;

    if (!isdigit(ch)) {
        buf[0] = (char)ch;
        buf[1] = '\0';
        return ch;
    }

    buf[idx++] = (char)ch;
    while (isdigit(ch = getchar()))
        buf[idx++] = (char)ch;
    buf[idx] = '\0';
    ungetc(ch, stdin);
    return '0';
}
