#include <ctype.h>
#include <stdio.h>

/*
 * argv strings are typically mutable process memory, but NOT malloc'd by you.
 * Changing content can be used for experiments; never free(argv[i]).
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <word>\n", argv[0]);
        return 1;
    }

    printf("before: argv[1]=%s\n", argv[1]);
    if (argv[1][0] != '\0')
        argv[1][0] = (char)toupper((unsigned char)argv[1][0]);
    printf("after:  argv[1]=%s\n", argv[1]);

    /* Do NOT: free(argv[0]); free(argv[1]); */
    return 0;
}
