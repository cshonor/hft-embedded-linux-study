## ① 初始化 PIT · 使用定时器

#### 为何需要硬件定时器？

键鼠 **事件驱动** — 没输入就没事；但 OS 还需要 **流逝时间**：超时、动画、**时间片**。

| 硬件 | 说明 |
|------|------|
| **PIT** | Programmable Interval Timer（如 **8254**） |
| **连线** | **IRQ0** → 经 PIC → CPU |
| **编程** | 设定 **分频** → 每隔固定间隔 **脉冲一次中断** |

原书示例：**约每秒 100 次** → tick 间隔 **10ms**。

```
PIT 计数减到 0 ──► IRQ0 ──► PIC ──► IDT[0x20] ──► inthandler20
```

**ISR 仍要短**（Day 7 原则）— 这里 mostly **count++**、检查 **next**。

→ [Day 6 PIC/IDT](../day-06-split-compile-irq/)

---
