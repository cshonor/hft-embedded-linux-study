## §18.2 内联汇编 (Inline Assembler)

> **Ch 18 · C 与汇编混合编程** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 典型用途

| 场景 | 例子 |
|------|------|
| **饱和数学** | **`SSAT`/`USAT`** — [Ch7](../chapter-07-integer-logic-arithmetic/) |
| **PSR / Q 标志** | 读 **APSR** 饱和位 **Q** — DSP 溢出跟踪 |
| **协处理器** | 编译器不生成的 **CP 指令** |
| **内存屏障** | 早期 **`__dmb`** 类（现代 C11 **`atomic_thread_fence`**） |

**语法（Keil/ARM 编译器风格，本书）：**

```c
__inline int read_q_flag(void) {
    __asm {
        MRS r0, APSR
        ; … 提取 Q …
    }
}
```

寄存器名 **当作 C 变量** — **编译器分配/保存** 物理寄存器。

---

### 优势

- **零独立函数** — 插入 C 控制流  
- **不用写** 完整序言/尾声（编译器包裹）  
- 适合 **1–5 条** 不可替代指令  

---

### 限制（书中强调 — 必读）

| 限制 | 含义 |
|------|------|
| **编译器优化** | 生成码 **可能与手写 asm 字面不一致** — 勿假设固定调度 |
| **不支持 Thumb** | 内联块内 **无 Thumb 指令**（ARM 编译器语境 — M 纯 Thumb 工程改用 **embedded/.S 或 GCC extended asm**） |
| **禁分支/系统** | **无 `BX`、`SVC`、`BKPT`** 等 |
| **不改 PC/SP** | 不能 **`MOV pc`**、不能直接 **改栈** |
| **无伪指令** | **无 `ADR`、`LDR Rx, =`** — 无文字池 |

**口述：** Inline = **「插入受控 opcode」**，不是 **写完整子程序**。

---

### 与 GCC 内联 asm 对照

| | **本书 Inline (ARMCC)** | **GCC extended asm** |
|---|-------------------------|----------------------|
| 语法 | **`__asm { }`** | **`__asm__ volatile(...)`** |
| 约束 | 寄存器即变量 | **`: "=r"(out)` 操作数** |
| Thumb/M | 书中限制 Thumb | **`-mthumb` 常用** |
| 学习路径 | Smith 本章 | [奔跑吧 Ch10](../arm64-programming-practice/chapter-10-gcc-inline-asm/) |

---

### 何时不用 inline

- 需要 **Thumb-2 全指令**（M4）  
- 需要 **`BL` 调 C** 或 **长循环**  
- 需要 **精确 AAPCS 帧** — 用 **§18.3 embedded** 或 **`.S`**

---

### 可复述要点

1. Inline = **compiler 不支持的少数指令 + PSR/Q**。  
2. **不能当完整函数** — 无 PC/SP/BX/SVC/伪指令。  
3. **优化可改写** — 勿依赖 inline 做 **精确时序**（除非 **`volatile` 语义**）。
