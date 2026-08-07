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

**HFT：** 收包尖刺不只在 **硬 IRQ** — `mpstat` 里 **`%soft`** 高说明 **NET_RX softirq** 在抢 CPU；策略线程与 **IRQ 核 + softirq 核** 同核时，尾延迟叠加。见 [§1.5 SysPerf](../../../../19-systems-performance/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md)。

→ **Ch 8** 下半部机制详解 · [Ch 7.5](section-7.5-中断上下文.md) 中断上下文 · [SysPerf §3.2 上下半部](../../../../19-systems-performance/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md)

### 常见陷阱

1. 混淆上半部和下半部——上半部在 hard IRQ 上下文（不可睡眠），下半部在 softirq/进程上下文
2. 以为 tasklet 还被推荐——tasklet 已 deprecated，推荐 workqueue 或 threaded IRQ
3. 在 softirq 中调用 mutex_lock()——softirq 不能睡眠，只能用 spinlock

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 上半部和下半部的分工原则？

<details><summary>答案</summary>

上半部（hard IRQ）：① 确认硬件 ACK。② 读取时间敏感数据。③ 调度下半部。必须 <10us。下半部（softirq/tasklet/workqueue）：① 大部分中断处理工作。② 可做 I/O、分配内存（workqueue 可睡眠）。③ 可被中断抢占但不能被同类型 softirq 抢占（per-CPU）。

</details>

**Q2.** softirq、tasklet、workqueue 三种下半部机制如何选择？

<details><summary>答案</summary>

softirq：编译时静态注册，性能最高，不能睡眠。tasklet：基于 softirq，动态注册，同类型不并发，已 deprecated。workqueue：在 kworker 线程运行，可睡眠/持 mutex，最灵活。选择：需要睡眠 → workqueue。需要高性能 → softirq（或 NAPI）。不再推荐 → tasklet（用 workqueue 或 threaded IRQ 替代）。

</details>

**Q3.** HFT 中下半部对延迟有什么影响？

<details><summary>答案</summary>

NIC 收包走 NET_RX_SOFTIRQ，在 hard IRQ 返回或 ksoftirqd 中执行。如果 softirq 积压，收包延迟增大。排查：`/proc/softirqs` 看 NET_RX 计数。解决：① 绑 softirq 到非交易核（RPS/RFS）。② NAPI 轮询模式。③ DPDK 完全绕过 softirq。④ `nohz_full` 减少软中断频率。

</details>

</details>


> ↔ [ULK Ch4 §7 可延迟函数与工作队列](../../../../08-linux-kernel-deep/chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md)
---
