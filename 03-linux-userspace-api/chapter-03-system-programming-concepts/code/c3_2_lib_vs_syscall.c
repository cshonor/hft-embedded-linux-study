/* TLPI 第 03 章 §3.2 —— 「库函数 ≠ 系统调用」的可测量版
 *
 * 口头说「atoi 不进内核、getpwuid 会读文件」很容易，本 demo 用 /proc/self/io
 * 的计数器把它**量出来**：
 *   rchar / wchar = 进程累计读/写的字节数
 *   syscr / syscw = 进程累计发起的 read / write 系统调用**次数**
 *
 * 方法论（很重要，否则读数会骗你）：
 *   读 /proc/self/io 这个动作自身就要 open + read + close，所以每测一次都自带
 *   一份「底噪」。本 demo 第一件事就是把这个底噪量出来，之后所有数字都与它对比，
 *   而不是假装底噪是 0。
 *   另外 /proc/self/io 只统计 read/write 这一类 I/O 系统调用 —— brk / mmap /
 *   ioctl 它看不见。所以它证明的是「没做 I/O」，不是「没进内核」。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_2 c3_2_lib_vs_syscall.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct io_counters {
    unsigned long long rchar, wchar, syscr, syscw;
};

static struct io_counters before, after;
static unsigned long long sink;
static char userbuf[64] = "ce";
static struct passwd *pw;
static FILE *out;

static int read_io(struct io_counters *io)
{
    FILE *f = fopen("/proc/self/io", "r");
    if (!f) {
        return -1;
    }
    char line[128];
    io->rchar = io->wchar = io->syscr = io->syscw = 0;
    while (fgets(line, sizeof line, f)) {
        sscanf(line, "rchar: %llu", &io->rchar);
        sscanf(line, "wchar: %llu", &io->wchar);
        sscanf(line, "syscr: %llu", &io->syscr);
        sscanf(line, "syscw: %llu", &io->syscw);
    }
    fclose(f);
    return 0;
}

/* 先把两次快照都取完，再打印 —— 打印本身也会产生 write 系统调用，
 * 若边打边测，就把自己的输出算进被测对象里了。 */
static void measure(const char *tag, void (*work)(void))
{
    if (read_io(&before) < 0) {
        printf("  %-34s /proc/self/io 不可读（errno=%d）\n", tag, errno);
        return;
    }
    work();
    if (read_io(&after) < 0) {
        return;
    }
    printf("  %-34s Δsyscr=%-3lld Δsyscw=%-3lld Δrchar=%-9lld Δwchar=%lld\n", tag,
           (long long) (after.syscr - before.syscr),
           (long long) (after.syscw - before.syscw),
           (long long) (after.rchar - before.rchar),
           (long long) (after.wchar - before.wchar));
}

/* ---------- 被测的五种「工作负载」 ---------- */

static void w_nothing(void)                 /* 空负载 → 量出底噪 */
{
}

static void w_userlib(void)                 /* 纯用户态：解析、拷贝、求长度 */
{
    char buf[256];
    for (int i = 0; i < 100000; i++) {
        strcpy(buf, "abcdef");
        sink += strlen(buf) + (unsigned long long) atoi("12345");
    }
}

static void w_snprintf(void)                /* 同样是「格式化」，但不落到任何 fd */
{
    char buf[64];
    for (int i = 0; i < 1000; i++) {
        snprintf(buf, sizeof buf, "%d-%s", i, "x");
        sink += (unsigned long long) buf[0];
    }
}

static void w_getpwnam(void)                /* 走 NSS 的查找：真的去读文件 */
{
    pw = getpwnam(userbuf);
}

static void w_malloc(void)                  /* 堆操作：brk/mmap 不在 /proc/self/io 视野里 */
{
    for (int i = 0; i < 1000; i++) {
        void *p = malloc(64);
        sink += (unsigned long long) (p != NULL);
        free(p);
    }
}

static void w_fprintf_buffered(void)        /* 100 次 fprintf，故意不 fflush */
{
    for (int i = 0; i < 100; i++) {
        fprintf(out, "x");
    }
}

