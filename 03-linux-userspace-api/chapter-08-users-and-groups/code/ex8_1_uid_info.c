/* ex8_1_uid_info.c —— Ch08 自测题 1 参考实现
 *
 * 给定一个 UID，打印：用户名、主组名、以及该进程当前持有的附属组。
 *
 * 关键点在于「三种失败要分开」：
 *   - UID 在 /etc/passwd 里没有记录  -> getpwuid 返回 NULL，errno 不变
 *   - /etc/group 里没有该 GID        -> getgrgid 返回 NULL，errno 不变
 *   - 真正的系统错误                 -> errno 被设成非 0
 * 把第一种当成「失败」退出是本类程序最常见的 bug。
 *
 * 编译：cc -Wall -Wextra -o ex8_1_uid_info ex8_1_uid_info.c
 * 运行：./ex8_1_uid_info [uid]        （不带参数则查当前进程的 uid）
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    uid_t uid = (argc > 1) ? (uid_t) strtol(argv[1], NULL, 10) : getuid();

    /* ---- 第 1 步：查用户（errno 必须自己清零） ---- */
    errno = 0;
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        if (errno == 0) {
            printf("UID %ld：/etc/passwd 里没有这条记录（这不算出错）\n",
                   (long) uid);
        } else {
            perror("getpwuid");
        }
        return 0;       /* 「查不到」不是程序失败，正常退出 */
    }

    printf("UID %ld -> 用户名 %s\n", (long) uid, pw->pw_name);
    printf("            注释   %s\n", pw->pw_gecos);
    printf("            家目录 %s\n", pw->pw_dir);
    printf("            壳     %s\n", pw->pw_shell);

    /* ---- 第 2 步：主组。来自 passwd 的 gid 字段 ---- */
    errno = 0;
    struct group *gr = getgrgid(pw->pw_gid);
    if (gr != NULL)
        printf("主组   %s(%ld)\n", gr->gr_name, (long) gr->gr_gid);
    else
        printf("主组   (%ld) —— /etc/group 里查不到该 GID（errno=%d）\n",
               (long) pw->pw_gid, errno);

    /* ---- 第 3 步：附属组。来自进程凭证，与上面的文件无关 ---- */
    int n = getgroups(0, NULL);
    if (n < 0) {
        perror("getgroups");
        return 1;
    }

    printf("附属组 %d 个", n);
    if (n > 0) {
        gid_t *gs = malloc((size_t) n * sizeof(gid_t));
        if (gs == NULL) {
            perror("malloc");
            return 1;
        }
        if (getgroups(n, gs) != n) {    /* 两次调用之间组数可能变，以第二次为准 */
            perror("getgroups");
            free(gs);
            return 1;
        }
        for (int i = 0; i < n; i++) {
            struct group *g = getgrgid(gs[i]);      /* 结果立即用掉，不留指针 */
            if (g != NULL)
                printf(" %s(%ld)", g->gr_name, (long) gs[i]);
            else
                printf(" ?(%ld)", (long) gs[i]);
        }
        free(gs);
    }
    putchar('\n');

    printf("主组 %ld 是否也在附属组里：%s\n", (long) pw->pw_gid,
           (n > 0) ? "看上面列表（内核不保证互斥）" : "列表为空，无从判断");
    puts("注：附属组由进程凭证携带，内核对它一无所知地放在 /etc/passwd 里 ——");
    puts("    真正把它塞进凭证的是 setgroups() / initgroups()，属 Ch9 §9.6。");
    return 0;
}
