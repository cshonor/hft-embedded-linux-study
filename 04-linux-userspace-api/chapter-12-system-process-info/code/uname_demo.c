/* POSIX uname — portable system identity.
 * cc -Wall -Wextra -o uname_demo uname_demo.c && ./uname_demo
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

int main(void)
{
    struct utsname u;
    char host[256];

    if (uname(&u) == -1) {
        perror("uname");
        return EXIT_FAILURE;
    }

    printf("sysname : %s\n", u.sysname);
    printf("nodename: %s\n", u.nodename);
    printf("release : %s\n", u.release);
    printf("version : %s\n", u.version);
    printf("machine : %s\n", u.machine);
#ifdef _GNU_SOURCE
    printf("domain  : %s\n", u.domainname);
#endif

    if (gethostname(host, sizeof(host)) == 0)
        printf("gethostname: %s\n", host);

    return 0;
}
