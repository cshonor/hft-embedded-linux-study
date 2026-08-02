/* Safe sysconf / pathconf printer — distinguish error vs indeterminate.
 * Build: cc -Wall -Wextra -o print_limits print_limits.c
 * Usage: ./print_limits [path...]
 *        default paths: . /
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_sysconf(int name, const char *label)
{
    errno = 0;
    long v = sysconf(name);
    if (v == -1) {
        if (errno == 0)
            printf("  %-28s  (indeterminate)\n", label);
        else
            printf("  %-28s  ERROR: %s\n", label, strerror(errno));
        return;
    }
    printf("  %-28s  %ld\n", label, v);
}

static void print_pathconf(const char *path, int name, const char *label)
{
    errno = 0;
    long v = pathconf(path, name);
    if (v == -1) {
        if (errno == 0)
            printf("  %-28s  (indeterminate)\n", label);
        else
            printf("  %-28s  ERROR: %s\n", label, strerror(errno));
        return;
    }
    printf("  %-28s  %ld\n", label, v);
}

static void dump_path(const char *path)
{
    static const struct {
        int name;
        const char *label;
    } pc[] = {
        { _PC_NAME_MAX, "_PC_NAME_MAX" },
        { _PC_PATH_MAX, "_PC_PATH_MAX" },
        { _PC_PIPE_BUF, "_PC_PIPE_BUF" },
        { _PC_NO_TRUNC, "_PC_NO_TRUNC" },
    };

    printf("\npathconf(\"%s\"):\n", path);
    for (size_t i = 0; i < sizeof(pc) / sizeof(pc[0]); i++)
        print_pathconf(path, pc[i].name, pc[i].label);
}

int main(int argc, char *argv[])
{
    printf("sysconf (system-wide):\n");
    print_sysconf(_SC_ARG_MAX, "_SC_ARG_MAX");
    print_sysconf(_SC_OPEN_MAX, "_SC_OPEN_MAX");
    print_sysconf(_SC_PAGESIZE, "_SC_PAGESIZE");
    print_sysconf(_SC_CLK_TCK, "_SC_CLK_TCK");
    print_sysconf(_SC_NGROUPS_MAX, "_SC_NGROUPS_MAX");
    print_sysconf(_SC_LOGIN_NAME_MAX, "_SC_LOGIN_NAME_MAX");
#ifdef _SC_THREADS
    print_sysconf(_SC_THREADS, "_SC_THREADS");
#endif

    if (argc > 1) {
        for (int i = 1; i < argc; i++)
            dump_path(argv[i]);
    } else {
        dump_path(".");
        dump_path("/");
    }

    printf("\nTip: compare with  getconf ARG_MAX   getconf NAME_MAX /path\n");
    return 0;
}
