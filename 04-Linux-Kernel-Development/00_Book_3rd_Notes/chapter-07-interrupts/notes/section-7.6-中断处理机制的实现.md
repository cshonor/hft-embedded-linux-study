## ⑥ 中断处理机制的实现

LKD 3rd 描述的是 **经典 x86 路径** — 虽随后续内核 **通用 IRQ 层（Generic IRQ）** 演进，**「硬件 IRQ → 向量 → handler 链 → 返回」** 的概念仍完全成立，读懂有助于排障与写驱动。

#### 自硬件到 C 层（概念路径）

```
硬件设备 assert IRQ 线
        │
        ▼
中断控制器（8259 PIC / IOAPIC / ARM GIC…）
        │  优先级仲裁 · 掩码 · 向 CPU 发 vector
        ▼
CPU 收到中断
        │  保存 minimal 寄存器 · 查 IDT
        ▼
架构相关汇编入口（如 x86 中断门 stub）
        │  切换到内核栈 · 构建 pt_regs
        ▼
do_IRQ() / generic_handle_irq()     ← 确认 IRQ 号、屏蔽该线
        │
        ▼
handle_IRQ_event() / handle_irq_event()
        │  遍历该 IRQ 上注册的 action 链
        ▼
各驱动 ISR 依次执行（共享 IRQ 时链式）
        │
        ▼
ret_from_intr / irq_exit
        │  need_resched? ·  pending softirq?
        ▼
返回被中断代码 或  schedule() 或  跑 softirq
```

| 阶段 | 要点 |
|------|------|
| **向量 → 入口** | 架构相关 — IDT/异常向量表 |
| **确认 IRQ 号** | 控制器 **ACK/EOI** — 防重复触发 |
| **屏蔽 IRQ 线** | 处理期间 **防重入** 同一设备 |
| **handler 链** | `request_irq` 注册的 `struct irqaction` 链表 |
| **返回路径** | 检查 **调度**、**软中断 pending**（Ch 4/8） |

#### `struct irqaction`（概念）

| 字段 | 作用 |
|------|------|
| **handler** | 驱动 ISR |
| **flags** | `IRQF_SHARED` 等 |
| **name** | `/proc/interrupts` 显示 |
| **dev_id** | 传给 handler 的 cookie |
| **next** | 共享 IRQ 时 **链下一个** handler |

#### 中断返回时的关键决策

```
irq_exit()
    │
    ├─► pending softirq? ──是──► do_softirq() / __do_softirq()
    │                              （NET_RX 等 — Ch 8）
    │
    ├─► need_resched? ──是──► schedule()
    │
    └─► 否则 restore 寄存器，返回被中断点
```

| 现象 | 可能原因 |
|------|----------|
| **高 `%hi` / hardirq** | ISR 太长或 IRQ 频率过高 |
| **高 `%soft`** | softirq 风暴 — 网络收包（Ch 8.3/8.6） |
| **`/proc/interrupts` 单核倾斜** | IRQ affinity 未调 — HFT 要迁核 |

#### 现代内核的演进（读书时知道即可）

| 书中（LKD 3rd） | 现代替代/补充 |
|-----------------|---------------|
| **`do_IRQ()`** | **`generic_handle_irq()`** + chip 驱动 |
| 单一入口 | **层级**：`irq_desc` → `irq_chip` → flow handler |
| | **Threaded IRQ**：hardirq 极简 + **kthread** 跑慢路径 |

> 驱动开发者 **不必** 改 arch 汇编 — 只需 **`request_irq`**。理解实现是为了：**知道 IRQ 何时被 mask**、**返回路径何时跑 softirq**、**为何 ISR 必须短**。

**HFT：** `perf record -g` 在 IRQ 风暴时常见栈：`do_IRQ` → 驱动 ISR → `net_rx_action`（softirq）。优化顺序通常是 **减 IRQ 次数** → **缩短 ISR** → **softirq 批处理（NAPI）** → **核隔离**。

→ [Ch 8.3](../../chapter-08-bottom-halves/notes/section-8.3-软中断.md) softirq · [Ch 4](../../chapter-04-process-scheduling/) 调度与 `need_resched` · [Ch 7.7](section-7.7-中断控制.md) 关中断 API

---
