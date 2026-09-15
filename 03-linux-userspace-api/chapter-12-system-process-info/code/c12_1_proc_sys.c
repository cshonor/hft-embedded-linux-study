/* c12_1_proc_sys.c — Ch12 §12.1.3：/proc/sys 是「把内核参数当文件」
 *
 * TLPI §12.1.3 的核心结论：改内核参数**不需要**专门系统调用，就是
 * `open` + `write` 一个文本文件。本程序把这件事的细节全钉住：
 *   ① /proc/sys 是一棵按子系统分层的树（kernel/ fs/ vm/ net/…）；
 *   ② 每个 knob 的读写权限不一样（0644 可读写 / 0444 只读）；
 *   ③ **对只读打开的 fd 调 write 是 EBADF**（不是 EACCES）；
 *   ④ 写完要重新 open（或 lseek 回 0）才读得到新值 —— 这就是原书
 *      Listing 12-1 在 write 之前插一句 lseek(fd, 0, SEEK_SET) 的原因；
 *   ⑤ 内核会**校验取值范围**，写非法值返回 EINVAL 且不改动原值；
 *   ⑥ `sysctl(2)` 系统调用已被删除（Linux 5.5），只能走文件接口。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_1_proc_sys c12_1_proc_sys.c
 * 取材： man-pages 6.19 proc_sys(5) 全文 + sysctl(2) 的 NOTES
 *       Linux v6.6 fs/proc/proc_sysctl.c（proc_sys_open / proc_sys_call_handler）
 *                      fs/proc/generic.c（proc_ops 的 read/write）
 *                      kernel/sysctl.c（pid_max 的 min/max：300 < v <= 4194304）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

/* 读一个 knob，把内容放进 out（去掉行尾换行）；返回实际长度，-1 表示失败 */
static ssize_t read_knob(const char *path, char *out, size_t cap)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1;
    ssize_t n = read(fd, out, cap - 1);
    close(fd);
    if (n < 0)
        return -1;
    out[n] = '\0';
    while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r'))
        out[--n] = '\0';
    return n;
}

/* 打开一次、读一次、然后**不清偏移**直接写；再 lseek 回 0 写一次 */
static void write_probe(const char *path, const char *newval)
{
    printf("  -- 写试探：%s ← \"%s\" --\n", path, newval);

    /* (a) 只读打开再写 */
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("     open(O_RDONLY) 失败 errno=%d(%s)\n", errno, strerror(errno));
    } else {
        errno = 0;
        ssize_t w = write(fd, newval, strlen(newval));
        printf("     对 O_RDONLY 的 fd write() = %zd errno=%d(%s)\n",
               w, w < 0 ? errno : 0, w < 0 ? strerror(errno) : "ok");
        close(fd);
    }

    /* (b) 读写打开：先 read（偏移推到末尾），不 lseek 直接写 */
    fd = open(path, O_RDWR);
    if (fd == -1) {
        printf("     open(O_RDWR)   失败 errno=%d(%s)"
               "  ← 非特权进程在这里就止步了\n", errno, strerror(errno));
        return;
    }
    char buf[64];
    ssize_t r = read(fd, buf, sizeof(buf) - 1);
    if (r < 0) {
        printf("     read 失败 errno=%d(%s)\n", errno, strerror(errno));
        close(fd);
        return;
    }
    buf[r] = '\0';
    printf("     打开后 read() 拿到 \"%.*s\"（偏移已到 %zd）\n",
           (int) (r && buf[r - 1] == '\n' ? r - 1 : r), buf, r);

    errno = 0;
    ssize_t w = write(fd, newval, strlen(newval));
    printf("     不清偏移直接 write() = %zd errno=%d(%s)\n",
           w, w < 0 ? errno : 0, w < 0 ? strerror(errno) : "ok");
    printf("       ⚠️ 偏移不为 0 时内核**静默忽略**这次写入，却照样返回成功：\n");
    printf("          fs/proc/proc_sysctl.c 的 proc_first_pos_non_zero_ignore()\n");
    printf("          在 sysctl_writes_strict=1（默认）时直接 return，count 照收。\n");
    printf("          所以「write 返回成功」≠「值被改了」。\n");

    if (lseek(fd, 0, SEEK_SET) == (off_t) -1) {
        printf("     lseek(fd, 0, SEEK_SET) 失败 errno=%d(%s)\n", errno, strerror(errno));
    } else {
        errno = 0;
        w = write(fd, newval, strlen(newval));
        printf("     lseek 回 0 再 write() = %zd errno=%d(%s)\n",
               w, w < 0 ? errno : 0, w < 0 ? strerror(errno) : "ok");
    }
    close(fd);

    char after[64];
    if (read_knob(path, after, sizeof(after)) >= 0)
        printf("     写后重新读 = \"%s\"\n", after);
}

