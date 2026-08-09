/* Print process credentials: R/E/S UID·GID + supplementary groups.
 * Build: cc -Wall -Wextra -o print_credentials print_credentials.c
 * Optional: sudo chown root print_credentials && sudo chmod u+s print_credentials
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;

    if (getresuid(&ruid, &euid, &suid) == -1)
        die("getresuid");
    if (getresgid(&rgid, &egid, &sgid) == -1)
        die("getresgid");

    printf("UID  real=%u  effective=%u  saved=%u\n",
           (unsigned)ruid, (unsigned)euid, (unsigned)suid);
    printf("GID  real=%u  effective=%u  saved=%u\n",
           (unsigned)rgid, (unsigned)egid, (unsigned)sgid);
    printf("privileged (EUID==0)? %s\n", euid == 0 ? "yes" : "no");

    int n = getgroups(0, NULL);
    if (n < 0)
        die("getgroups(0)");

    gid_t *list = calloc((size_t)n, sizeof(*list));
    if (!list)
        die("calloc");

    if (getgroups(n, list) == -1) {
        free(list);
        die("getgroups");
    }

    printf("supplementary groups (%d):", n);
    for (int i = 0; i < n; i++)
        printf(" %u", (unsigned)list[i]);
    printf("\n");

    /* FUID: no portable getter; on modern Linux equals EUID unless setfsuid used */
    printf("filesystem IDs: treat as EUID/EGID unless setfsuid/setfsgid was used\n");

    free(list);
    return 0;
}
