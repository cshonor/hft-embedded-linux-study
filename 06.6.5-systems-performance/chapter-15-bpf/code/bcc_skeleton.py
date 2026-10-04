#!/usr/bin/env python3
# bcc_skeleton.py — BCC Python 最小骨架（ch15 §15.1 块3 补全为可运行）
#
# 笔记里是"伪代码"片段；这里补全为完整可执行形态：
#   BPF_HASH map + kprobe attach + 定时读 map 渲染
# 演示: 统计 vfs_read 被各进程调用的次数（map: pid → count）
#
# 运行: sudo python3 bcc_skeleton.py [秒数，默认 10]
# 依赖: python3-bcc（本机 bcc 0.29.1 ✓）；需要 root（加载 BPF + attach kprobe）

import sys
from time import sleep

SECS = int(sys.argv[1]) if len(sys.argv) > 1 else 10

prog = r"""
#include <uapi/linux/ptrace.h>

BPF_HASH(count, u32, u64);          // map: pid → 调用次数

int trace_vfs_read(struct pt_regs *ctx) {
    u32 pid = bpf_get_current_pid_tgid() >> 32;   // 高 32 位 = tgid（用户视角的 pid）
    u64 one = 1;
    count.increment(pid);                          // map[pid]++（原子，无锁）
    return 0;
}
"""


def main():
    from bcc import BPF

    b = BPF(text=prog)
    b.attach_kprobe(event="vfs_read", fn_name="trace_vfs_read")
    print(f"统计 vfs_read 调用 {SECS} 秒…（Ctrl-C 提前结束）")
    try:
        sleep(SECS)
    except KeyboardInterrupt:
        pass

    print(f"\n{'PID':>8} {'count':>10}")
    for k, v in sorted(b["count"].items(), key=lambda kv: kv[1].value, reverse=True)[:15]:
        print(f"{k.value:>8} {v.value:>10}")

    # 定时读 map 的循环形态（生产监控骨架）：
    # while True:
    #     sleep(1)
    #     snapshot = dict(b["count"].items())
    #     b["count"].clear()          # 读差口径：清掉下一轮重计
    #     render(snapshot)


if __name__ == "__main__":
    main()
