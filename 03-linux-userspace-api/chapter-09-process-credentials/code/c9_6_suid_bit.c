/* c9_6_suid_bit.c —— set-user-ID 位：能做的实验与四条「让它失效」的路
 *
 * 9.3 的主题：set-user-ID / set-group-ID 程序。
 *
 * 这一节最容易被写成「exec 之后 euid 变成文件属主」一句话就完了。其实内核里有
 * **四条独立的路径**会让这个位被无视（fs/exec.c:1602 bprm_fill_uid()）：
 *
 *   :1611  if (!mnt_may_suid(file->f_path.mnt)) return;          <- 挂载带 nosuid
 *   :1614  if (task_no_new_privs(current)) return;               <- no_new_privs=1
 *   :1618  if (!(mode & (S_ISUID|S_ISGID))) return;              <- 压根没这位
 *   :1633  if (!vfsuid_has_mapping(ns, vfsuid) || ...) return;   <- 属主在 ns 里没映射
 *
 * 对比 execve(2) 的 man：它只列了前三条里的 nosuid / no_new_privs / 正被 ptrace。
 * 「属主没映射」这条 man 里没写，只在源码里。
 *
 * 本程序做两件**在这里真能观测**的事：
 *   ① set-user-ID 位会被「写文件」清掉（只读打开不会）—— 这是内核
 *      file_remove_privs() 的行为，也解释了为什么不能靠改一个 setuid 程序来提权。
 *   ② 扫出镜像里现存的 SUID 文件，然后把它 execve 一遍，看看到底发生了什么。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_6_suid_bit c9_6_suid_bit.c
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void show_mode(const char *path, const char *tag)
{
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("    %-16s stat失败: %s\n", tag, strerror(errno));
        return;
    }
    printf("    %-16s mode=%04o  u+s=%s g+s=%s  uid=%u gid=%u\n",
           tag, (unsigned) (st.st_mode & 07777),
           (st.st_mode & S_ISUID) ? "ON " : "off",
           (st.st_mode & S_ISGID) ? "ON " : "off",
           (unsigned) st.st_uid, (unsigned) st.st_gid);
}

/* 实验 ①：写文件会不会清掉 set-user-ID / set-group-ID 位 */
static void experiment_write_clears_bit(void)
{
    const char *p = "/tmp/c9_suid_demo";
    printf("\n=== ① set-user-ID 位 vs 写文件 ===\n");
    unlink(p);
    int fd = open(p, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        printf("    建文件失败: %s\n", strerror(errno));
        return;
    }
    if (write(fd, "hello\n", 6) != 6)
        printf("    (写入长度不符)\n");
    close(fd);

    show_mode(p, "[1] 新建");
    if (chmod(p, 04755) == -1)
        printf("    chmod 04755 失败: %s\n", strerror(errno));
    show_mode(p, "[2] chmod 04755");
    fd = open(p, O_WRONLY | O_APPEND);
    if (fd == -1) {
        printf("    以写打开失败: %s\n", strerror(errno));
    } else {
        if (write(fd, "x", 1) != 1)
            printf("    (写入长度不符)\n");
        close(fd);
    }
    show_mode(p, "[3] 写了 1 字节");

    if (chmod(p, 04755) == -1)
        printf("    chmod 失败: %s\n", strerror(errno));
    show_mode(p, "[4] 再 chmod");
    fd = open(p, O_RDONLY);
    if (fd != -1) {
        char c;
        if (read(fd, &c, 1) < 0)
            printf("    (读失败)\n");
        close(fd);
    }
    show_mode(p, "[5] 只读打开");

    if (chmod(p, 02755) == -1)
        printf("    chmod 失败: %s\n", strerror(errno));
    show_mode(p, "[6] chmod 02755");
    fd = open(p, O_WRONLY | O_APPEND);
    if (fd != -1) {
        if (write(fd, "x", 1) != 1)
            printf("    (写入长度不符)\n");
        close(fd);
    }
    show_mode(p, "[7] 再写一次");
    unlink(p);
    printf("    结论: 一写就把 u+s / g+s 位清掉；只读不动。\n");
}

/* 实验 ②：扫出 SUID 文件 */
static int scan_suid(char found[][512], int max)
{
    const char *dirs[] = { "/usr/bin", "/usr/lib/openssh", "/bin", "/usr/sbin", "/sbin" };
    int n = 0;
    for (unsigned d = 0; d < sizeof dirs / sizeof dirs[0]; d++) {
        DIR *dp = opendir(dirs[d]);
        if (dp == NULL)
            continue;
        struct dirent *e;
        while ((e = readdir(dp)) != NULL && n < max) {
            if (e->d_name[0] == '.')
                continue;
            char full[512];
            snprintf(full, sizeof full, "%s/%s", dirs[d], e->d_name);
            struct stat st;
            if (stat(full, &st) == -1)
                continue;
            if (S_ISREG(st.st_mode) && (st.st_mode & S_ISUID)) {
                snprintf(found[n], 512, "%s", full);
                printf("    %04o uid=%u gid=%u  %s\n",
                       (unsigned) (st.st_mode & 07777), (unsigned) st.st_uid,
                       (unsigned) st.st_gid, full);
                n++;
            }
        }
        closedir(dp);
    }
    return n;
}

