## ③ 上半部与下半部 · Top Halves vs Bottom Halves

中断处理常 **既要快又要干很多事** — 这在物理上矛盾。Linux 经典解法：**拆分** 为 **上半部（top half）** 与 **下半部（bottom half）**。

| 部分 | 何时跑 | 做什么 | 上下文 |
|------|--------|--------|--------|
| **上半部（Top Half）** | **收到 IRQ 立刻** | **时限内必须完成** — ACK、复位硬件、读少量寄存器、入队 | **硬中断上下文** |
| **下半部（Bottom Half）** | **稍后** — 中断返回路径或 ksoftirqd | **非时间关键** — 协议解析、大量拷贝、唤醒 socket、块 I/O 提交 | softirq / tasklet / **进程**（workqueue） |

```
时间线 ─────────────────────────────────────────────────────────────►

  IRQ 到达
      │
      ▼
  ┌─────────────┐
  │  上半部 ISR  │  ← 极短：ACK · 摘 descriptor · tasklet_schedule()
  └──────┬──────┘
         │ return from interrupt
         ▼
  ┌─────────────┐
  │  下半部      │  ← 可稍长：NET_RX softirq · 协议栈 · wake_up
  └──────┬──────┘
         ▼
  用户态策略 / 其他内核路径继续
```

#### 为什么要拆分

| 约束（上半部） | 若全在 ISR 里做 |
|----------------|-----------------|
| **不能睡眠** | 复杂逻辑里难免触发可睡眠路径 |
| **常屏蔽该 IRQ 线** | 关中断越久 → **系统整体 IRQ 延迟** 上升 |
| **共享中断栈** | 栈深、大数组 → **栈溢出** 风险 |
| **多 CPU 上 IRQ 可并发** | 长 ISR 占用 CPU 更久 → **吞吐下降** |

| 推到下半部的好处 | 说明 |
|------------------|------|
| **缩短关中断窗口** | 硬件及时 ACK，后续包可再触发 IRQ |
| **中断可重新打开** | 下半部跑时 **其他 IRQ 仍可进来**（视具体机制） |
| **选对机制可睡眠** | workqueue 是唯一可阻塞的下半部（Ch 8.5） |

#### 上半部典型工作清单

| 操作 | 原因 |
|------|------|
| **ACK / EOI** 中断 | 告诉硬件/控制器「已收到」 — 防丢失或重复 |
| 读 **状态寄存器** 判断事件类型 | 区分 TX done / RX ready / error |
| **DMA 环摘描述符** | 只记录指针/长度，不解析 payload |
| **`tasklet_schedule` / `raise_softirq`** | 把重活 defer 出去 |

#### 下半部典型工作清单

| 操作 | 常见机制 |
|------|----------|
| **sk_buff 分配、协议解析** | NET_RX **softirq** |
| **唤醒阻塞在 read/poll 的进程** | softirq 末尾或 workqueue |
| **块 I/O 完成、bio 结束** | softirq（BLOCK）或 workqueue |
| **需要 mutex / 大块 GFP_KERNEL 分配** | **workqueue** |

#### 与「中断上下文不能睡眠」的关系

| 层次 | 能否 `schedule()` / 睡眠 |
|------|--------------------------|
| 上半部（hardirq） | **绝对禁止** |
| softirq / tasklet | **禁止** — 仍是原子上下文 |
| workqueue worker | **允许** — 进程上下文（Ch 8.5） |

**HFT：** 收包尖刺不只在 **硬 IRQ** — `mpstat` 里 **`%soft`** 高说明 **NET_RX softirq** 在抢 CPU；策略线程与 **IRQ 核 + softirq 核** 同核时，尾延迟叠加。见 [§1.5 SysPerf](../../../../15-Systems-Performance-2nd/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md)。

→ **Ch 8** 下半部机制详解 · [Ch 7.5](section-7.5-中断上下文.md) 中断上下文 · [SysPerf §3.2 上下半部](../../../../15-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md)

---
