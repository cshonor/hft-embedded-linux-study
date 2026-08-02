/* Parse a few fields from /proc/self/status.
 * cc -Wall -Wextra -o proc_self_status proc_self_status.c && ./proc_self_status
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_if_prefix(const char *line, const char *key)
{
    size_t n = strlen(key);
    if (strncmp(line, key, n) == 0)
        fputs(line, stdout);
}

int main(void)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) {
        perror("fopen /proc/self/status");
        return EXIT_FAILURE;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        print_if_prefix(line, "Name:");
        print_if_prefix(line, "Pid:");
        print_if_prefix(line, "PPid:");
        print_if_prefix(line, "Uid:");
        print_if_prefix(line, "Gid:");
        print_if_prefix(line, "VmSize:");
        print_if_prefix(line, "VmRSS:");
        print_if_prefix(line, "Threads:");
    }

    if (ferror(fp))
        perror("fgets");
    fclose(fp);

    /* cmdline: NUL-separated */
    fp = fopen("/proc/self/cmdline", "r");
    if (fp) {
        char buf[512];
        size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
        fclose(fp);
        buf[n] = '\0';
        printf("cmdline:");
        for (size_t i = 0; i < n; i++)
            putchar(buf[i] == '\0' ? ' ' : buf[i]);
        putchar('\n');
    }

    return 0;
}
