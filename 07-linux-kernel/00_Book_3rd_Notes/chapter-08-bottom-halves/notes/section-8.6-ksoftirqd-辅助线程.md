## ⑥ ksoftirqd 辅助线程

**ksoftirqd** 是为解决 **softirq 风暴** 而设计的 **每 CPU 辅助内核线程** — 在 **软中断负载过重** 时 **延后** 部分 softirq，防止 **用户态饿死**。

#### 问题：为何需要 ksoftirqd

| 问题 | 说明 |
|------|------|
| **中断返回路径跑 softirq** | `irq_exit()` → `__do_softirq()` — **仍在「中断尾巴」语义** |
| **softirq 不断 re-pending** | 网络 flood 时 **NET_RX** 永远处理不完 |
| **若一直跑 softirq** | **用户态线程**（含策略进程）**得不到 CPU** — 系统「假死」 |

```
无 ksoftirqd 极限情况：
  IRQ → softirq → 更多包 → 再 IRQ → 再 softirq → …
                                    │
                                    └── 用户态 0% CPU
```

#### ksoftirqd 是什么

| 属性 | 说明 |
|------|------|
| **线程名** | **`ksoftirqd/N`** — 每 CPU 一个，`N` = CPU 号 |
| **优先级** | **nice 19** — 低于普通进程，高于完全饿死 |
| **触发** | `__do_softirq()` 发现 **超过 budget** 仍有 pending → `wakeup_ksoftirqd()` |
| **行为** | 在 **进程上下文** 里继续 `run_ksoftirqd()` — 仍跑 softirq handler，但 **可被调度打断** |

#### 两阶段处理（概念）

```
raise_softirq(NET_RX)
        │
        ▼
irq_exit: __do_softirq()
        │  处理最多 budget 个（如 net.core.netdev_budget）
        │
        ├─► pending 清空 ──► 返回用户态
        │
        └─► 仍 pending ──► wakeup ksoftirqd/N
                              │
                              ▼
                         ksoftirqd 继续 do_softirq
                              │
                              └── 与用户态 **交替** 获得 CPU
```

| 参数（sysctl 示例） | 作用 |
|---------------------|------|
| **`net.core.netdev_budget`** | 一次 softirq **最多处理包数** |
| **`net.core.netdev_max_backlog`** | 每 CPU 输入队列深度 |

#### 观测方法

| 工具 | 看什么 |
|------|--------|
| **`top` / `htop`** | `ksoftirqd/0` 等 **CPU% 高** |
| **`mpstat -P ALL 1`** | **`%soft`** 高 |
| **`/proc/softirqs`** | `NET_RX` 各 CPU 计数 **极不均衡** |

#### 与硬 IRQ、NAPI 的关系

| 层次 | 角色 |
|------|------|
| **hardirq** | 极短 — schedule NAPI |
| **softirq（NET_RX）** | 批量 poll 网卡 |
| **ksoftirqd** | softirq **消化不完** 时的 **后备** |

**HFT：** 行情洪峰时 **`%soft` + ksoftirqd`** 与策略线程 **同核** → **P99 尾延迟** 叠加。常见手段：

| 手段 | 目的 |
|------|------|
| **IRQ affinity 迁核** | IRQ 与策略分离 |
| **RPS/RSS** |  spread softirq 到多核 |
| **调 budget / coalescing** | 批处理换延迟 |
| **CPU isolation（isolcpus）** | 策略核不收 softirq |
| **DPDK / 内核旁路** | 跳过 net_rx 路径 |

→ [Ch 8.3](section-8.3-软中断.md) softirq · [§1.5 SysPerf IRQ/softirq 同核](../../../../19-systems-performance/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md) · [Ch 4](../../chapter-04-process-scheduling/) 调度与 nice

---
