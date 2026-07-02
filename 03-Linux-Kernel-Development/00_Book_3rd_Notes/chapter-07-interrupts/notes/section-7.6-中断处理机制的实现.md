## ⑥ 中断处理机制的实现

硬件 IRQ 到 C 层（书中经典路径，概念仍成立）：

```
硬件 IRQ
    ▼
中断控制器（PIC/APIC/IOAPIC…）
    ▼
CPU 向量 ──► 架构相关汇编入口
    ▼
do_IRQ()          ── 确认、屏蔽该线
    ▼
handle_IRQ_event() ── 遍历该线上注册的 handler
    ▼
各 ISR 执行
    ▼
ret_from_intr()   ── 返回前：调度？下半部？
```

| 阶段 | 要点 |
|------|------|
| **屏蔽 IRQ 线** | 防重入处理同一设备 |
| **返回路径** | 可能 **need_resched**、触发 **下半部**（Ch 4/8） |

> 现代内核 **通用 IRQ 层**（`handle_irq_event` 等）替代部分 `do_IRQ` 细节 — 读书抓 **「向量 → handler 链 → 返回」** 即可。

---
