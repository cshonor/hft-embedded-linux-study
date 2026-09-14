/* TLPI 第 03 章 §3.4（+ §3.1 的「参数怎么传进内核」）—— EFAULT：把非法指针交给内核会怎样
 *
 * 内核**不能信任**用户指针：它可能指向内核地址、指向别的进程、或根本没映射。
 * 所以每个带指针参数的系统调用都要先 access_ok() 校验、再 copy_from_user() 拷贝。
 * 校验失败时系统调用返回 -1 + EFAULT(14)（裸 syscall 拿到的是 -14）。
 *
 * 本 demo 逐种「坏指针」实测，并把 errno 与裸 syscall 的返回值并排打印，
 * 顺带把 EFAULT 与其它「看着像但完全不同」的错误码分开（EFAULT vs EBADF vs EINVAL）。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_13 c3_13_efault.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

/* printf 的 %-34s 是按**字节**补齐的，而一个汉字 3 字节、显示只占 2 列 →
 * 直接用会参差不齐。这里自己数显示宽度：ASCII 记 1 列，非 ASCII 记 2 列。 */
static int disp_width(const char *s)
{
    int w = 0;
    for (; *s; s++) {
        unsigned char c = (unsigned char) *s;
        if (c < 0x80) {
            w += 1;
        } else if ((c & 0xC0) != 0x80) {   /* UTF-8 续字节不重复计数 */
            w += 2;
        }
    }
    return w;
}

static void pad(const char *s, int width)
{
    fputs(s, stdout);
    for (int i = disp_width(s); i < width; i++) {
        putchar(' ');
    }
}

static void probe(const char *tag, const void *p, size_t n)
{
    /* ① glibc 包装：-1 + errno */
    errno = 0;
    ssize_t r = write(STDOUT_FILENO, p, n);
    int e1 = errno;

    /* ② 裸 syscall 指令：直接拿内核的 -errno（不经过 glibc 翻译） */
    errno = 0;
    long r2;
    __asm__ volatile ("syscall"
                      : "=a"(r2)
                      : "a"((long) SYS_write), "D"((long) STDOUT_FILENO),
                        "S"((long) p), "d"((long) n)
                      : "rcx", "r11", "memory");
    int e2 = errno;

    printf("  ");
    pad(tag, 30);
    printf("write()=%2zd errno=%-3d %-14s | raw=%4ld errno=%d\n",
           r, e1, e1 ? strerror(e1) : "(成功)", r2, e2);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    char ok[] = "[3.13] 合法缓冲区，正常写出\n";
    char *unmapped = (char *) 0x1;                     /* 极低地址，必然未映射 */
    char *kernel_ish = (char *) 0xffffffff81000000ULL;  /* 看着像内核地址 */

    /* 只读映射页：先写内容，再把写权限去掉。write(2) 只需要对用户缓冲有**读**权限 */
    void *map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map != MAP_FAILED) {
        memcpy(map, "RO-PAGE\n", 8);
        mprotect(map, 4096, PROT_READ);
    }

    printf("=== 各种指针喂给 write(1, p, n) 的结果 ===\n");
    probe("合法栈缓冲区", ok, strlen(ok));
    probe("NULL（n=10）", NULL, 10);
    probe("NULL（n=0）", NULL, 0);            /* n=0 时内核对 buf 不做检查 */
    probe("(char*)0x1（未映射）", unmapped, 10);
    probe("(char*)0x1（n=0）", unmapped, 0);
    probe("0xffffffff81000000（越界）", kernel_ish, 10);
    probe("只读映射页（无写权限）", map, 8);   /* 只能读的页，仍然能作为 write 的源 */

    printf("\n=== 逐条结论 ===\n");
    printf("  1. NULL 与野指针都得到 EFAULT(14)，**不是** SIGSEGV —— 内核在拷贝前就拦下了\n");
    printf("     这一点很重要：用户态传坏指针不会把内核搞崩，只会拿到一个错误码\n");
    printf("  2. n=0 是特例：没有任何字节要拷，内核通常不再校验 buf（Linux 上 write(fd,NULL,0)=0）\n");
    printf("     所以「n=0 时指针可以随便传」是**依赖实现**的行为，别写进代码\n");
    printf("  3. raw 那一列拿到的是 -14，而 glibc 那列是 -1 + errno=14\n");
    printf("     同一件事的两种表示，正是 §3.1 讲的「内核用 [-4095,-1] 表示错误」\n");
    printf("  4. 只读映射页这一行说明：write(2) 只**读**你的缓冲，不写它。\n");
    printf("     所以 mmap(PROT_READ) 的内存可以直接作为 write/send 的源\n");
    printf("  5. EFAULT ≠ EBADF ≠ EINVAL：\n");
    printf("     EFAULT = 指针/地址不合法（内存 bug，先查指针生命周期）\n");
    printf("     EBADF  = fd 不是有效的打开文件描述符（fd 管理 bug）\n");
    printf("     EINVAL = 参数取值不在允许范围（调用方逻辑 bug）\n");

    if (map != MAP_FAILED) {
        munmap(map, 4096);
    }
    printf("\n=== 为什么内核必须拷贝：本节的机制落点 ===\n");
    printf("  内核拿到用户指针后不能直接解引用，必须先把数据 copy_from_user 进内核缓冲；\n");
    printf("  拷贝那一刻用户态若改了这块内存，内核用的仍是已拷贝的那份副本 → 无竞争。\n");
    printf("  代价就是要付一次内存拷贝：这是「大缓冲 write 有开销」的来源之一，\n");
    printf("  也是零拷贝方案（mmap / sendfile / splice）存在的理由。\n");
    return 0;
}
