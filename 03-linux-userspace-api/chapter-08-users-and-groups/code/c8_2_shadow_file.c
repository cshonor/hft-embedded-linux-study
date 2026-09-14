/* c8_2_shadow_file.c —— 8.2：为什么密文要搬到 /etc/shadow
 *
 * 三件事一起做：
 *   1. stat() 看 /etc/shadow 的权限位（密文只给 root + shadow 组）
 *   2. 以 root 读一次；再 fork 出子进程 setuid() 降权到普通账号读一次，
 *      把「普通用户读不到」从书上的一句话变成可复现的实验
 *   3. fgetspent() 解析一份合成 shadow，看字段含义与 ! / * 的锁定语义
 *
 * 编译：cc -Wall -Wextra -o c8_2_shadow_file c8_2_shadow_file.c
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static void try_shadow(const char *who)
{
    errno = 0;
    FILE *fp = fopen("/etc/shadow", "r");
    printf("[%s] fopen(/etc/shadow) -> %s  errno=%d(%s)\n",
           who, fp ? "ok" : "NULL", errno, strerror(errno));
    if (fp != NULL)
        fclose(fp);

    errno = 0;
    struct spwd *sp = getspnam("ce");
    printf("[%s] getspnam(\"ce\")   -> %s  errno=%d(%s)\n",
           who, sp ? "non-NULL" : "NULL", errno, strerror(errno));
    if (sp != NULL)
        printf("[%s]     sp_pwdp=%s lstchg=%ld min=%ld max=%ld warn=%ld\n",
               who, sp->sp_pwdp, sp->sp_lstchg, sp->sp_min, sp->sp_max, sp->sp_warn);
}

int main(void)
{
    struct stat st;
    if (stat("/etc/shadow", &st) == 0) {
        printf("/etc/shadow mode=%04o uid=%ld gid=%ld size=%ld\n",
               (unsigned) (st.st_mode & 07777), (long) st.st_uid,
               (long) st.st_gid, (long) st.st_size);
    } else {
        perror("stat /etc/shadow");
    }

    try_shadow("root");

    struct passwd *pw = getpwnam("ce");
    if (pw == NULL) {
        puts("找不到 ce 账号，跳过降权实验");
        return 0;
    }

    printf("\n尝试降权到 %s(%ld) 复现「普通用户视角」…\n",
           pw->pw_name, (long) pw->pw_uid);

    fflush(stdout);                     /* fork 前刷缓冲，否则子进程会复制一份未 flush 的输出 */
    pid_t pid = fork();
    if (pid == 0) {
        if (setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0) {
            printf("[child] setuid(%ld) 失败：errno=%d(%s)\n",
                   (long) pw->pw_uid, errno, strerror(errno));
            puts("[child] 评测容器跑在 user namespace 里，只映射了 uid 0，");
            puts("[child] 所以 setuid 到别的 uid 直接 EINVAL —— 这条实验在这里做不成。");
            puts("[child] 「普通用户读不到 shadow」的结论改由文件权限位 + man 5 shadow 说明。");
            fflush(stdout);             /* _exit 不 flush stdio */
            _exit(0);
        }
        printf("[child] 降权后 uid=%ld\n", (long) getuid());
        try_shadow("ce");
        fflush(stdout);
        _exit(0);
    }
    wait(NULL);

    /* ---- 合成一份 shadow，看字段与锁定语义 ---- */
    const char *sf = "/tmp/c8_shadow_demo";
    FILE *w = fopen(sf, "w");
    if (w != NULL) {
        fputs("alice:$6$abcdefgh$abc:19000:0:99999:7:14:20000\n"
              "bob:!:19000:0:99999:7:::\n"
              "carol:*:19000:0:99999:7:::\n", w);
        fclose(w);
    }

    puts("\n-- fgetspent() 解析合成 shadow --");
    FILE *f = fopen(sf, "r");
    struct spwd *sp;
    while (f != NULL && (sp = fgetspent(f)) != NULL) {
        const char *verdict = "正常";
        if (sp->sp_pwdp[0] == '!')
            verdict = "已锁定（! 开头）";
        else if (sp->sp_pwdp[0] == '*')
            verdict = "禁用（* 开头，无口令）";
        printf("%-6s pwdp=%-24s lstchg=%-6ld max=%-6ld inact=%-4ld expire=%-6ld %s\n",
               sp->sp_namp, sp->sp_pwdp, sp->sp_lstchg, sp->sp_max,
               sp->sp_inact, sp->sp_expire, verdict);
    }
    if (f != NULL)
        fclose(f);

    return 0;
}
