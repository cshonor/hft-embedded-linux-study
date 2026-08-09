## 2. IST 与定时器中断栈修复

---

### 一、Bug 现象

**Ch20 场景：** 应用执行 **`syscall` 的极短窗口** 内 **定时器中断** 到达。

| 问题 | 后果 |
|------|------|
| CPU 从 Ring3 进 ISR | 需 **内核栈** |
| **RSP0 指向应用/user 栈** 或 **无效区** | **#PF** · 系统崩溃 |

**根因：** 仅 **TSS.RSP0** 在 **syscall 路径** 上可能仍关联 **用户上下文栈** — 与 **Ch20 TSS.RSP0** 修复 **互补但不足**。

→ [Ch20 TSS.RSP0](../chapter-20-syscall/notes/section-3-TSS与RSP0内核栈.md)

---

### 二、中断堆栈表 IST

**x86-64 TSS 扩展：** **IST1–IST7** — **特定中断向量** 可 **强制指定栈**。

```
TSS.IST[1] = timer_kernel_stack_top

IDT[timer_vector].ist = 1   // 使用 IST1，忽略 RSP0
```

| 对比 | RSP0 | IST1 |
|------|------|------|
| **默认** | 多数 **Ring3→Ring0 中断** | — |
| **本书定时器** | — | **APIC 定时器专用安全栈** |

**效果：** 定时器 ISR **永远在预分配 OS 栈** — **syscall 期间亦如此**。

---

### 三、配置步骤

| 步骤 | 操作 |
|------|------|
| 1 | **AllocatePages** — IST1 栈 |
| 2 | **`tss.ist[1] = stack_top`** |
| 3 | **SetIDTEntry(timer, handler, ist=1)** |
| 4 | **LoadTR** 刷新 TSS |

→ [Ch7 IDT](../chapter-07-interrupt-fifo/) · [Ch11 APIC 定时器](../chapter-11-timer-acpi/)

---

### 四、与多任务

**Ch13 抢占** 依赖 **定时器中断** — IST 保证 **抢占路径栈可靠** — **GUI 应用长运行** 之前提。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. printf](./section-3-PutString与printf适配.md)
