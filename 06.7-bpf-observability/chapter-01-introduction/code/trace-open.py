#!/usr/bin/env python3
# trace-open.py — BCC 追踪 openat（02-ch01 §9 块5）
# 《BPF Performance Tools》第 1 章：同一件事 bpftrace 一行 vs BCC 完整程序
#
# 运行: sudo python3 trace-open.py，另开终端随便操作
# 对照: 06.6/ch15 与 ../code/scripts/ 里的 bpftrace 单行版本
from bcc import BPF

b = BPF(text=r'''
#include <uapi/linux/ptrace.h>
TRACEPOINT_PROBE(syscalls, sys_enter_openat) {
    bpf_trace_printk("%s\n", args->filename);   // 内核态：C，事件发生时执行
    return 0;
}
''')

print("追踪 sys_enter_openat（Ctrl-C 退出）")
b.trace_print()                                   # 用户态：Python，收事件打印
