/* Demonstrate printf (stdio) + write(fd) reordering; fix with fflush.
 *   cc -Wall -Wextra -o mix_stdio_write mix_stdio_write.c
 *   ./mix_stdio_write            # often: WRITE then PRINTF (wrong order)
 *   ./mix_stdio_write fix        # fflush before write → correct order
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fix = (argc > 1 && strcmp(argv[1], "fix") == 0);

    printf("via printf (stdio buffer)");
    if (fix)
        fflush(stdout);

    const char *msg = " via write(2)\n";
    if (write(STDOUT_FILENO, msg, strlen(msg)) == -1) {
        perror("write");
        return 1;
    }

    printf(" [printf after write]\n");
    return 0;
}
