/* Permanent privilege drop: setuid(getuid()) clears Saved-UID too.
 * Contrast with seteuid_drop_restore.c (temporary drop).
 *
 *   cc -Wall -Wextra -o setuid_permanent_drop setuid_permanent_drop.c
 *   sudo chown root:root setuid_permanent_drop
 *   sudo chmod u+s setuid_permanent_drop
 *   ./setuid_permanent_drop
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void show(const char *tag)
{
    uid_t r, e, s;
    if (getresuid(&r, &e, &s) == -1) {
        perror("getresuid");
        exit(EXIT_FAILURE);
    }
    printf("%-14s R=%u E=%u S=%u\n", tag, (unsigned)r, (unsigned)e, (unsigned)s);
}

int main(void)
{
    uid_t ruid = getuid();

    show("start");
    if (geteuid() != 0) {
        fprintf(stderr, "need setuid-root binary\n");
        return 1;
    }

    /* Permanent: R, E, and Saved all become ruid — cannot restore */
    if (setuid(ruid) == -1) {
        perror("setuid(ruid)");
        return 1;
    }
    show("after setuid");

    if (seteuid(0) == -1)
        printf("seteuid(0) failed as expected (no saved root)\n");
    else
        printf("UNEXPECTED: seteuid(0) succeeded\n");

    show("final");
    return 0;
}
