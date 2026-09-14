/* ugid_functions.h —— 原书 Listing 8-1 的头文件（复刻版）
 *
 * 原书说明（man7.org/tlpi/code/online/dist/users_groups/ugid_functions.h.html）：
 *     "This is users_groups/ugid_functions.h, an example to accompany the book
 *      ... This file is not printed in the book;
 *      it is the header file for ugid_functions.c (Listing 8-1, page 159)."
 * 也就是说它**没有印进书里**，只是 Listing 8-1 的头。
 *
 * 与原书的唯一差别：原书 #include "tlpi_hdr.h"（书自带的公共头，含 Boolean /
 * errExit 等）；本文件改成只引 <sys/types.h>（uid_t / gid_t 的出处）。
 * 函数声明逐字保持原样。
 */
#ifndef UGID_FUNCTIONS_H
#define UGID_FUNCTIONS_H

#include <sys/types.h>

char *userNameFromId(uid_t uid);

uid_t userIdFromName(const char *name);

char *groupNameFromId(gid_t gid);

gid_t groupIdFromName(const char *name);

#endif
