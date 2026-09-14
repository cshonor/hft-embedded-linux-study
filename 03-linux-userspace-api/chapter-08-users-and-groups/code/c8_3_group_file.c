/* c8_3_group_file.c —— 8.3：/etc/group 与「主组 / 附属组」
 *
 * 本容器里 /etc/group 根本不存在（第一条输出就是证据），所以这里做两件事：
 *   1. 如实报告真实 API 的行为（getgrent 计数、getgrgid 返回 NULL）
 *   2. 用 fgetgrent() 解析一份合成 group 文件，看 4 字段格式与 gr_mem 语义
 *
 * 编译：cc -Wall -Wextra -o c8_3_group_file c8_3_group_file.c
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* ---- 1. 真实环境 ---- */
    errno = 0;                          /* errno 必须在调用前清零！ */
    FILE *f = fopen("/etc/group", "r");
    printf("fopen(/etc/group) -> %s  errno=%d(%s)\n",
           f ? "ok" : "NULL", errno, strerror(errno));
    if (f != NULL)
        fclose(f);

    int n = 0;
    struct group *gr;
    setgrent();
    while ((gr = getgrent()) != NULL)
        n++;
    endgrent();
    printf("getgrent() 遍历到 %d 个组\n", n);

    errno = 0;
    gr = getgrgid(10240);
    printf("getgrgid(10240) -> %s（errno=%d）—— passwd 里写了 gid 10240，\n"
           "                  但 group 文件不存在，所以查不到\n",
           gr ? gr->gr_name : "NULL", errno);

    struct passwd *pw = getpwnam("ce");
    if (pw != NULL) {
        const char *gname = NULL;
        struct group *g2 = getgrgid(pw->pw_gid);
        if (g2 != NULL)
            gname = g2->gr_name;
        printf("用户 ce 的主组 gid=%ld -> 组名 %s\n", (long) pw->pw_gid,
               gname ? gname : "(查不到)");
    }

    /* ---- 2. 合成 group 文件：看格式与 gr_mem ---- */
    const char *gf = "/tmp/c8_group_demo";
    FILE *w = fopen(gf, "w");
    if (w == NULL) {
        perror("fopen for write");
        return 1;
    }
    fputs("rocket:x:500:alice,bob\n"
          "toolchain:x:501:carol\n"
          "emptygrp:x:502:\n", w);      /* 空组：冒号后面什么都没有 */
    fclose(w);

    puts("\n-- fgetgrent() 解析合成 group（格式 name:x:gid:member,member） --");
    f = fopen(gf, "r");
    while (f != NULL && (gr = fgetgrent(f)) != NULL) {
        printf("组名=%-12s 组密文=%-3s gid=%-5ld 成员数=",
               gr->gr_name, gr->gr_passwd, (long) gr->gr_gid);

        if (gr->gr_mem == NULL) {
            printf("gr_mem==NULL");
        } else {
            int cnt = 0;
            for (char **m = gr->gr_mem; *m != NULL; m++)
                cnt++;
            printf("%d", cnt);
        }
        printf("  成员=");
        if (gr->gr_mem != NULL) {
            for (char **m = gr->gr_mem; *m != NULL; m++)
                printf("[%s]", *m);
        }
        /* 空组的 gr_mem 不是 NULL，而是「首元素为 NULL 的数组」 */
        printf("   (gr_mem==NULL? %s ; gr_mem[0]==NULL? %s)\n",
               gr->gr_mem == NULL ? "yes" : "no",
               (gr->gr_mem == NULL || gr->gr_mem[0] == NULL) ? "yes" : "no");
    }
    if (f != NULL)
        fclose(f);

    puts("\n主组：写在 /etc/passwd 的 gid 字段（每个用户恰好一个）");
    puts("附属组：写在 /etc/group 的成员列表里（一个用户可以出现在多行）");
    return 0;
}
