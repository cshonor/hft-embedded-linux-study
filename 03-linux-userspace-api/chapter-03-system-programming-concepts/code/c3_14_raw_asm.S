/* c3_14_raw_asm.S —— 无 libc 纯汇编：_start 直接 svc/syscall 指令
 *
 * 对应 3.1 节「拆开包装看真相」：write() 是 glibc 薄封装（Pi 实测仅 28 字节，
 * 尾部走 svc #0x0），本文件把封装彻底剥掉——不链任何 libc，进程入口就是 _start，
 * 自己填寄存器、自己执行陷入指令。
 *
 * 编译（gcc 驱动 gas 汇编 + 链接，-nostdlib 甩掉 C 运行时）：
 *   gcc -nostdlib -static -o c3_14_raw_asm c3_14_raw_asm.S
 * 运行：
 *   ./c3_14_raw_asm          # 输出 "raw syscall: no libc\n"，退出码 42
 *   echo $?                  # 42 —— exit(2) 也是我们自己发的
 *
 * aarch64（树莓派 5 实测通过）：调用号放 x8，陷入指令 svc #0
 * x86_64：调用号放 rax，陷入指令 syscall（同仓库 x86_64 约定：rdi/rsi/rdx/r10/r8/r9）
 */

#if defined(__aarch64__)
    .arch armv8-a
    .text
    .global _start
_start:
    /* write(1, msg, 21) */
    mov     x0, #1                  /* fd = 1 (stdout) */
    adr     x1, msg                 /* buf */
    mov     x2, #(msgend - msg)     /* len */
    mov     x8, #64                 /* __NR_write = 64（aarch64 通用表） */
    svc     #0x0                    /* ← 陷入内核，和 glibc 包装底部是同一条指令 */

    /* exit(42) */
    mov     x0, #42
    mov     x8, #93                 /* __NR_exit = 93 */
    svc     #0x0

    .section .rodata
msg:    .ascii "raw syscall: no libc\n"
    .equ    msgend, .

#elif defined(__x86_64__)
    .text
    .global _start
_start:
    /* write(1, msg, 21) */
    mov     $1, %eax                /* __NR_write = 1（x86_64 专用表） */
    mov     $1, %edi                /* fd = stdout */
    lea     msg(%rip), %rsi         /* buf */
    mov     $(msgend - msg), %edx   /* len */
    syscall                         /* ← x86_64 的陷入指令（rcx/rip、r11/rflags 被内核覆写） */

    /* exit(42) */
    mov     $60, %eax               /* __NR_exit = 60 */
    mov     $42, %edi
    syscall

    .section .rodata
msg:    .ascii "raw syscall: no libc\n"
    .equ    msgend, .

#else
#error "only aarch64 / x86_64 supported"
#endif
