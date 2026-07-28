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

→ [03 SysPerf §3.2 下半部](../../../../15-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [§1.5 IRQ/softirq 同核](../../../../15-Systems-Performance-2nd/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md) · [Ch 7](../../chapter-07-interrupts/) 上半部

---
