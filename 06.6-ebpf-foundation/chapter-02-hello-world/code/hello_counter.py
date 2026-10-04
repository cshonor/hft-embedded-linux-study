#!/usr/bin/env python3
# hello_counter.py — BPF_HASH map：按 UID 统计 execve 次数（ch02 §2.0 块8 / §2.1 块2/3）
#
# 运行: sudo python3 hello_counter.py
# 每 2 秒打印一次各 UID 的 execve 计数，然后清零重新统计（读差口径）
#
# 数据通道①：map —— 内核程序写、用户态周期性读（聚合统计的标准形态）

from bcc import BPF
from time import sleep

program = r"""
BPF_HASH(counter_table);                    // BCC 宏：默认 u64 key → u64 value

int hello(void *ctx) {
    u64 uid;
    u64 counter = 0;
    u64 *p;

    uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;  // ① 低32位=UID，高32位=GID（掩掉）
    p = counter_table.lookup(&uid);                // ② 查表，返回 value 指针；无命中返回 NULL
    if (p != 0) { counter = *p; }                  // ③ 必须判空！（verifier 要求）
    counter++;
    counter_table.update(&uid, &counter);          // ④ 写回
    return 0;
}
"""

b = BPF(text=program)
syscall = b.get_syscall_fnname("execve")
b.attach_kprobe(event=syscall, fn_name="hello")

print("按 UID 统计 execve（每 2 秒一轮，Ctrl-C 退出）")
try:
    while True:
        sleep(2)
        for k, v in b["counter_table"].items():
            print(f"UID {k.value}: {v.value}")
        b["counter_table"].clear()      # 清零重新统计
except KeyboardInterrupt:
    pass
