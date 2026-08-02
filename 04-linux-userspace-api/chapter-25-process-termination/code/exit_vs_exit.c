/* exit() flushes stdio; _exit() does not.
 * cc -Wall -Wextra -o exit_vs_exit exit_vs_exit.c
 * ./exit_vs_exit exit
 * ./exit_vs_exit _exit > /tmp/out.txt ; cat /tmp/out.txt
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int use_exit = 1;

    if (argc > 1 && strcmp(argv[1], "_exit") == 0)
        use_exit = 0;

    /* No newline: stays in fully-buffered stdout when redirected */
    printf("hello from %s", use_exit ? "exit" : "_exit");

    if (use_exit)
        exit(0);
    _exit(0);
}
