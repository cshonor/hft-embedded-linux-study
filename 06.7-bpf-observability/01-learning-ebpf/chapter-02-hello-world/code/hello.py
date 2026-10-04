#!/usr/bin/env python3
# hello.py — BCC Hello World（ch02 §2.0 块2 / §2.1 块1）
# 出自《Learning eBPF》第 2 章第一个例子
#
# 运行: sudo python3 hello.py，然后另开一个终端随便敲命令
# 输出: /sys/kernel/tracing/trace_pipe 里的 "Hello World!"
# 退出: Ctrl-C

from bcc import BPF
import sys

program = r"""
int hello(void *ctx) {
    bpf_trace_printk("Hello World!");
    return 0;
}
"""

b = BPF(text=program)                            # ① BCC 现场编译 C 字符串并加载进内核
syscall = b.get_syscall_fnname("execve")         # ② syscall 内核函数名随架构不同（x86: __x64_sys_execve）
b.attach_kprobe(event=syscall, fn_name="hello")  # ③ 挂 kprobe

try:
    b.trace_print()                              # ④ 无限循环读 trace_pipe
except KeyboardInterrupt:
    sys.exit(0)
