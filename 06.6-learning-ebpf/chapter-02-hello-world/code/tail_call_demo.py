#!/usr/bin/env python3
# tail_call_demo.py — 尾调用：按 syscall 号分发到不同 BPF 程序（ch02 §2.2 块8/9）
#
# 运行: sudo python3 tail_call_demo.py，另开终端执行命令
# 输出: execve 触发时打印；其他 syscall 被分发给"静默吞掉"程序
#
# 尾调用的意义：一个入口程序挂一次 raw tracepoint（覆盖全部 syscall），
# 按 syscall 号跳进对应处理程序——省得为几百个 syscall 各挂一个探针。
# 没有登记的 syscall 号：尾调用"失败"并**继续往下执行**（不是异常）。

import ctypes as cctypes
from bcc import BPF

program = r"""
BPF_PROG_ARRAY(syscall, 300);            // PROG_ARRAY map，300 个槽位（> syscall 最大号）

int hello_execve(struct bpf_raw_tracepoint_args *ctx) {
    bpf_trace_printk("execve caught!");
    return 0;
}

int hello_ignore(void *ctx) {            // 噪音 syscall 全塞这里 → 静默吞掉
    return 0;
}

int hello(struct bpf_raw_tracepoint_args *ctx) {
    int opcode = ctx->args[1];           // raw tracepoint 的第2参 = syscall 号
    syscall.call(ctx, opcode);           // BCC 重写为 bpf_tail_call(ctx, syscall, opcode)
    // ↓ 只有尾调用失败（该 opcode 没登记程序）才走到这
    bpf_trace_printk("Another syscall: %d", opcode);
    return 0;
}
"""

b = BPF(text=program)
b.attach_raw_tracepoint(tp="sys_enter", fn_name="hello")

# 登记分发表：execve → hello_execve；多个 entry 可指向同一程序
prog_array = b.get_table("syscall")
prog_array[cctypes.c_int(59)] = cctypes.c_int(b.load_func("hello_execve", BPF.RAW_TRACEPOINT))
# 常见噪音 syscall 静默（21 access / 9 mmap / 10 mprotect / 12 brk / 228 clock_gettime...）
for nr in (21, 9, 10, 12, 228, 96, 202):
    prog_array[cctypes.c_int(nr)] = cctypes.c_int(b.load_func("hello_ignore", BPF.RAW_TRACEPOINT))

print("尾调用分发中：execve 会打印，噪音 syscall 静默，其他报号（Ctrl-C 退出）")
try:
    b.trace_print()
except KeyboardInterrupt:
    pass
