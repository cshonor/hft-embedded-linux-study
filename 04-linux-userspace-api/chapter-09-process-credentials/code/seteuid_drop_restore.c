/* Demonstrate seteuid drop/restore using Saved-UID (needs setuid-root binary).
 *
 *   cc -Wall -Wextra -o seteuid_drop_restore seteuid_drop_restore.c
 *   sudo chown root:root seteuid_drop_restore
 *   sudo chmod u+s seteuid_drop_restore
 *   ./seteuid_drop_restore          # as normal user
 *
 * Without the setuid bit, restore to 0 will fail (non-privileged).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static void show(const char *tag)
{
    uid_t r, e, s;
    if (getresuid(&r, &e, &s) == -1) {
        perror("getresuid");
        exit(EXIT_FAILURE);
    }
    printf("%-12s R=%u E=%u S=%u\n", tag, (unsigned)r, (unsigned)e, (unsigned)s);
}

int main(void)
{
    uid_t ruid = getuid();

    show("start");

    if (geteuid() != 0) {
        fprintf(stderr,
                "EUID!=0: make binary setuid-root to exercise drop/restore\n");
        return 1;
    }

    if (seteuid(ruid) == -1) {
        perror("seteuid(ruid) drop");
        return 1;
    }
    show("dropped");

    if (seteuid(0) == -1) {
        perror("seteuid(0) restore");
        return 1;
    }
    show("restored");

    return 0;
}
