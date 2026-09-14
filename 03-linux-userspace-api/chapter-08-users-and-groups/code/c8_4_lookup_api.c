/* c8_4_lookup_api.c —— 8.4：查询 API 的返回值语义
 *
 * 三个必须记住的点，全部实测：
 *   1. 「查不到」= 返回 NULL 且 errno 不变（要把 errno 先清零才能区分）
 *   2. 返回的指针指向静态缓冲，下一次调用就把它改写掉
 *   3. 静态缓冲的地址由 libc 决定，两次调用可能不同
 *
 * 编译：cc -Wall -Wextra -o c8_4_lookup_api c8_4_lookup_api.c
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
    /* ---- 1. errno 范式 ---- */
    errno = 0;
    struct passwd *pw = getpwnam("nosuchuser");
    printf("errno=0; getpwnam(\"nosuchuser\") -> %s  errno=%d  => %s\n",
           pw ? "non-NULL" : "NULL", errno,
           (pw == NULL && errno == 0) ? "「用户不存在」" : "「系统错误」");

    errno = 0;
    pw = getpwuid(0);
    printf("errno=0; getpwuid(0)              -> %s  errno=%d  => %s\n",
           pw ? pw->pw_name : "NULL", errno,
           (pw == NULL && errno == 0) ? "「无此 UID 的记录」" : "「系统错误」");

    errno = 0;
    pw = getpwnam("ce");
    if (pw != NULL)
        printf("errno=0; getpwnam(\"ce\")           -> uid=%ld gid=%ld gecos=%s\n",
               (long) pw->pw_uid, (long) pw->pw_gid, pw->pw_gecos);

    /* ---- 2. 静态缓冲：连续读两条记录，前一个指针的内容被改写 ---- */
    const char *pf = "/tmp/c8_passwd_demo";
    FILE *w = fopen(pf, "w");
    if (w == NULL) {
        perror("fopen for write");
        return 1;
    }
    fputs("alice:x:1001:1001:Alice:/home/alice:/bin/bash\n"
          "bob:x:1002:1002:Bob:/home/bob:/bin/sh\n"
          "carol:x:1003:1003:Carol:/home/carol:/sbin/nologin\n", w);
    fclose(w);

    puts("\n-- 静态缓冲实测：fgetpwent() 读三条记录 --");
    FILE *f = fopen(pf, "r");
    struct passwd *p1 = fgetpwent(f);
    printf("第 1 条: p1=%p p1->pw_name=\"%s\"\n", (void *) p1, p1->pw_name);

    struct passwd *p2 = fgetpwent(f);
    printf("第 2 条: p2=%p p2->pw_name=\"%s\"\n", (void *) p2, p2->pw_name);
    printf("        此刻再用 p1 读 -> p1->pw_name=\"%s\"  (p1==p2? %s)\n",
           p1->pw_name, p1 == p2 ? "是" : "否");

    struct passwd *p3 = fgetpwent(f);
    printf("第 3 条: p3=%p p3->pw_name=\"%s\"\n", (void *) p3, p3->pw_name);
    printf("        此刻 p1->pw_name=\"%s\"，p2->pw_name=\"%s\" —— 三个指针是同一块内存\n",
           p1->pw_name, p2->pw_name);
    fclose(f);

    /* ---- 3. 地址由 libc 决定，别做任何假设 ---- */
    struct passwd *a = getpwnam("ce");
    struct passwd *b = getpwuid(10240);
    printf("\ngetpwnam(\"ce\")=%p  getpwuid(10240)=%p  (同一用户，地址%s)\n",
           (void *) a, (void *) b, a == b ? "相同" : "不同");

    puts("\n结论：返回的指针「有效到下一次同类调用为止」，要留住内容就得自己拷一份；");
    puts("      多线程里必须换 _r 版本（见 c8_5_lookup_r.c）。");
    return 0;
}
