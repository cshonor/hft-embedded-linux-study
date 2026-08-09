/* Listing 8-2 style: walk passwd and group databases. */
#include <grp.h>
#include <pwd.h>
#include <stdio.h>

int main(void) {
    struct passwd *pwd;
    struct group *grp;

    printf("=== users (/etc/passwd) ===\n");
    setpwent();
    while ((pwd = getpwent()) != NULL)
        printf("%-16s uid=%-6u gid=%-6u home=%s shell=%s\n",
               pwd->pw_name, (unsigned)pwd->pw_uid, (unsigned)pwd->pw_gid,
               pwd->pw_dir, pwd->pw_shell);
    endpwent();

    printf("\n=== groups (/etc/group) ===\n");
    setgrent();
    while ((grp = getgrent()) != NULL) {
        printf("%-16s gid=%-6u members=", grp->gr_name, (unsigned)grp->gr_gid);
        if (grp->gr_mem != NULL) {
            for (char **m = grp->gr_mem; *m != NULL; m++)
                printf("%s%s", *m, *(m + 1) ? "," : "");
        }
        putchar('\n');
    }
    endgrent();
    return 0;
}
