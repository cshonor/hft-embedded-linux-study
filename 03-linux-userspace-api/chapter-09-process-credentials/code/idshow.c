/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2026.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* idshow.c

   Display all user and group identifiers associated with a process.

   Note: This program uses Linux-specific calls and the Linux-specific
   file-system user and group IDs.
*/
/* ---------------------------------------------------------------------------
 * 本文件是 TLPI 官方源码 proccred/idshow.c 的逐行复刻（= 原书 Listing 9-1）。
 * 官方页面: https://man7.org/tlpi/code/online/dist/proccred/idshow.c
 *
 * 为了能在本仓库里单独编译，只做了两处**与逻辑无关**的调整：
 *   1. 去掉 "#include \"tlpi_hdr.h\""，改为直接包含它提供的那几个标准头，
 *      并内联一个最小的 errExit()（原来是 tlpi_hdr.h 里的宏）。
 *   2. 原样保留 "#include \"ugid_functions.h\""：那是 Listing 8-1 的助手函数，
 *      本仓库不复制它，直接用 Ch08 目录下那一份（编译命令见 code/README.md）。
 *
 * 原书的两个教学点在这份源码里看得最清楚：
 *   - 它用 **getresuid/getresgid** 一次拿 R/E/S，而不是 getuid/geteuid/getgid 三连；
 *   - 它读 fsuid/fsgid 用的是 `fsuid = setfsuid(0);` —— 这个「返回值其实是旧值」
 *     的怪接口，配套注释写的是 "Attempts to change the file-system IDs are always
 *     ignored for unprivileged processes, but even so, the following calls return
 *     the current file-system IDs"。
 * ------------------------------------------------------------------------- */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/types.h>
#include <unistd.h>

#include "ugid_functions.h"   /* userNameFromId() & groupNameFromId() */

#define SG_SIZE (NGROUPS_MAX + 1)

/* 替代 tlpi_hdr.h 的 errExit()（原书里是宏） */
static void errExit(const char *msg)
{
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    uid_t ruid, euid, suid, fsuid;
    gid_t rgid, egid, sgid, fsgid;
    gid_t suppGroups[SG_SIZE];
    int numGroups, j;
    char *p;

    (void) argc;
    (void) argv;

    if (getresuid(&ruid, &euid, &suid) == -1)
        errExit("getresuid");
    if (getresgid(&rgid, &egid, &sgid) == -1)
        errExit("getresgid");

    /* Attempts to change the file-system IDs are always ignored
       for unprivileged processes, but even so, the following
       calls return the current file-system IDs */

    fsuid = setfsuid(0);
    fsgid = setfsgid(0);

    printf("UID: ");
    p = userNameFromId(ruid);
    printf("real=%s (%ld); ", (p == NULL) ? "???" : p, (long) ruid);
    p = userNameFromId(euid);
    printf("eff=%s (%ld); ", (p == NULL) ? "???" : p, (long) euid);
    p = userNameFromId(suid);
    printf("saved=%s (%ld); ", (p == NULL) ? "???" : p, (long) suid);
    p = userNameFromId(fsuid);
    printf("fs=%s (%ld); ", (p == NULL) ? "???" : p, (long) fsuid);
    printf("\n");

    printf("GID: ");
    p = groupNameFromId(rgid);
    printf("real=%s (%ld); ", (p == NULL) ? "???" : p, (long) rgid);
    p = groupNameFromId(egid);
    printf("eff=%s (%ld); ", (p == NULL) ? "???" : p, (long) egid);
    p = groupNameFromId(sgid);
    printf("saved=%s (%ld); ", (p == NULL) ? "???" : p, (long) sgid);
    p = groupNameFromId(fsgid);
    printf("fs=%s (%ld); ", (p == NULL) ? "???" : p, (long) fsgid);
    printf("\n");

    numGroups = getgroups(SG_SIZE, suppGroups);
    if (numGroups == -1)
        errExit("getgroups");

    printf("Supplementary groups (%d): ", numGroups);
    for (j = 0; j < numGroups; j++) {
        p = groupNameFromId(suppGroups[j]);
        printf("%s (%ld) ", (p == NULL) ? "???" : p, (long) suppGroups[j]);
    }
    printf("\n");

    exit(EXIT_SUCCESS);
}
