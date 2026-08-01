# 不链接 libc，纯汇编直接发起 write / exit 系统调用（x86_64 Linux）
# 编译:
#   as --64 -o write_raw_syscall.o write_raw_syscall.s
#   ld -o write_raw_syscall write_raw_syscall.o
# 运行: ./write_raw_syscall
# 观察: strace -e write,exit ./write_raw_syscall
#
# 证明: 系统调用独立于 libc；libc 只是用户态包装器。

        .global _start
        .section .rodata
msg:
        .ascii  "hi via syscall\n"
        .equ    msg_len, . - msg

        .section .text
_start:
        mov     $1, %rax            # __NR_write
        mov     $1, %rdi            # fd = stdout
        lea     msg(%rip), %rsi
        mov     $msg_len, %rdx
        syscall

        mov     $60, %rax           # __NR_exit
        xor     %rdi, %rdi          # status = 0
        syscall
