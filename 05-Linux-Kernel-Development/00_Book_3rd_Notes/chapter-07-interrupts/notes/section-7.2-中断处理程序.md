## ② 中断处理程序 · Interrupt Handlers (ISR)

| 项 | 说明 |
|----|------|
| **谁写** | **设备驱动** 的一部分 |
| **本质** | 普通 **C 函数** |
| **运行环境** | **中断上下文（interrupt context）** |
| **要求** | **尽可能快** — 尽快恢复被中断代码 |

```
设备 ──IRQ──► CPU ──► ISR（驱动注册）
                         │
                    应答硬件、最小工作
```

**HFT：** 网卡 **每包一次 IRQ**（或合并中断）— ISR/上半部过长 → **P99 尾延迟**、cache 被冲刷。

→ [03 SysPerf §1.5 IRQ 与策略同核](../../../../03-Systems-Performance-2nd/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md)

---
