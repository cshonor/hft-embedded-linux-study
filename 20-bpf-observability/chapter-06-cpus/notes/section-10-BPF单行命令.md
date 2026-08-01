# 10. BPF 单行命令 (One-Liners)

本章末尾示例 — 与 [Ch 5](../../chapter-05-bpftrace/)、[附录 A](../../appendix-A-bpftrace单行命令.md) 衔接：

```bash
# 全系统 CPU 栈采样（bpftrace）
bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'

# 上下文切换时的内核栈
bpftrace -e 'tracepoint:sched:sched_switch { @[kstack] = count(); }'

# 某 PID 的 on-CPU 用户栈
bpftrace -e 'profile:hz:99 /pid == 1234/ { @[ustack] = count(); }'

# runqlat 等价思路（教学用；生产直接用 runqlat-bpfcc）
# 见 man runqlat-bpfcc
```

**原则：** 固定场景用 **BCC 工具**（已优化、有 man）；**验证假设** 用 bpftrace 单行。

---
