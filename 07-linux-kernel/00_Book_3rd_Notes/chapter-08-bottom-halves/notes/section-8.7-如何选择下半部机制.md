## ⑦ 如何选择下半部机制

三种下半部 **没有绝对最优** — 按 **是否睡眠**、**性能要求**、**并发模型** 选型。LKD 3rd 给出清晰决策路径。

#### 快速对照表

| 需求 | 选择 |
|------|------|
| 工作需要 **睡眠 / 阻塞**（mutex、大块 GFP_KERNEL、块 I/O） | **只能 workqueue** |
| 不睡眠、**普通设备驱动** 下半部 | **tasklet**（易用、锁简单） |
| **核心子系统**、要多 CPU **同类型并行** | **softirq** |
| 需要 **严格顺序**、怕堵塞全局池 | **专用 workqueue** |
| **极热路径**、已 profile 瓶颈 | softirq + per-CPU 设计 |

#### 决策树

```
需要 defer 的工作
    │
    ├─ 会 sleep / 阻塞 / 长 I/O？
    │       │
    │      是 ──► workqueue（或 threaded IRQ + process ctx）
    │       │
    │      否
    │       │
    │       ├─ 驱动级、单设备 defer？
    │       │       │
    │       │      是 ──► tasklet（默认首选）
    │       │       │
    │       │      否 ──► 子系统级、要多 CPU 同类型并行？
    │       │               │
    │       │              是 ──► softirq
    │       │               │
    │       │              否 ──► 再评估 tasklet 是否够用
    │       │
    └─ 延迟要求微秒级且在内核栈内？
            └── 尽量 ISR + per-CPU ring；或 NAPI；或旁路
```

#### 三维对比

| 机制 | 上下文 | 睡眠 | 同实例多 CPU 并行 | 典型延迟 |
|------|--------|------|-------------------|----------|
| **softirq** | 软中断 | 否 | **是**（同类型） | **最低** |
| **tasklet** | 软中断 | 否 | **否** | 低 |
| **workqueue** | **进程** | **是** | 是（多 worker） | 较高（调度） |

#### 按子系统举例

| 子系统 | 选择 | 原因 |
|--------|------|------|
| **TCP/IP 收包** | NET_RX **softirq** + NAPI | 多 CPU 并行、已优化 |
| **自定义字符驱动** | **tasklet** + kfifo | 简单、够快 |
| **WiFi / 复杂 USB** | tasklet + **workqueue** 组合 | 快慢路径分离 |
| **块层完成** | BLOCK softirq 或 workqueue | 视路径 |
| **驱动 remove** | `tasklet_kill` / `cancel_work_sync` | 必须匹配所选机制 |

#### 常见错误

| 错误 | 后果 |
|------|------|
| 在 tasklet 里 **`mutex_lock`** | BUG / 死锁 |
| 热路径滥用 **workqueue** | 调度延迟、tail latency |
| 该用锁却靠 **「tasklet 串行」** | 不同 tasklet / ISR 仍并发 |
| unload 不 **kill/cancel** | use-after-free |

#### Embedded vs HFT 侧重点

| 场景 | 倾向 |
|------|------|
| **嵌入式驱动** | tasklet + workqueue — **简单正确** |
| **HFT 收包** | 内核栈：调 **NAPI/affinity**；或 **用户态轮询** |
| **FPGA 卡** | ISR → **lock-free ring** → 用户态 mmap 消费 |

**HFT：** 选型前先 **`perf record`** 看时间花在 **hardirq / softirq / process**。若 softirq 主导，换 tasklet **不会** 更快 — 要 **减包数、迁核、旁路**。

→ [Ch 8.3](section-8.3-软中断.md) · [Ch 8.4](section-8.4-tasklet.md) · [Ch 8.5](section-8.5-工作队列.md) · [Ch 7](../../chapter-07-interrupts/) 上半部

---
