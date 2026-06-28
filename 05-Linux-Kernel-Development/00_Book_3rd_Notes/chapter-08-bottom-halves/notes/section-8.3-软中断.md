## ③ 软中断 · Softirqs

| 属性 | 说明 |
|------|------|
| 分配 | **编译期静态** 定义 |
| 典型用户 | **网络、块设备** — 最耗时、最性能关键的路径 |
| 并发 | **同一 softirq 类型可在多 CPU 上同时跑** |

#### 开发者义务

| 手段 | 原因 |
|------|------|
| **严密锁** | 共享数据竞态 |
| **per-CPU 变量** | 减少跨核锁争用 |

```
CPU0: NET_RX softirq ──┐
CPU1: NET_RX softirq ──┼──► 可能同时处理不同包 — 共享队列要锁
CPU2: NET_RX softirq ──┘
```

→ [12 Rosen Ch14 NAPI/softirq](../../13-Linux-Kernel-Networking/chapter-14-advanced-topics/)

---
