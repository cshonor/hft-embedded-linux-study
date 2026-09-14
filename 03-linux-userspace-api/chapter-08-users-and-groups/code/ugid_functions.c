/* ugid_functions.c —— 原书 Listing 8-1（p.159）复刻版
 *
 * 四个「名字 ↔ 数字 ID」的转换助手。TLPI 后面几乎所有例子都用它们把
 * uid/gid 翻译成人能读的名字（第 9/12/15/34/38/39/40 章…）。
 *
 * 与原书的差别：只把 #include "tlpi_hdr.h" 去掉（改由 ugid_functions.h
 * 提供 uid_t/gid_t），并补上原书靠 tlpi_hdr.h 间接引入的 <stdlib.h>
 * （strtol 的声明）。函数体逐行保持原样。
 *
 * 编译：cc -Wall -Wextra -c ugid_functions.c
 */
#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include "ugid_functions.h"

char *          /* Return name corresponding to 'uid', or NULL on error */
userNameFromId(uid_t uid)
{
    struct passwd *pwd;

    pwd = getpwuid(uid);
    return (pwd == NULL) ? NULL : pwd->pw_name;
}

uid_t           /* Return UID corresponding to 'name', or -1 on error */
userIdFromName(const char *name)
{
    struct passwd *pwd;
    uid_t u;
    char *endptr;

    if (name == NULL || *name == '\0')  /* On NULL or empty string */
        return -1;                      /* return an error */

    u = strtol(name, &endptr, 10);      /* As a convenience to caller */
    if (*endptr == '\0')                /* allow a numeric string */
        return u;

    pwd = getpwnam(name);
    if (pwd == NULL)
        return -1;

    return pwd->pw_uid;
}

char *          /* Return name corresponding to 'gid', or NULL on error */
groupNameFromId(gid_t gid)
{
    struct group *grp;

    grp = getgrgid(gid);
    return (grp == NULL) ? NULL : grp->gr_name;
}

gid_t           /* Return GID corresponding to 'name', or -1 on error */
groupIdFromName(const char *name)
{
    struct group *grp;
    gid_t g;
    char *endptr;

    if (name == NULL || *name == '\0')  /* On NULL or empty string */
        return -1;                      /* return an error */

    g = strtol(name, &endptr, 10);      /* As a convenience to caller */
    if (*endptr == '\0')                /* allow a numeric string */
        return g;

    grp = getgrnam(name);
    if (grp == NULL)
        return -1;

    return grp->gr_gid;
}