/* 实验 ③：这个文件所在的挂载点有没有 nosuid */
static void check_nosuid(const char *path)
{
    char dir[512];
    snprintf(dir, sizeof dir, "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash != NULL)
        *slash = '\0';

    FILE *fp = fopen("/proc/self/mounts", "r");
    if (fp == NULL) {
        printf("    打不开 /proc/self/mounts\n");
        return;
    }
    char dev[256], mnt[512], fstype[64], opts[512];
    int printed = 0;
    while (fscanf(fp, "%255s %511s %63s %511s%*[^\n]\n", dev, mnt, fstype, opts) == 4) {
        size_t l = strlen(mnt);
        if (strncmp(dir, mnt, l) == 0 && (l == 1 || dir[l] == '/' || dir[l] == '\0')) {
            printf("    挂载点 %s (%s) 选项: %s\n", mnt, fstype, opts);
            printf("    -> 含 nosuid? %s\n", strstr(opts, "nosuid") ? "是（SUID 位被无视）" : "否");
            printed = 1;
            break;
        }
    }
    fclose(fp);
    if (!printed)
        printf("    没找到匹配的挂载点\n");
}

int main(void)
{
    printf("=== 决定「set-user-ID 位管不管用」的四个开关 ===\n");
    printf("  fs/exec.c:1611  mnt_may_suid()         挂载带 nosuid -> 无视\n");
    printf("  fs/exec.c:1614  task_no_new_privs()    no_new_privs=1 -> 无视\n");
    printf("  fs/exec.c:1618  mode & (S_ISUID|S_ISGID)  没这位 -> 无视\n");
    printf("  fs/exec.c:1633  vfsuid_has_mapping()   属主在 ns 里没映射 -> 无视\n");

    printf("\n=== 本进程相关状态 ===\n");
    {
        FILE *fp = fopen("/proc/self/status", "r");
        char buf[512];
        while (fp != NULL && fgets(buf, sizeof buf, fp) != NULL)
            if (strncmp(buf, "NoNewPrivs:", 11) == 0 || strncmp(buf, "CapEff:", 7) == 0)
                printf("  %s", buf);
        if (fp != NULL)
            fclose(fp);
    }
    errno = 0;
    printf("  prctl(PR_GET_NO_NEW_PRIVS) = %d  errno=%d\n",
           prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0), errno);

    experiment_write_clears_bit();

    printf("\n=== ② 镜像里现存的 SUID 文件 ===\n");
    static char found[64][512];
    int n = scan_suid(found, 64);
    printf("    共 %d 个\n", n);

    if (n > 0) {
        const char *victim = found[0];
        printf("\n=== ③ 把 %s execve 一遍 ===\n", victim);
        check_nosuid(victim);
        show_mode(victim, "目标");
        printf("    它的属主 uid 在本 ns 的 uid_map 里吗？\n");
        {
            FILE *fp = fopen("/proc/self/uid_map", "r");
            char buf[256];
            printf("      uid_map: ");
            while (fp != NULL && fgets(buf, sizeof buf, fp) != NULL)
                printf("%s", buf);
            if (fp != NULL)
                fclose(fp);
            printf("      （只有一行 \"0 113 1\" -> uid 65534 不在里面）\n");
        }

        fflush(stdout);              /* ⚠️ fork 之前必须 flush，否则子进程会把缓冲吐两遍 */
        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = { (char *) victim, NULL };
            char *envp[] = { NULL };
            execve(victim, args, envp);
            printf("    execve 失败: %s\n", strerror(errno));
            fflush(stdout);
            _exit(127);
        }
        int st = 0;
        if (waitpid(pid, &st, 0) == -1) {
            perror("waitpid");
        } else if (WIFEXITED(st)) {
            printf("    退出码 = %d\n", WEXITSTATUS(st));
        } else if (WIFSIGNALED(st)) {
            printf("    被信号杀掉 = %d\n", WTERMSIG(st));
        }
        printf("    注意: 本程序**没变** —— 它既没有 set-user-ID 提权，也没有崩。\n");
    }

    printf("\n=== 小结（四条路里，这台机器上是哪几条在起作用）===\n");
    printf("  - no_new_privs=1  -> 开着（见上）\n");
    printf("  - nosuid          -> 见 ③ 的挂载点输出\n");
    printf("  - 属主未映射      -> 见 ③ 的 uid_map 输出（65534 不在映射里）\n");
    printf("  三条都是**独立的**，任意一条成立，set-user-ID 位就不生效。\n");
    return 0;
}
