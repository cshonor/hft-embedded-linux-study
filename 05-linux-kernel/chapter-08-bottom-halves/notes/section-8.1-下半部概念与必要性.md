## ① 下半部概念与必要性

**上半部（ISR，Ch 7）** 在 IRQ 到达后 **立刻** 运行，受到 **硬约束**；任何 **可延迟** 的工作都应推到 **下半部（bottom half）** — 在 **相对安全、中断已恢复** 的时机执行。

#### 上半部的硬约束（回顾 Ch 7）

| 限制 | 后果 |
|------|------|
| **异步插入** | 随时打断用户态策略、内核协议栈 |
| **不能阻塞/睡眠** | 不能用 `mutex`、不能等 I/O、不能 `GFP_KERNEL` 大块分配 |
| **常屏蔽中断线** | 关 IRQ 越久 → **系统 IRQ 延迟**、丢事件风险 |
| **栈空间有限** | 中断栈通常 **几 KB** — 不宜深调用链 |

#### 下半部解决什么问题

| 问题 | 下半部策略 |
|------|------------|
| ISR 太长 | **ACK + 入队** 即可返回 |
| 需要复杂逻辑 | 在 softirq/tasklet/workqueue 里做 |
| 需要阻塞 | **只能 workqueue**（进程上下文） |
| 与进程共享数据 | 缩短 **持锁 + 关中断** 窗口 |

```
IRQ 到达
    │
    ▼
┌─────────────────┐
│ 上半部（极短）    │  ACK · 读状态 · DMA 摘环 · schedule 下半部
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 下半部（可稍长）  │  协议解析 · sk_buff · wake socket · 提交 bio
└────────┬────────┘
         │
         ▼
用户态 read/poll 返回 · 策略线程继续
```

#### 三种现代下半部（Ch 8 详述）

| 机制 | 上下文 | 睡眠 | 典型 |
|------|--------|------|------|
| **softirq** | 软中断 | 否 | NET_RX、BLOCK |
| **tasklet** | 软中断（封装） | 否 | 普通驱动 defer |
| **workqueue** | **进程** | **是** | 慢路径、需 mutex |

#### 驱动常见模式

| 模式 | 上半部 | 下半部 |
|------|--------|--------|
| **网卡（NAPI）** | IRQ → `__napi_schedule` | NET_RX softirq 批量 poll |
| **字符设备** | IRQ → 读 FIFO 入 **kfifo** | tasklet 唤醒 waitqueue |
| **块设备完成** | IRQ → 标记 bio done | softirq 或 workqueue 完成路径 |
| **需 I2C/SPI 慢总线** | IRQ 仅标记 | **workqueue** 里发消息 |

#### 延迟 vs 吞吐权衡

| 倾向 | 做法 |
|------|------|
| **低延迟** | 上半部略多 + tasklet 立刻 schedule |
| **高吞吐** | IRQ 合并 + NAPI 批量 + softirq 多包一次 |
| **HFT 常见** | 旁路内核栈（DPDK）或 **隔离 IRQ 核** |

**HFT：** 收包尖刺不只在 **硬 IRQ** — `mpstat` 看 **`%soft`** 是否飙高；`top` 里 **`ksoftirqd`** 忙说明 softirq 溢出到辅助线程。策略线程与 **IRQ + softirq 同核** 是经典 tail latency 来源。

→ [06.6 SysPerf §3.2 下半部](../../../06.6-systems-performance/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [§1.5 IRQ/softirq 同核](../../../06.6-systems-performance/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md) · [Ch 7](../../chapter-07-interrupts/) 上半部

### 常见陷阱

1. 以为中断处理函数可以做所有工作——耗时操作必须延迟到下半部，否则中断延迟过大
2. 混淆上半部和下半部的执行上下文——上半部在 hard IRQ 上下文，下半部在 softirq/进程上下文
3. 在下半部中用错误的同步原语——softirq/tasklet 只能用 spinlock，workqueue 可以用 mutex

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 为什么需要下半部机制？不能全在中断处理函数中做吗？

<details><summary>答案</summary>

中断处理函数运行在中断上下文，期间该 CPU 的中断可能被禁用（或同优先级中断被阻塞）。如果处理时间长：① 其他中断延迟增大。② 系统响应变慢。③ 可能丢中断。解决方案：上半部只做最紧急的（确认硬件 + 读取数据），大部分工作延迟到下半部（softirq/tasklet/workqueue）执行。

</details>

**Q2.** 上半部和下半部的执行上下文有什么区别？

<details><summary>答案</summary>

上半部：hard IRQ 上下文，无 task_struct，不可睡眠，不可调度，中断可能被禁用。下半部：softirq/tasklet 仍在 softirq 上下文（不可睡眠但可被中断抢占），workqueue 在 kworker 线程（进程上下文，可睡眠/可调度）。HFT 热路径应避免在 softirq 中做大量工作。

</details>

**Q3.** HFT 中下半部机制对延迟的影响？

<details><summary>答案</summary>

softirq 在 hard IRQ 返回时执行，会延迟用户线程恢复。NET_RX_SOFTIRQ（收包）是最常见的延迟源。解决：① RPS/RFS 把 softirq 迁移到非交易核。② NAPI 轮询减少 softirq 频率。③ DPDK 完全绕过 softirq。④ `nohz_full` 减少定时器 softirq。

</details>

</details>

---
