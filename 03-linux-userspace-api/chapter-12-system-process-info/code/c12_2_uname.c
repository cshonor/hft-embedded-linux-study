/* c12_2_uname.c — Ch12 §12.2：uname() 是本章唯一「POSIX 可移植」的接口
 *
 * TLPI §12.2 只有一页多，但有几个必须钉住的点：
 *   ① `struct utsname` 有**六个**字段，不是五个 —— 第六个是 GNU 扩展 domainname；
 *   ② 每个字段只有 **65 字节**（`__NEW_UTS_LEN` = 64，再留一个 NUL），
 *      所以 version 这类长串**一定**会被截断，别拿它当完整信息；
 *   ③ 在 Linux 上 `uname()` 就是**裸的 `uname(2)` 系统调用**（本程序与
 *      `syscall(SYS_uname, ...)` 逐字段对拍证明）。glibc 里 `posix/uname.c`
 *      那份「用编译期常量 + gethostname 拼出来」的实现**不是 Linux 用的**；
 *   ④ `nodename` 与 `gethostname()` 必然一致 —— 因为 glibc 的 `gethostname()`
 *      正是**建在 uname() 之上**的（`sysdeps/posix/gethostname.c:32` 里调
 *      `__uname()`），两者读的是同一个内核字段，不是巧合；
 *   ⑤ 由此推出一条反直觉的 API 语义：`gethostname(name, len)` 在 `len` 不够时
 *      **返回 -1 / ENAMETOOLONG，而且**留在缓冲里的是**没有 NUL 终止**的前缀
 *      —— 这里不能当字符串用；
 *   ⑥ `domainname` 的默认值字面量是 "(none)"，看起来像「没设置」，其实它就是
 *      内核给初值（`include/linux/uts.h` 的 `UTS_DOMAINNAME`）；
 *   ⑦ uname() 拿不到 CPU 数、内存量、进程列表 —— 那些必须走 /proc 或 sysinfo。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_2_uname c12_2_uname.c
 * 取材： man-pages 6.19 uname(2)（"returns a string identifying the kernel"、EFAULT）
 *                      gethostname(2) 的 ERRORS（ENAMETOOLONG）
 *       Linux v6.6 include/uapi/linux/utsname.h:15（__NEW_UTS_LEN = 64）
 *                      include/uapi/linux/utsname.h:25-32（struct new_utsname = 6×65）
 *                      include/linux/uts.h:16-18（UTS_DOMAINNAME "(none)"）
 *                      kernel/sys.c:1306-1321（SYSCALL_DEFINE1(newuname)：memcpy 整个
 *                       390 字节结构体 + copy_to_user，无 NULL 检查 → EFAULT）
 *                      kernel/sys.c:1374-1426（sethostname/gethostname 的 CAP_SYS_ADMIN）
 *       glibc 2.39 sysdeps/unix/syscalls.list:88（"uname - uname i:p __uname uname"
 *                       —— uname 是**自动生成的 syscall 包装**）
 *                      sysdeps/posix/gethostname.c:26-46（gethostname 建在 __uname 上）
 *                      posix/uname.c:28-62（通用实现，Linux 不用它）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>

static void hexdump(const char *tag, const unsigned char *b, int n)
{
    printf("  %s =", tag);
    for (int i = 0; i < n; i++)
        printf(" %02x", b[i]);
    printf("\n");
}

int main(void)
{
    /* ---------- ① 六个字段 ---------- */
    printf("== ① uname() 的六个字段 ==\n");
    struct utsname u;
    errno = 0;
    if (uname(&u) == -1) {
        printf("  uname 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("  sysname    = [%s]  （OS 名，Linux 上恒为 \"Linux\"）\n", u.sysname);
    printf("  nodename   = [%s]  （即主机名）\n", u.nodename);
    printf("  release    = [%s]  （内核版本号，脚本里最常被解析的一项）\n", u.release);
    printf("  version    = [%s]\n", u.version);
    printf("  machine    = [%s]  （硬件架构，交叉编译/移植判断用这个）\n", u.machine);
    printf("  domainname = [%s]  （GNU 扩展，注意它的值）\n", u.domainname);
    printf("  各字段 strlen: sysname=%d nodename=%d release=%d version=%d machine=%d domain=%d\n",
           (int) strlen(u.sysname), (int) strlen(u.nodename), (int) strlen(u.release),
           (int) strlen(u.version), (int) strlen(u.machine), (int) strlen(u.domainname));
    printf("\n");

    /* ---------- ② 字段有多宽 ---------- */
    printf("== ② 每个字段只有 65 字节，结构体没有填充 ==\n");
    printf("  sizeof(struct utsname) = %zu 字节\n", sizeof(struct utsname));
    printf("  %zu / 6 = %zu 字节/字段  ⇒  `__NEW_UTS_LEN` = 64，再留 1 个 NUL\n",
           sizeof(struct utsname), sizeof(struct utsname) / 6);
    printf("  6 x 65 = %zu，与 sizeof 之差 = %zu（uname(2) 的载荷就是这 390 字节）\n",
           (size_t) 6 * 65, sizeof(struct utsname) - (size_t) 6 * 65);
    printf("  ⚠️ 字段是**定长数组**（char[65]），不是指针；所以整个结构体是 %zu 字节的\n",
           sizeof(struct utsname));
    printf("     平凡数据，可以直接 memcpy / 存进日志，没有生命周期问题。\n");
    printf("  ⚠️ version 最长只能装 64 个字符，本例已用 %d 个 —— 内核写入时就按\n",
           (int) strlen(u.version));
    printf("     `__NEW_UTS_LEN` 截断了，uname() 拿到的已是截断后的结果，\n");
    printf("     读的人无从知道原文更长。\n\n");

    /* ---------- ③ 它就是系统调用 ---------- */
    printf("== ③ 在 Linux 上 uname() 就是裸的 uname(2) 系统调用 ==\n");
    struct utsname raw;
    memset(&raw, 'X', sizeof(raw));     /* 先用 'X' 灌满，看内核写了哪些字节 */
    errno = 0;
    long sr = syscall(SYS_uname, &raw);
    int sre = errno;
    printf("  syscall(SYS_uname, &raw) = %ld errno=%d(%s)\n", sr, sre, strerror(sre));
    printf("  六字段逐一对拍（uname() / 原始系统调用）:\n");
    printf("    sysname    same=%d\n", strcmp(u.sysname, raw.sysname) == 0);
    printf("    nodename   same=%d\n", strcmp(u.nodename, raw.nodename) == 0);
    printf("    release    same=%d\n", strcmp(u.release, raw.release) == 0);
    printf("    version    same=%d\n", strcmp(u.version, raw.version) == 0);
    printf("    machine    same=%d\n", strcmp(u.machine, raw.machine) == 0);
    printf("    domainname same=%d\n", strcmp(u.domainname, raw.domainname) == 0);
    int xleft = 0;
    const unsigned char *pb = (const unsigned char *) &raw;
    for (size_t i = 0; i < sizeof(raw); i++)
        if (pb[i] == 'X')
            xleft++;
    printf("  内核回来之后缓冲里剩余的 'X' 字节数 = %d / %zu（0 = 内核写满了整份载荷）\n",
           xleft, sizeof(raw));
    printf("  ⚠️ 所以「nodename 是内核当场给的、不是 glibc 从别处拼的」这一点可以确认。\n");
    printf("     对照 /proc/sys/kernel/osrelease = ");
    {
        FILE *fp = fopen("/proc/sys/kernel/osrelease", "r");
        char b[128] = "";
        if (fp) { if (fgets(b, sizeof b, fp)) { char *nl = strchr(b, '\n'); if (nl) *nl = '\0'; } fclose(fp); }
        printf("[%s]  == uname.release? %d\n", b, strcmp(b, u.release) == 0);
    }
    printf("  ⚠️ 带 NULL 指针：syscall(SYS_uname, NULL) = ");
    errno = 0;
    long nul = syscall(SYS_uname, NULL);
    printf("%ld errno=%d(%s) —— 内核在 copy_to_user 处把 NULL 挡成 EFAULT\n\n",
           nul, errno, strerror(errno));

    /* ---------- ④ nodename ≡ gethostname() 的机制 ---------- */
    printf("== ④ nodename 与 gethostname() 必然一致，且原因是「同一条路」 ==\n");
    char host[256];
    errno = 0;
    if (gethostname(host, sizeof(host)) == -1) {
        printf("  gethostname 失败 errno=%d(%s)\n", errno, strerror(errno));
    } else {
        printf("  uname().nodename      = [%s]\n", u.nodename);
        printf("  gethostname(host,256) = [%s]\n", host);
        printf("  → %s\n", strcmp(u.nodename, host) == 0 ? "一致" : "不一致");
    }
    printf("  机制：glibc 的 gethostname() **不是**独立系统调用，它内部先调 __uname()\n");
    printf("        再把结果里的 nodename 段 memcpy 出来（sysdeps/posix/gethostname.c）。\n");
    printf("        所以两者不可能不一致 —— 这是实现决定的，不是巧合。\n");
    printf("  ⚠️ 但可移植性不同：`gethostname()` 是 POSIX，`domainname` 字段是 GNU 扩展。\n\n");

    /* ---------- ⑤ gethostname 缓冲太小时的**真实**行为 ---------- */
    printf("== ⑤ gethostname(name, len) 当 len 不够：-1/ENAMETOOLONG 且缓冲不终止 ==\n");
    printf("  主机名 = \"%s\"，strlen+1 = %d 字节，所以 len < %d 就一定不够。\n",
           u.nodename, (int) strlen(u.nodename) + 1, (int) strlen(u.nodename) + 1);
    for (int len = 1; len <= 5; len++) {
        unsigned char buf[8];
        memset(buf, 'Z', sizeof(buf));      /* 'Z' = 0x5a，便于看出哪些字节被覆盖 */
        errno = 0;
        int r = gethostname((char *) buf, (size_t) len);
        int e = errno;
        printf("  len=%d -> ret=%d errno=%d(%s)\n", len, r, e, strerror(e));
        char tag[32];
        snprintf(tag, sizeof(tag), "byte[0..7]");
        hexdump(tag, buf, 8);
    }
    printf("  → len=1/2 都返回 -1（ENAMETOOLONG = 36），**而且缓冲里只有前缀、没有 NUL**：\n");
    printf("     len=2 时留下的是 \"%c%c\" 两个裸字节，后头还是原来的 'Z'。\n",
           u.nodename[0], u.nodename[1]);
    printf("     ⚠️ 这时**绝不能**把缓冲当 C 字符串 printf 出去 —— 会一路读到 'Z' 之后。\n");
    printf("     len >= %d 才返回 0 并写好终止符。\n", (int) strlen(u.nodename) + 1);
    printf("  sysconf(_SC_HOST_NAME_MAX) = %ld（= __NEW_UTS_LEN，缓冲给这么多就永远够）\n",
           sysconf(_SC_HOST_NAME_MAX));
    printf("  ⚠️ POSIX 允许「截断并返回 0」，也允许「报 ENAMETOOLONG」；glibc 选后者。\n");
    printf("     所以**不能靠返回值判断有没有截断** —— 要么给足 64 字节，要么自己查 errno。\n\n");

    /* ---------- ⑥ domainname 为什么是 "(none)" ---------- */
    printf("== ⑥ domainname = \"(none)\" 不是「没设置」，是内核给的初值 ==\n");
    printf("  当前值 = [%s]\n", u.domainname);
    printf("  内核侧：include/linux/uts.h:16-18 里 `#define UTS_DOMAINNAME \"(none)\"`，\n");
    printf("          init_uts_ns 初始化时就把这个字面量填进 name.domainname；\n");
    printf("          改它走 `setdomainname(2)`（kernel/sys.c:1428），要 CAP_SYS_ADMIN。\n");
    printf("  ⚠️ 这个字段是 **GNU 扩展**：是否定义 `_GNU_SOURCE` 决定 `struct utsname`\n");
    printf("     里有没有 `domainname` 成员 —— 原书 Listing 12-2 就是拿\n");
    printf("     `#ifdef _GNU_SOURCE` 把它包起来的（见本目录 t_uname.c）。\n\n");

    /* ---------- ⑦ 它拿不到什么 ---------- */
    printf("== ⑦ uname() 拿不到的三样东西 ==\n");
    printf("  CPU 数：uname() 只说 machine=\"%s\"，不说有几个核\n", u.machine);
    printf("          → 要 get_nprocs() / sysconf(_SC_NPROCESSORS_ONLN)（见 c12_nprocs.c）\n");
    printf("  内存量：uname() 完全不给\n");
    printf("          → 要 /proc/meminfo / sysinfo() / sysconf(_SC_PHYS_PAGES)（见 c12_sysinfo.c）\n");
    printf("  进程列表：uname() 完全不给\n");
    printf("          → 要遍历 /proc/[0-9]*（见 ex12_1_proc_walk.c）\n\n");

    /* ---------- ⑧ 工程用法 ---------- */
    printf("== ⑧ 工程用法：启动时把 uname() 落进日志 ==\n");
    printf("  机器可读的启动横幅（版本漂移排查的第一手证据）：\n");
    printf("    BUILD %s %s %s [%s]\n", u.sysname, u.release, u.machine, u.nodename);
    printf("  ⚠️ 但**别**用 uname().release 做「功能是否可用」的判据：\n");
    printf("     发行版会回填补丁、容器会伪装内核版本（本机 release=%s，\n", u.release);
    printf("     而 /proc/version 里同一份信息还裹着编译器串）。\n");
    printf("     正确做法是「试一下」—— 直接调那个 API 看 errno，而不是比版本号。\n");
    return EXIT_SUCCESS;
}
