#!/usr/bin/env python3
# hello_perf_output.py — BPF_PERF_OUTPUT：结构化事件推送到用户态（ch02 §2.1 块4/5）
#
# 运行: sudo python3 hello_perf_output.py，另开终端执行命令触发 execve
#
# 数据通道②：perf/ring buffer —— 内核程序把"事件"逐条推给用户态
# 与 map 的分工：map 答"累计多少"（用户态来拉），buffer 答"发生了什么"（内核来推）

from bcc import BPF

program = r"""
BPF_PERF_OUTPUT(output);

struct data_t {
    int pid;
    int uid;
    char command[16];
    char message[12];
};

int hello(void *ctx) {
    struct data_t data = {};                              // ① 零初始化（verifier 要求栈上先初始化）
    char message[12] = "Hello World";

    data.pid = bpf_get_current_pid_tgid() >> 32;          // ② 高32位=PID(tgid)，低32位=线程ID
    data.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;    //    UID 在低 32 位——注意与上行方向相反！
    bpf_get_current_comm(&data.command, sizeof(data.command)); // ③ 当前进程名写进缓冲区
    bpf_probe_read_kernel(&data.message, sizeof(data.message), message);
    output.perf_submit(ctx, &data, sizeof(data));         // ④ 提交进 ring buffer
    return 0;
}
"""

b = BPF(text=program)
syscall = b.get_syscall_fnname("execve")
b.attach_kprobe(event=syscall, fn_name="hello")


def print_event(cpu, data, size):
    data = b["output"].event(data)          # 字节流 → 结构体
    print(f"{data.pid:>7} {data.uid:>5} {data.command.decode():<16} {data.message.decode()}")


b["output"].open_perf_buffer(print_event)   # 注册回调
print(f"{'PID':>7} {'UID':>5} {'COMM':<16} MESSAGE  （Ctrl-C 退出）")
try:
    while True:
        b.perf_buffer_poll()                # 底层是 epoll：无事件时休眠，零忙等
except KeyboardInterrupt:
    pass
