/* TLPI 第 2 章 §2.1 —— 完全不依赖 glibc：手写 svc 指令裸调 write/exit（AArch64）
 *
 * 编译运行（Linux arm64，如 Raspberry Pi 5）：
 *   gcc -nostdlib -static c2_1_nolibc_arm64.S -o c2_1_nolibc_arm64
 *   ./c2_1_nolibc_arm64
 *   ldd ./c2_1_nolibc_arm64      # 期望：not a dynamic executable
 *   strace ./c2_1_nolibc_arm64   # 期望：整个进程只有 write + exit 两次系统调用
 *
 * 与 x86-64 版的三点差异（ABI 不同，语义相同）：
 *   ① 陷入指令：x86-64 用 syscall，AArch64 用 svc #0
 *   ② 号码寄存器：x86-64 用 rax，AArch64 用 x8
 *   ③ 系统调用号不同：AArch64 没有 write=1，而是 write=64 / exit=93
 *     （arch/arm64 走 asm-generic 表：include/uapi/asm-generic/unistd.h）
 *
 * 已实测：Raspberry Pi 5（Debian 13，kernel 6.18，gcc 14.2）——见笔记正文输出。
 */
    .section .rodata
msg:
    .ascii "[nolibc-arm64] hello from raw svc instruction, zero glibc\n"
    .set msg_len, . - msg

    .text
    .globl _start
_start:
    /* write(1, msg, msg_len) */
    mov     x8, 64              /* __NR_write = 64（asm-generic/unistd.h） */
    mov     x0, 1               /* fd = STDOUT_FILENO */
    adrp    x1, msg
    add     x1, x1, :lo12:msg   /* buf */
    mov     x2, msg_len         /* count */
    svc     #0                  /* EL0 → EL1 陷入内核 */

    /* exit(0) */
    mov     x8, 93              /* __NR_exit = 93 */
    mov     x0, 0
    svc     #0