int main(void)
{
    /* ---------- ① 树形结构：列出几个子系统的 knob ---------- */
    printf("== ① /proc/sys 是按子系统分层的树 ==\n");
    static const char *KNOBS[] = {
        "/proc/sys/kernel/pid_max",
        "/proc/sys/kernel/threads-max",
        "/proc/sys/kernel/ngroups_max",
        "/proc/sys/kernel/randomize_va_space",
        "/proc/sys/kernel/osrelease",
        "/proc/sys/kernel/version",
        "/proc/sys/vm/overcommit_memory",
        "/proc/sys/vm/swappiness",
        "/proc/sys/fs/file-max",
        "/proc/sys/fs/nr_open",
        "/proc/sys/net/core/somaxconn",
    };
    for (size_t i = 0; i < sizeof(KNOBS) / sizeof(KNOBS[0]); i++) {
        char v[128];
        if (read_knob(KNOBS[i], v, sizeof(v)) >= 0)
            printf("  %-42s = %s\n", KNOBS[i], v);
        else
            printf("  %-42s 读失败 errno=%d(%s)\n", KNOBS[i], errno, strerror(errno));
    }
    printf("  tip: 树根就是 `/proc/sys/`，`sysctl -a` 只是把这棵树全部走一遍。\n\n");

    /* ---------- ② 权限：可写 vs 只读 ---------- */
    printf("== ② 权限位不代表一切 ==\n");
    struct { const char *p; int flag; const char *how; } FL[] = {
        {"/proc/sys/kernel/pid_max",   O_RDONLY, "O_RDONLY"},
        {"/proc/sys/kernel/pid_max",   O_WRONLY, "O_WRONLY"},
        {"/proc/sys/kernel/pid_max",   O_RDWR,   "O_RDWR"},
        {"/proc/sys/kernel/osrelease", O_WRONLY, "O_WRONLY"},
        {"/proc/sys/kernel/version",   O_WRONLY, "O_WRONLY"},
    };
    for (size_t i = 0; i < sizeof(FL) / sizeof(FL[0]); i++) {
        errno = 0;
        int fd = open(FL[i].p, FL[i].flag);
        printf("  open(%-32s, %-9s) = %-3d errno=%d(%s)\n",
               FL[i].p, FL[i].how, fd, fd < 0 ? errno : 0,
               fd < 0 ? strerror(errno) : "ok");
        if (fd >= 0)
            close(fd);
    }
    printf("  ⚠️ 只读 knob（osrelease / version）用 O_WRONLY 打开就失败；\n");
    printf("     但**用 O_RDONLY 打开一个可写 knob 会成功**，失败发生在 write 那一刻，\n");
    printf("     且错误码是 EBADF（fd 不是为写打开的），不是 EACCES。\n\n");

    /* ---------- ③ 写入：非法值 / 正确值 ---------- */
    printf("== ③ 写 /proc/sys/kernel/pid_max ==\n");
    char orig[64];
    if (read_knob("/proc/sys/kernel/pid_max", orig, sizeof(orig)) < 0) {
        printf("  读 pid_max 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("  当前值 = \"%s\"\n", orig);
    printf("  先写一个**越界**的值，验证内核会拒绝：\n");
    write_probe("/proc/sys/kernel/pid_max", "0");
    printf("  再写回**原值**（等价于空操作，但能验证写通道是通的）：\n");
    write_probe("/proc/sys/kernel/pid_max", orig);
    printf("\n");

    /* ---------- ④ sysctl(2) 已经不存在 ---------- */
    printf("== ④ sysctl(2) 系统调用已删除 ==\n");
#ifdef SYS_sysctl
    errno = 0;
    long r = syscall(SYS_sysctl, (void *) 0, 0, (void *) 0, (void *) 0, 0);
    printf("  本机 glibc 头里有 SYS_sysctl=%d；syscall() 返回 %ld errno=%d(%s)\n",
           SYS_sysctl, r, errno, strerror(errno));
    printf("  → 若 errno=ENOSYS(38)，说明这个 syscall 已被内核移除。\n");
#else
    printf("  本机 glibc 头里**没有** SYS_sysctl —— 这个系统调用自 Linux 5.5 起已删除，\n");
    printf("  glibc 也不再导出包装函数。旧代码里的 sysctl(3) 一律应改写成\n");
    printf("  open/write(\"/proc/sys/...\")。\n");
#endif
    printf("  ⚠️ 另一条路：glibc 的 `sysctl` 名字现在只作为**兼容符号**存在，\n");
    printf("     成功调用也不会得到数据（返回 0 但 buffer 未填），比报错更难查。\n");
    return EXIT_SUCCESS;
}
