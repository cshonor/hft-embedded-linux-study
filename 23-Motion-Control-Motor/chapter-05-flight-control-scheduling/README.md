# Ch5 · 飞控调度 · PREEMPT_RT · 绑核

← [23 总览](../README.md)

> **HFT 独家优势区：** 用已学的 **低延迟 / 绑核 / 测量** 补 Linux 实时性

---

## 要点

| 手段 | 说明 |
|------|------|
| **控制环周期** | 典型 250Hz–1kHz — 明确 `Ts` |
| **isolcpus / taskset** | 飞控线程 **独占核** — [05 LKD](../../03-Linux-Kernel-Development/) |
| **SCHED_FIFO** | RT 调度策略 — 用户态飞控线程 |
| **PREEMPT_RT 补丁** | 内核可抢占 — 降低内核延迟 |
| **cyclictest / perf** | 验证 **周期 jitter p99** — [03 SysPerf](../../13-Systems-Performance-2nd/) |

## 与 HFT 对照

| HFT T2T | 飞控 |
|---------|------|
| 行情 → 发单 μs 级 | IMU → 电机 **固定 Ts** |
| p99 尾延迟 | **控制环 overrun** 计数 |
| 无锁环 IPC | 传感器批次 **零拷贝**（可选） |

---

## 验收

- [ ] `cyclictest` 在目标板上给出可接受 **max latency**  
- [ ] 飞控环 **overrun** 可日志 + 统计（异步，不阻塞控制）

→ [22 无人机实战](../../22-Embedded-Linux-Practice/) · [15 HFT 测量](../../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md)
