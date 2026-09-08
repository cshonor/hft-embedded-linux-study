/* T7c: -Wformat-security, %n, and fortify */
#include <stdio.h>

int main(int argc, char **argv)
{
    int n = 0;
    (void)argc;
    printf(argv[0]);        /* A: non-literal format, no args -> -Wformat-security */
    printf(argv[0], 1);     /* B: non-literal format, has args -> -Wformat-nonliteral */
    printf("%n\n", &n);     /* C: %n in a literal format                            */
    printf("%s\n");         /* D: missing arg -> -Wformat                            */
    return n;
}
