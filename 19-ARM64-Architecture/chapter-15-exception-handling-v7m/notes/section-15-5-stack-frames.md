## §15.5 处理器出入栈序列

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### v7-M 最大亮点：硬件栈帧

异常 **入口** 硬件自动 **压 8 个字** 到当前栈（MSP 或 PSP，取决于 Thread 配置）：

| 栈上顺序（先压的在高地址） | 寄存器 |
|----------------------------|--------|
| 1 | **xPSR** |
| 2 | **PC**（返回地址） |
| 3 | **LR**（异常前的 LR，非 EXC_RETURN） |
| 4 | **r12** |
| 5 | **r3** |
| 6 | **r2** |
| 7 | **r1** |
| 8 | **r0** |

**对比 Ch14 ARM7：** 软件须 **`STMDB r0-r12`**；M 系 **硬件完成 r0–r3,r12,LR,PC,xPSR** — **延迟极低**。

---

### FPU 扩展帧（Cortex-M4F）

若异常时 **浮点上下文活跃**，硬件可能额外压 **S0–S15 + FPSCR** 等 — **Lazy stacking** 可推迟直到 ISR 真用 FPU（[Ch9–11](../chapter-09-floating-point-basics/) 可选）。

---

### EXC_RETURN — LR 中的特殊值

异常入口后 **LR 不是普通返回地址**，而是 **`EXC_RETURN` magic**，例如：

| 典型值 | 含义（口述） |
|--------|--------------|
| **0xFFFFFFF1** | 返回 **Handler**，用 **MSP** |
| **0xFFFFFFF9** | 返回 **Thread**，用 **MSP** |
| **0xFFFFFFFD** | 返回 **Thread**，用 **PSP** |

（带 FPU 时还有其他编码位 — 查 ARM 手册 / CMSIS。）

**位含义概要：** 指定 **回 Thread 还是 Handler**、**用 MSP 还是 PSP**、**是否扩展 FPU 栈帧**。

---

### 异常退出：无需 SUBS pc,lr,#n

```asm
    BX      lr          ; lr 仍是 EXC_RETURN
; 或 C ISR 结尾的 POP {..., pc} 若 lr 被正确保持
```

当 **PC ← EXC_RETURN 值** 时，CPU 识别 **非合法代码地址** → 触发 **硬件弹栈**：

- 恢复 **r0–r3, r12, LR, PC, xPSR**  
- 恢复 **Thread/Handler** 与 **MSP/PSP** 状态  

**口述：** **`BX lr`** 在 Handler 里 = **「结束异常」**，不是跳到 LR 字面地址。

---

### 软件还需保存什么

硬件 **不保存 r4–r11** — AAPCS **callee-save**：

| 写法 | 谁保存 r4–r11 |
|------|---------------|
| **C ISR** | 编译器 prologue/epilogue |
| **Asm ISR** | 手动 **PUSH/POP** |

**Ch13 序言/尾声** 仍在，但 **范围缩小**。

---

### 入口/出口总图

```
Entry:  HW push 8-word frame · LR ← EXC_RETURN
        ↓
        ISR body（C 或 asm）
        ↓
Exit:   BX lr (EXC_RETURN) → HW pop frame → 恢复 Thread
```

---

### 可复述要点

1. **8 寄存器硬件压栈** = v7-M 低延迟核心。  
2. **LR = EXC_RETURN**；**`BX lr`** = 异常返回。  
3. **r4–r11** 仍靠 **编译器或手写** 保存。
