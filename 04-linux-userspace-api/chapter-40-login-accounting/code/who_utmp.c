/* Mini who / last-boot helper via utmp API.
 * cc -Wall -Wextra -o who_utmp who_utmp.c
 * ./who_utmp
 * ./who_utmp wtmp
 * ./who_utmp boot
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utmp.h>
#include <unistd.h>

static void print_user(const struct utmp *ut)
{
    char tbuf[32];
    time_t t = (time_t)ut->ut_tv.tv_sec;

    strftime(tbuf, sizeof(tbuf), "%F %T", localtime(&t));
    printf("%-12.12s %-12.12s %-16.16s %s\n",
           ut->ut_user, ut->ut_line, ut->ut_host, tbuf);
}

static void print_boot(const struct utmp *ut)
{
    char tbuf[32];
    time_t t = (time_t)ut->ut_tv.tv_sec;

    strftime(tbuf, sizeof(tbuf), "%F %T", localtime(&t));
    printf("BOOT_TIME %s\n", tbuf);
}

int main(int argc, char *argv[])
{
    struct utmp *ut;
    int want_boot = 0;
    const char *file = NULL;

    if (argc > 1) {
        if (strcmp(argv[1], "wtmp") == 0)
            file = "/var/log/wtmp";
        else if (strcmp(argv[1], "boot") == 0) {
            file = "/var/log/wtmp";
            want_boot = 1;
        } else {
            fprintf(stderr, "usage: %s [wtmp|boot]\n", argv[0]);
            return 1;
        }
    }

    if (file != NULL && utmpname(file) == -1) {
        perror("utmpname");
        return 1;
    }

    setutent();
    while ((ut = getutent()) != NULL) {
        if (want_boot) {
            if (ut->ut_type == BOOT_TIME)
                print_boot(ut);
        } else if (ut->ut_type == USER_PROCESS && ut->ut_user[0] != '\0') {
            print_user(ut);
        }
    }
    endutent();
    return 0;
}
