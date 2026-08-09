/* Print st_dev / st_ino / st_nlink — same FS hard links share inode#.
 * cc -Wall -Wextra -o print_inode print_inode.c && ./print_inode PATH...
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s PATH...\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("%-24s %12s %12s %6s\n", "path", "dev", "ino", "nlink");
    for (i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }
        printf("%-24s %12llu %12llu %6lu\n",
               argv[i],
               (unsigned long long)st.st_dev,
               (unsigned long long)st.st_ino,
               (unsigned long)st.st_nlink);
    }
    return 0;
}