static void w_fflush(void)                  /* 把用户态缓冲真正交出去 */
{
    fflush(out);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    /* 先从 /etc/passwd 里自己读一个**真实存在**的用户名。
     * 不能写死 getuid()：本容器里 uid 0 并不在 /etc/passwd 中，
     * getpwuid(0) 会返回 NULL（那是「查无此人」，不是出错）。 */
    FILE *pwf = fopen("/etc/passwd", "r");
    if (pwf) {
        char line[256];
        if (fgets(line, sizeof line, pwf)) {
            char *colon = strchr(line, ':');
            if (colon) {
                *colon = '\0';
                size_t n = strlen(line);
                if (n > sizeof userbuf - 1) {      /* 手工截断，避免 -Wformat-truncation */
                    n = sizeof userbuf - 1;
                }
                memcpy(userbuf, line, n);
                userbuf[n] = '\0';
            }
        }
        fclose(pwf);
    }
    printf("=== 0. 本环境参数 ===\n");
    printf("  euid=%d uid=%d，/etc/passwd 第一条记录的用户名 = \"%s\"\n\n",
           (int) geteuid(), (int) getuid(), userbuf);

    /* ④ 要用的可写 FILE*：/dev/null 是最好的靶子（写进去的字节数照样计入 wchar）。
     * 这里必须用 open() + fdopen()：fopen("/dev/null","w") 在本容器里会被拒
     * （它隐式带 O_CREAT|O_TRUNC），而 open(O_WRONLY) 是允许的。 */
    int dn = open("/dev/null", O_WRONLY);
    errno = 0;
    FILE *probe = fopen("/dev/null", "w");
    printf("=== 0b. 先记一个 §3.2 的活教材：库函数封装给你加了什么 ===\n");
    printf("  open(\"/dev/null\", O_WRONLY)   → %s\n", dn >= 0 ? "成功" : "失败");
    printf("  fopen(\"/dev/null\", \"w\")       → %s（errno=%d %s）\n",
           probe ? "成功" : "失败", errno, strerror(errno));
    if (probe) {
        fclose(probe);
    }
    printf("    ↑ fopen 的 \"w\" 隐含 O_WRONLY|O_CREAT|O_TRUNC，比上面多了两位 flag；\n"
           "      本环境恰好只拒绝带 O_CREAT 的打开（/dev 是受限挂载）→ 同一个文件，\n"
           "      open() 能开、fopen() 不能开。所以下面改用 fdopen 绕开这一层。\n\n");
    out = dn >= 0 ? fdopen(dn, "w") : tmpfile();
    if (!out) {
        printf("  可写 FILE* 拿不到，后面的缓冲实验跳过\n");
    }

    printf("=== 1. 底噪：读一次 /proc/self/io 自身要花几个系统调用 ===\n");
    measure("空负载（这就是底噪）", w_nothing);
    printf("\n");

    printf("=== 2. 纯用户态库函数：strcpy / strlen / atoi ×100000 ===\n");
    measure("×100000 轮", w_userlib);
    printf("    ↑ 与上一行的底噪完全相同 → 放大十万倍也没多出一次 I/O。\n"
           "      这就是「库函数不进内核」的可测量形式。\n\n");

    printf("=== 3. snprintf：同样是「格式化」，所以它当然也不进内核 ===\n");
    measure("snprintf ×1000 次到内存", w_snprintf);
    printf("    ↑ 和 fprintf 的区别不在「格式化」，只在**写给了谁**\n\n");

    printf("=== 4. getpwnam()：走 NSS 的名称解析，真的会去读 /etc/passwd ===\n");
    measure("getpwnam() 第 1 次", w_getpwnam);
    measure("getpwnam() 第 2 次", w_getpwnam);
    measure("getpwnam() 第 3 次", w_getpwnam);
    printf("  → 查到用户 = %s，uid = %d，errno = %d\n", pw ? pw->pw_name : "NULL",
           pw ? (int) pw->pw_uid : -1, errno);
    printf("    ↑ 三次的增量都一样、且都明显大于底噪 → 每次调用都重新 open+read 了\n"
           "      /etc/passwd。本环境（glibc 无 nscd/systemd-userdbd）**没有跨调用的\n"
           "      缓存**；反过来说，在 HFT 的日志热路径里调 getpwuid/getpwnam，\n"
           "      每一行日志都要多付几次 read 系统调用。\n"
           "    另注：查无此人时返回 NULL 且**不设 errno** —— 本容器 getpwuid(0) 就是\n"
           "      这种情形，别把它当成 ENOENT 那种错误。\n\n");

    printf("=== 5. stdio 的缓冲：fprintf 不立即产生 write 系统调用 ===\n");
    if (out) {
        measure("fprintf ×100 次（未 fflush）", w_fprintf_buffered);
        measure("fflush() 一次", w_fflush);
        printf("    ↑ 100 次 fprintf 的 Δsyscw 是 0，字节只进了用户态缓冲；\n"
               "      fflush 一次才把它变成 1 次 write —— 这就是「printf 慢」的真正位置。\n"
               "      注意 Δwchar：缓冲里的字节数只有真正 write 出去才计入。\n\n");
    }

    printf("=== 6. malloc/free：/proc/self/io 看不见的「内核活动」 ===\n");
    measure("malloc(64)+free ×1000", w_malloc);
    printf("    ↑ 增量与底噪相同，但这**不能**推出「malloc 没进内核」：\n"
           "      brk / mmap 不是 read/write，压根不在 /proc/self/io 的统计范围内。\n"
           "      用本文件当可移植的「库函数 vs 系统调用」证据时，只对 I/O 类调用有效。\n\n");

    printf("=== 读表要点 ===\n");
    printf("  · Δsyscr / Δsyscw 是「次数」，Δrchar / Δwchar 是「字节数」\n");
    printf("  · 每测一次自带一份底噪（读 /proc/self/io 自身），所以只能比相对差\n");
    printf("  · 同一个「格式化」动作：snprintf→内存 0 次 I/O，fprintf→FILE* 由缓冲决定\n");
    printf("  · NSS 查找一次 = 数次 read；这是 getpwuid 出现在日志热路径里的代价\n");
    if (out) {
        fclose(out);
    }
    return 0;
}
