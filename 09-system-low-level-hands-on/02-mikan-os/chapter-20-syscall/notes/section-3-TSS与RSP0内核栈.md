## 3. TSS 与 RSP0 内核栈

---

### 一、问题：Ring 3 中断用谁的栈？

**应用在 Ring 3 运行时发生硬件中断**（APIC 定时器 · 键盘中断）：

```
CPU 需切到 Ring 0 执行 ISR
    → 不能使用 用户 RSP（会被应用破坏 / 空间不足）
    → 必须换 **内核栈**
```

---

### 二、TSS.RSP0

**x86-64 TSS（64-bit Task State Segment）** 关键字段：

| 字段 | 作用 |
|------|------|
| **`RSP0`** | **CPL 0 栈指针** — Ring3→Ring0 **硬件自动加载** |
| （其他） | IST 等 — 本书用 **RSP0** 为主 |

**流程：**

```
定时器中断 @ Ring3
  → CPU: RSP ← TSS.RSP0
  → 压栈 SS/RSP/RFLAGS/CS/RIP（用户态）
  → 跳转 IDT 中 ISR（Ring 0）
```

---

### 三、实现要点

| 步骤 | 说明 |
|------|------|
| **分配内核栈** | 独立 **4KiB×N** — 勿与用户栈重叠 |
| **InitializeTSS()** | `tss.rsp0 = kernel_stack_top` |
| **LoadTR()** | GDT 中 **TSS 描述符** · **`ltr`** |

**未正确设置 TSS：** ISR 在 **用户栈** 上跑 → **覆盖应用/内核数据** · **随机 #PF/GP**。

→ [Ch8 GDT/TSS 预告](../chapter-08-memory/notes/section-4-GDT与分段.md) · [Ch7 IDT](../chapter-07-interrupt-fifo/)

---

### 四、与多任务

**Ch13 Task 切换** + **Ch20 每应用用户栈** — 中断仍统一 **TSS.RSP0** 或 **IST**（高阶 OS 每 CPU 内核栈）。

**本章：** 单核 QEMU — **一组 RSP0** 通常够用。

---

← [2. Ring3](./section-2-Ring3与页表User位.md) · 下一节 [4. 异常调试](./section-4-异常处理与调试.md)
