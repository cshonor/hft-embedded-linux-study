/* TLPI 第 2 章 §2.19 —— /proc 文件系统：内核暴露出来的运行时数据库
 *
 * 编译：gcc -O0 -Wall -Wextra c2_19_proc.c -o c2_19
 * 运行：./c2_19
 *
 * 本节要钉死的事实：
 *   ① /proc 不是磁盘上的文件系统：它没有 inode-to-disk 映射，读它的每一下
 *      都在调用一段内核函数，把内核数据结构**现场格式化**成文本。
 *   ② /proc 下有两类东西：全局的（version/uptime/meminfo）与
 *      per-process 的（/proc/PID/ 目录下那一批，读 /proc/self 就是读自己）。
 *   ③ 每个进程目录里的一组固定文件，构成这个进程的「全息档案」。
 *   ④ 工具（ps/top/free/htop）本质都是「解析 /proc 的文本」，没有魔法。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

/* 读一个文件的前若干行 */
static void head_file(const char *path, int maxlines)
{
    FILE *f = fopen(path, "r");
    if (!f) { printf("    %-28s 打不开 (%s)\n", path, strerror(errno)); errno = 0; return; }
    char line[512];
    int n = 0;
    while (fgets(line, sizeof(line), f) && n < maxlines) {
        line[strcspn(line, "\n")] = '\0';
        printf("    %-28s | %s\n", n == 0 ? path : "", line);
        n++;
    }
    if (n == 0) printf("    %-28s | (空)\n", path);
    fclose(f);
}

/* 从 /proc/self/status 里取一行 */
static void status_line(const char *key)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;
    char line[512];
    size_t klen = strlen(key);
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, klen) == 0) {
            line[strcspn(line, "\n")] = '\0';
            printf("    %s\n", line);
            break;
        }
    }
    fclose(f);
}

static void readlink_of(const char *path)
{
    char buf[512];
    ssize_t n = readlink(path, buf, sizeof(buf) - 1);
    if (n < 0) printf("    %-28s -> (读不到: %s)\n", path, strerror(errno));
    else { buf[n] = '\0'; printf("    %-28s -> %s\n", path, buf); }
    errno = 0;
}

