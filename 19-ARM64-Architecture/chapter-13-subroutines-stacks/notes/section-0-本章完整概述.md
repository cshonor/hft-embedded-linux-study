## Ch13 完整概述 · 子程序与堆栈

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Subroutines and Stacks · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **分而治之** | 大任务 → **`BL` 子程序**；返回地址在 **LR (r14)** |
| **堆栈** | **SP (r13)** + **LDM/STM** / **PUSH/POP** 保存/恢复上下文 |
| **可重入** | 入口压栈 **LR + 将改写的寄存器**；出口 **`LDM … pc`** 一次返回 |
| **传参** | 寄存器 · 指针 · 堆栈 — 与 **AAPCS** 对齐才能混 C |

**前置：** [Ch8 BL/分支](../chapter-08-branches-loops/notes/section-8-2-branches.md) · [Ch5 Load/Store](../chapter-05-loads-stores-addressing/) · （可选）[Ch12 查表](../chapter-12-tables/notes/section-0-本章完整概述.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **动机** | §13.1 | [section-13-1-intro.md](./section-13-1-intro.md) |
| **LDM/STM · 堆栈类型 · PUSH/POP** | §13.2 | [section-13-2-stacks.md](./section-13-2-stacks.md) |
| **BL · 可重入 · 返回** | §13.3 | [section-13-3-subroutines.md](./section-13-3-subroutines.md) |
| **三种传参** | §13.4 | [section-13-4-parameters.md](./section-13-4-parameters.md) |
| **AAPCS / APCS** | §13.5 | [section-13-5-apcs.md](./section-13-5-apcs.md) |
| **练习** | §13.6 | [section-13-6-exercises.md](./section-13-6-exercises.md) |

---

### 三、知识流（口述版）

```
Ch8：BL → LR = 返回地址
        ↓
§13.2：SP 指向栈顶 · STMDB/LDMIA ≡ PUSH/POP
        ↓
§13.3：入口 STMDB sp!, {r4-r7, lr}
       出口 LDMIA sp!, {r4-r7, pc}
        ↓
§13.4：r0-r3 传参 / 指针 / 栈上参数
        ↓
§13.5：AAPCS — callee-save r4-r11 · 8 字节栈对齐
        ↓
Ch14 异常也改 SP/LR · Ch17 C↔Asm · 内核/UBoot .S
```

---

### 四、堆栈与指令速查

| 概念 | ARM C 默认 |
|------|------------|
| **类型** | **满递减 (Full Descending, FD)** |
| **压栈** | `PUSH` = **`STMDB sp!`** = **`STMFD`** |
| **出栈** | `POP` = **`LDMIA sp!`** = **`LDMFD`** |
| **M3/M4** | 多寄存器仅 **IA + DB**（够用） |

---

### 五、AAPCS 契约（口述必背）

| 寄存器 | 角色 |
|--------|------|
| **r0–r3** | 参数 + 返回值；**调用者**保存（可破坏） |
| **r4–r8, r10, r11** | **被调用者**若用则必须压栈恢复 |
| **LR** | 返回地址；子程序若再 `BL` 须保存 |
| **s0–s15** | 浮点参数/临时；可破坏 |
| **s16–s31** | **被调用者**保存 |
| **SP** | **8 字节对齐** |

---

### 六、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [02 C](../../02-c-programming/) | 函数调用 = AAPCS |
| [07 TLPI](../../07-The-Linux-Programming-Interface/) | x86/ARM 调用约定对照 |
| [Ch11 泰勒 sin](../chapter-11-floating-point-data-processing/notes/section-11-8-examples.md) | `BL` 浮点子程序 |
| [Ch14 异常](../chapter-14-exception-handling-arm7tdmi/) | 异常帧 = 硬件压栈 |
| [Ch17 混合编程](../chapter-17-mixed-c-assembly/) | `extern "C"` + AAPCS |
| [20 U-Boot](../../20-UBoot-Kernel-Build/) | 启动/板级 `.S` 大量 **SP 设置 + BL** |

---

### 七、下一章（按 OUTLINE）

→ **[Ch14 异常处理 ARM7TDMI](../chapter-14-exception-handling-arm7tdmi/)**（选读 — 中断也会动 SP/LR）  
→ 精读链继续：**Ch16 MMIO** 或 **Ch17 C/Asm**
