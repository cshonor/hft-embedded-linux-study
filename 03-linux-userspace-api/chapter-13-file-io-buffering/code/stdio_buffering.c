/* Show stdout buffering: terminal (line) vs redirected file (full).
 *   cc -Wall -Wextra -o stdio_buffering stdio_buffering.c
 *   ./stdio_buffering                 # often line-buffered on TTY
 *   ./stdio_buffering > /tmp/out.txt; cat /tmp/out.txt
 *   ./stdio_buffering fflush          # force flush each line
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int do_fflush = (argc > 1 && strcmp(argv[1], "fflush") == 0);

    printf("isatty(stdout)=%d\n", isatty(STDOUT_FILENO));
    for (int i = 0; i < 3; i++) {
        printf("line %d (no nl yet)...", i);
        if (do_fflush)
            fflush(stdout);
        sleep(1);
        printf(" done\n");
        if (do_fflush)
            fflush(stdout);
    }
    return 0;
}