int main(void)
{
    printf("=== ① /proc 是虚拟文件系统，不是磁盘 ===\n");
    struct stat st;
    if (stat("/proc", &st) == 0)
        printf("  stat(\"/proc\")  st_dev=%lu（一个不存在的块设备号）st_ino=%lu\n",
               (unsigned long)st.st_dev, (unsigned long)st.st_ino);
    printf("  实测几个「读一下就触发内核函数」的文件：\n");
    head_file("/proc/version", 1);
    head_file("/proc/uptime", 1);
    head_file("/proc/loadavg", 1);
    head_file("/proc/sys/kernel/pid_max", 1);
    head_file("/proc/sys/fs/pipe-max-size", 1);

    printf("\n=== ② 全局文件里值得记住的几项 ===\n");
    head_file("/proc/meminfo", 3);
    head_file("/proc/self/mountinfo", 1);
    printf("    /proc/filesystems          —— 内核支持的 FS 类型\n");
    printf("    /proc/devices              —— 已注册的字符/块设备主设备号\n");
    printf("    /proc/interrupts           —— 每个 CPU 的中断计数（看中断打在哪个核）\n");
    printf("    /proc/softirqs             —— 软中断统计\n");
    printf("    /proc/net/*                —— socket/路由/网卡统计\n");

    printf("\n=== ③ 遍历 /proc，数一数系统里有多少进程 ===\n");
    DIR *d = opendir("/proc");
    if (!d) { perror("opendir /proc"); return 1; }
    struct dirent *e;
    int npid = 0, first_pid = -1, last_pid = -1;
    pid_t me = getpid();
    int seen_me = 0;
    while ((e = readdir(d))) {
        if (!isdigit((unsigned char)e->d_name[0])) continue;
        int pid = atoi(e->d_name);
        if (pid <= 0) continue;
        npid++;
        if (first_pid < 0 || pid < first_pid) first_pid = pid;
        if (pid > last_pid) last_pid = pid;
        if (pid == (int)me) seen_me = 1;
    }
    closedir(d);
    printf("  /proc 下的数字目录（= 进程）共 %d 个\n", npid);
    printf("  其中最小的 pid = %d，最大的 pid = %d\n", first_pid, last_pid);
    printf("  我自己的 pid = %d，被我找到了吗？%s\n", (int)me, seen_me ? "YES" : "no");
    printf("  -> pid_max 决定上限；内核是循环分配的，所以数字不连续。\n");

    printf("\n=== ④ 一个进程的「全息档案」：/proc/self/* ===\n");
    printf("  身份与状态：\n");
    status_line("Name:");
    status_line("Pid:");
    status_line("PPid:");
    status_line("Uid:");
    status_line("Gid:");
    status_line("State:");

    printf("\n  资源用量：\n");
    status_line("VmRSS:");
    status_line("VmSize:");
    status_line("Threads:");
    status_line("FDSize:");

    printf("\n  符号链接（指向别处的引用）：\n");
    readlink_of("/proc/self/exe");
    readlink_of("/proc/self/cwd");
    readlink_of("/proc/self/root");

    printf("\n  二进制原貌：\n");
    FILE *cm = fopen("/proc/self/cmdline", "rb");
    if (cm) {
        char b[512];
        size_t k = fread(b, 1, sizeof(b) - 1, cm);
        printf("    /proc/self/cmdline           | ");
        for (size_t i = 0; i < k; i++) putchar(b[i] == '\0' ? '|' : b[i]);
        printf("\n");
        fclose(cm);
    }

    printf("\n  fd 表（用户态视角的 fdtable）：\n");
    DIR *fd_dir = opendir("/proc/self/fd");
    if (fd_dir) {
        int self = dirfd(fd_dir);
        int cnt = 0;
        struct dirent *fe;
        while ((fe = readdir(fd_dir))) {
            if (fe->d_name[0] == '.') continue;
            if (atoi(fe->d_name) == self) continue;
            char p[320], tgt[320];
            snprintf(p, sizeof(p), "/proc/self/fd/%s", fe->d_name);
            ssize_t k = readlink(p, tgt, sizeof(tgt) - 1);
            tgt[k < 0 ? 0 : k] = '\0';
            printf("    fd %-3s -> %s\n", fe->d_name, tgt);
            cnt++;
        }
        closedir(fd_dir);
        printf("    共 %d 个 fd\n", cnt);
    }

    printf("\n=== ⑤ 为什么工具都是解析 /proc ===\n");
    printf("  %-12s %-34s %s\n", "工具", "它其实在读什么", "对应字段");
    printf("  %-12s %-34s %s\n", "------------", "----------------------------------",
           "----------------------");
    printf("  %-12s %-34s %s\n", "ps", "/proc/PID/stat, /proc/PID/cmdline", "state, comm, args");
    printf("  %-12s %-34s %s\n", "top / htop", "/proc/PID/stat + /proc/meminfo", "utime, stime, RSS");
    printf("  %-12s %-34s %s\n", "free", "/proc/meminfo", "MemTotal, MemAvailable");
    printf("  %-12s %-34s %s\n", "uptime", "/proc/uptime + /proc/loadavg", "load average");
    printf("  %-12s %-34s %s\n", "lsof", "/proc/*/fd/*", "每个 fd 的目标");
    printf("  %-12s %-34s %s\n", "ss / netstat", "/proc/net/tcp, /proc/net/unix", "socket 表");
    printf("  %-12s %-34s %s\n", "lsblk / df", "/proc/partitions, /proc/self/mountinfo", "块设备与挂载");
    printf("\n  → 所以「监控」不需要任何特权 API：读文本就够了。\n");
    printf("     HFT 里最常见的做法也是直接读 /proc/self/status 和\n");
    printf("     /proc/self/schedstat，而不是调某个库。\n");

    printf("\n=== ⑥ /proc 的写入面（只读是错觉）===\n");
    printf("  /proc/sys/* 是可写的：写它就是改内核参数（sysctl 的本质）。\n");
    printf("  例：echo 1 > /proc/sys/net/ipv4/ip_forward\n");
    printf("      → sysctl -w net.ipv4.ip_forward=1 完全等价。\n");
    printf("  实测 /proc/sys 是否可写（本环境）:\n");
    printf("    access(\"/proc/sys/kernel/pid_max\", W_OK) = %d\n",
           access("/proc/sys/kernel/pid_max", W_OK));
    printf("  → 而 /proc/<pid>/ 下多数文件是只读的，少数可写（如 oom_score_adj）。\n");
    errno = 0;
    return 0;
}
