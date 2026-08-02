/* Minimal process list: scan /proc/[pid]/comm (and status Name).
 * cc -Wall -Wextra -o mini_ps mini_ps.c && ./mini_ps
 *
 * Race: PIDs can disappear between readdir and open → skip ENOENT.
 */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_pid_dir(const char *name)
{
    if (!*name)
        return 0;
    for (const char *p = name; *p; p++) {
        if (!isdigit((unsigned char)*p))
            return 0;
    }
    return 1;
}

static void print_one(const char *pid)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%s/comm", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (errno != ENOENT)
            fprintf(stderr, "fopen %s: %s\n", path, strerror(errno));
        return;
    }

    char name[256];
    if (!fgets(name, sizeof(name), fp)) {
        fclose(fp);
        return;
    }
    fclose(fp);

    size_t len = strlen(name);
    if (len && name[len - 1] == '\n')
        name[len - 1] = '\0';

    printf("%8s  %s\n", pid, name);
}

int main(void)
{
    DIR *d = opendir("/proc");
    if (!d) {
        perror("opendir /proc");
        return EXIT_FAILURE;
    }

    printf("%8s  %s\n", "PID", "COMM");
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (is_pid_dir(ent->d_name))
            print_one(ent->d_name);
    }

    closedir(d);
    return 0;
}
