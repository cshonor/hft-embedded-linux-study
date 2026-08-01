## §14.4 异常序列

> **Ch 14 · 异常处理：ARM7TDMI** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 硬件自动四步（必背）

异常发生时 **CPU 硬件** 连续完成 — **无需** 程序员写入口序言：

```
1. CPSR → SPSR_<mode>     （保存旧状态：标志、Thumb位、中断屏蔽、模式）
2. 改 CPSR 模式位         （进入 irq/fiq/abt/und/svc/sys…）
                           通常：禁 IRQ；Reset/FIQ 还禁 FIQ
                           Thumb 时强制回 ARM 状态（ARM7 路径）
3. LR_<mode> ← 返回地址   （与异常类型相关的 PC 修正值）
4. PC ← 向量表入口        （强制跳转）
```

**与 Ch13 对比：** **BL** 只改 **LR + PC**；异常还改 **CPSR/SPSR + 模式**。

---

### 模式切换示意

```
User 模式跑应用
        ↓ IRQ 到来
IRQ 模式：SP_irq / LR_irq 生效；SPSR_irq 存 User 的 CPSR
        ↓ handler 执行
返回：恢复 CPSR ← SPSR；PC ← 修正后的 LR
        ↓
回到 User 继续
```

[Ch2 §2.3](../../chapter-02-programmers-model/notes/section-2-3-arm7tdmi.md)：**banked SP/LR** 使 **IRQ handler 不必立刻换全局 SP**。

---

### LR 中的返回地址（流水线）

ARM7 **五级流水线** — 异常时 **LR 不是「当前指令」**：

| 异常类型 | LR 含义（典型） | 返回指令 |
|----------|-----------------|----------|
| **IRQ** | PC+4 | **`SUBS pc, lr, #4`** |
| **Data Abort** | PC+8 | **`SUBS pc, lr, #8`** |
| **Prefetch Abort** | PC+4 | **`SUBS pc, lr, #4`** |
| **SVC/Undefined** | PC+4 等 | 依手册 |

**`SUBS pc, lr, #n`：** **`S` 位** 写 PC 时 **SPSR → CPSR** — **原子** 恢复模式+标志+返回址。

---

### 软件 handler 额外职责

硬件 **不保存 r0–r12**（FIQ 部分除外）：

```asm
IRQ_Handler
    STMDB   sp!, {r0-r12, lr}    ; 软件保存
    ; … 服务外设 …
    LDMIA   sp!, {r0-r12, lr}
    SUBS    pc, lr, #4           ; 返回 + 恢复 CPSR
```

**Ch13 栈** + **Ch14 硬件帧** = 完整 ISR。

---

### 可复述要点

1. **SPSR/CPSR/LR/PC** 四件套 — 硬件做前三步，软件管 **通用寄存器**。  
2. 返回用 **`SUBS pc, lr, #imm`** — **imm 因异常类型而异**。  
3. 异常入口 **自动关 IRQ** — handler 内再按需 **开中断**（嵌套）。
