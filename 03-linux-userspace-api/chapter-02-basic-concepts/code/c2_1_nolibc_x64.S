/* TLPI 第 2 章 §2.1 —— 完全不依赖 glibc：手写 syscall 指令裸调 write/exit（x86-64）
 *
 * 编译运行（Linux x86-64）：
 *   gcc -nostdlib -static c2_1_nolibc_x64.S -o c2_1_nolibc_x64
 *   ./c2_1_nolibc_x64
 *   ldd ./c2_1_nolibc_x64        # 期望：not a dynamic executable（连 libc.so.6 都没链接）
 *
 * 三方对比（本节要钉死的第二层事实）：
 *   write(1, buf, len)            glibc 薄封装：本体是 glibc 函数，可能带取消点等逻辑
 *   syscall(SYS_write, 1, ...)    glibc 通用小封装：本体仍是 glibc 函数，只做参数整理
 *   本文件                        连 glibc 都没有：_start 里直接摆寄存器 + syscall 指令
 *
 * 注意：-nostdlib 意味着没有 CRT 启动文件，入口必须是 _start（不是 main），
 *       退出必须用 SYS_exit(60) 系统调用，不能 return（没有栈上的返回地址）。
 * 系统调用号来源：arch/x86/entry/syscalls/syscall_64.tbl（write=1, exit=60）
 */
    .intel_syntax noprefix

    .section .rodata
msg:
    .ascii "[nolibc-x64] hello from raw syscall instruction, zero glibc\n"
    .set msg_len, . - msg

    .text
    .globl _start
_start:
    /* write(1, msg, msg_len) —— 号码/参数摆寄存器，一条 syscall 陷入内核 */
    mov     rax, 1              /* SYS_write = 1（syscall_64.tbl） */
    mov     rdi, 1              /* fd = STDOUT_FILENO */
    lea     rsi, [rip + msg]    /* buf */
    mov     rdx, msg_len        /* count（字节数） */
    syscall                     /* 就这一条指令：用户态 → 内核态 */

    /* exit(0) —— 没有 glibc 的 exit() 可用，直接发 SYS_exit */
    mov     rax, 60             /* SYS_exit = 60 */
    xor     rdi, rdi            /* status = 0 */
    syscall
