## §14.5 向量表

> **Ch 14 · 异常处理：ARM7TDMI** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 向量表在哪

通常位于 **低地址 `0x00000000`**（或 **`0xFFFF0000`** — 由 **复位配置/引脚** 决定）。

每个异常占 **4 字节槽** — 内容是 **指令**，不是裸地址：

```asm
    B       Reset_Handler
    B       Undefined_Handler
    B       SWI_Handler
    B       Prefetch_Handler
    B       DataAbort_Handler
    B       IRQ_Handler
    ; FIQ 槽 @ 0x18 — 见下
```

**与 Cortex-M 对比（Ch15）：** M 系表里直接放 **32 bit handler 地址**；ARM7 放 **`B`/`LDR PC`**。

---

### 标准偏移（ARM7TDMI）

| 偏移 | 异常 |
|------|------|
| **0x00** | Reset |
| **0x04** | Undefined |
| **0x08** | SVC (SWI) |
| **0x0C** | Prefetch Abort |
| **0x10** | Data Abort |
| **0x14** | IRQ |
| **0x18** | FIQ |

[Ch2 向量表](../../chapter-02-programmers-model/notes/section-2-3-arm7tdmi.md) 已列 — 本章讲 **如何写 handler**。

---

### FIQ 在表末的「快速」技巧

**FIQ 是最后一项（0x18）** — 之后 **没有其它向量占用 0x1C**：

```
0x18:  FIQ 向量槽 — 可写短跳转
0x1C:  可直接开始 FIQ handler 本体（无需再 B 到远处）
```

**效果：** 少一次分支、handler 可 **更短** — 配合 **banked r8–r12** 实现 **低延迟**。

---

### 远程向量：`LDR PC, [PC, #imm]`

当 handler 超出 **`B` 范围**：

```asm
    LDR     pc, =IRQ_Handler_Long   ; 伪指令：从文字池取地址到 PC
```

**VIC 扩展（§14.8）：**

```asm
    LDR     pc, [pc, #-0x1C]   ; 从 VIC 向量地址寄存器取 ISR 地址
```

PC 直接跳到 **外设专用** handler — 不用公共 IRQ 里 **轮询** 是谁。

---

### Reset 向量特殊

**Reset** 不返回 — 跳 **初始化代码**（设各模式 **SP**、时钟、内存），再 **`B main`**。

→ [20 U-Boot](../../20-UBoot-Kernel-Build/) 启动链同类思想。

---

### 可复述要点

1. ARM7 向量表 = **指令槽**，不是 M 系的 **函数指针表**。  
2. **FIQ @ 0x18** — 表末后可 **0x1C 紧接代码**。  
3. **VIC + LDR PC** = **硬件分发** 多源 IRQ。
