## ① 下半部概念与必要性

**上半部（ISR）** 的硬约束（Ch 7）：

| 限制 | 后果 |
|------|------|
| **异步** | 随时打断别的代码 |
| **不能阻塞/睡眠** | 不能用 mutex、不能等 I/O |
| **常屏蔽中断线** | 关中断越久，系统越迟钝 |

**策略：** 上半部只做 **应答硬件 + 最小拷贝**；其余 **允许延迟** 的工作 → **下半部**，在 **中断全开、相对安全** 的时机跑。

```
IRQ 上半部（极短）
    │ ACK · 摘环 · 入队
    ▼
下半部（可稍长）
    │ 协议处理 · 唤醒 socket · 块 I/O 提交…
    ▼
用户态 / 其他内核路径继续
```

**HFT：** 收包尖刺不只在 **硬 IRQ**，常在 **softirq 网络 RX** — `mpstat` 看 **`%soft`**。

→ [03 SysPerf §3.2 下半部](../../../../03-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [§1.5 IRQ/softirq 同核](../../../../03-Systems-Performance-2nd/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md)

---
