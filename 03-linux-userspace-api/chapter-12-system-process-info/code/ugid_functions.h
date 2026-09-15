/*************************************************************************\
*  ⚠️ 这不是原书的 lib/ugid_functions.h！                                *
*                                                                         *
*  这是它的**最小替身**，只为 procfs_user_exe.c（原书 Ch12 习题 12-1 的    *
*  解答）里的 userIdFromName() 能零改动编译而存在。                        *
*                                                                         *
*  原书是「ugid_functions.h 声明 + ugid_functions.c 实现」两层，提供四个   *
*  函数：userNameFromId / userIdFromName / groupNameFromId /                *
*  groupIdFromName。本替身只保留 procfs_user_exe.c 真正调用的那一个。       *
*                                                                         *
*  真实实现可从 man7 取：                                                   *
*    https://man7.org/tlpi/code/online/dist/lib/ugid_functions.c           *
*  替身的语义与真实实现**逐条对齐**（连线号顺序都一样）：                  *
*    1) NULL 或空串        → 返回 (uid_t) -1                               *
*    2) 纯数字串（strtol 后 *endptr == '\0'）→ 直接把数字当 UID 返回       *
*       ← 这一步很关键：**"0" 这类数字串根本不查 /etc/passwd**，           *
*         所以即便系统没有 passwd 文件也能用它按 UID 过滤                  *
*    3) 其它字符串          → getpwnam()，查不到返回 (uid_t) -1            *
*                                                                         *
*  ⚠️ 与 Ch04/Ch05/Ch10/Ch11 无关：那些章没有这个头。别跨章复制。          *
\*************************************************************************/
#ifndef UGID_FUNCTIONS_H
#define UGID_FUNCTIONS_H

#include <pwd.h>        /* struct passwd, getpwnam(), getpwuid() */
#include <stdlib.h>     /* strtol() */

/* 原书 ugid_functions.c:30-49：userIdFromName
   "Return UID corresponding to 'name', or -1 on error" */
static inline uid_t userIdFromName(const char *name)
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

/* 原书 ugid_functions.c:21-28：userNameFromId
   本章没用到，但留着让替身形状与原书一致（procfs_user_exe.c 若改成打印
   用户名就用得上）。未被调用时 static inline 不会产生代码。*/
static inline char *userNameFromId(uid_t uid)
{
    struct passwd *pwd;

    pwd = getpwuid(uid);
    return (pwd == NULL) ? NULL : pwd->pw_name;
}

#endif /* UGID_FUNCTIONS_H */
