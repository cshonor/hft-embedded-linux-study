## ① 中断的概念 · Interrupts

硬件用 **电子信号** **异步** 打断 CPU 正在执行的指令流，迫使内核 **立即** 响应外设事件。这是 **设备驱动** 与硬件世界交互的 **主入口** — 网卡收包、磁盘完成 DMA、定时器 tick 都依赖它。

| 概念 | 说明 |
|------|------|
| **IRQ（Interrupt Request，中断请求线）** | 每条物理/逻辑中断线对应 **唯一数字**（IRQ 号）— 内核据此区分来源设备 |
| **异步中断（asynchronous interrupt）** | 与当前正在执行的 **指令无关** — 随时可能插入 |
| **中断控制器（interrupt controller）** | PIC / APIC / IOAPIC / GIC 等 — 汇聚多路 IRQ，向 CPU 发 **向量（vector）** |
| **中断描述符表（IDT）** | x86 上存放 **中断门** 入口地址 — CPU 收到向量后跳转到对应处理例程 |

#### 中断 vs 异常（同步陷阱）

| 类型 | 来源 | 时机 | 典型例子 |
|------|------|------|----------|
| **硬件中断（IRQ）** | **外设** | **异步** — 与当前指令无关 | 网卡、磁盘、键盘、定时器 |
| **异常（Exceptions）** | **CPU 执行指令** | **同步** — 由某条指令 **直接触发** | 缺页（page fault）、除零、非法指令 |
| **系统调用陷入** | 用户态 **主动** `int 0x80` / `syscall` | **同步** | 进入内核态执行 `sys_*` |

```
异步 IRQ 插入指令流：
  用户/内核代码:  insn₁ ──► insn₂ ──► [IRQ!] ──► ISR ──► insn₃ ──► …
                                    ↑
                              与 insn₂ 无因果关系

同步异常：
  insn ──► 触发 page fault ──► 缺页处理 ──► 可能返回原 insn 重试
```

#### 中断的代价

| 代价 | 原因 |
|------|------|
| **CPU 流水线冲刷** | 跳转 ISR 破坏分支预测、I-cache 局部性 |
| **缓存污染** | ISR 访问的代码/数据挤掉热路径 working set |
| **关中断窗口** | 处理期间可能屏蔽该 IRQ 线 — 丢后续事件或延迟 |

**HFT：** 高频行情场景下，**每包一次 IRQ** 的网卡会把 CPU 从策略线程 **反复打断** — 合并中断（interrupt coalescing）、NAPI、RSS/RPS 迁核都是为了 **降低 IRQ 频率** 与 **隔离 IRQ 核**。

#### 与驱动开发的关系

| 阶段 | 驱动做什么 |
|------|------------|
| **probe** | 从设备树/PCI 获取 **IRQ 号** |
| **open/启动** | `request_irq()` 注册 ISR |
| **运行时** | ISR 应答硬件、最小工作、调度下半部 |
| **remove/关闭** | `free_irq()` 注销 |

→ [Ch 1](../../chapter-01-intro/) syscall vs 中断 · [Ch 5](../../chapter-05-system-calls/) 进程上下文 · [Ch 8](../../chapter-08-bottom-halves/) 下半部

→ 教学对照：[01 Day 5 GDT/IDT](../../../../05-os-from-scratch/thirty-days-os/day-05-gdt-idt/) · [Day 7 PIC](../../../../05-os-from-scratch/thirty-days-os/day-07-fifo-mouse/)

---
