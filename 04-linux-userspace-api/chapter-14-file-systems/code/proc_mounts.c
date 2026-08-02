/* List mounts from /proc/self/mounts (kernel mount table).
 * cc -Wall -Wextra -o proc_mounts proc_mounts.c && ./proc_mounts
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *fp;
    char line[512];
    int n = 0;

    fp = fopen("/proc/self/mounts", "r");
    if (fp == NULL) {
        perror("/proc/self/mounts");
        return EXIT_FAILURE;
    }

    printf("%-20s %-24s %-10s %s\n", "source", "target", "fstype", "opts...");
    while (fgets(line, sizeof(line), fp) != NULL) {
        char src[256], tgt[256], type[64], opts[256];
        int dump, pass;

        if (sscanf(line, "%255s %255s %63s %255s %d %d",
                   src, tgt, type, opts, &dump, &pass) < 4)
            continue;
        printf("%-20s %-24s %-10s %s\n", src, tgt, type, opts);
        n++;
    }
    fclose(fp);
    printf("(%d mounts)\n", n);
    return 0;
}
