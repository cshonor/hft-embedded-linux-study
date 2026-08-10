# §21.2 调用约定与栈帧 ⭐

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

AAPCS64（ARM Architecture Procedure Call Standard for AArch64）的调用约定：寄存器分配规则、栈帧结构、FP/LR 的作用、栈回溯原理。

## 核心要点

### 寄存器分配

| 寄存器 | 角色 | 调用约定 |
|--------|------|----------|
| X0-X7 | 参数 / 返回值 | caller-saved（调用者保存） |
| X8 | 间接结果寄存器 / syscall 号 | caller-saved |
| X9-X15 | 临时寄存器 | caller-saved |
| X16-X17 | IP0/IP1，链接器 PLT 用 | caller-saved |
| X18 | 平台寄存器（Linux 用于 shadow call stack） | 保留 |
| X19-X28 | callee-saved | 被调用者负责保存 |
| X29 (FP) | 帧指针 | callee-saved |
| X30 (LR) | 返回地址 | callee-saved |
| SP | 栈指针 | 16 字节对齐 |

### 栈帧结构

```
高地址
┌──────────────────┐
│ 调用者保存的寄存器  │  (X19-X28, 如果用了)
├──────────────────┤
│ 局部变量           │
├──────────────────┤
│ ...               │
├──────────────────┤
│ FP (X29) →────────├── 帧指针（指向上一个 FP）
├──────────────────┤
│ LR (X30)          │  返回地址
├──────────────────┤
│ ...               │
低地址 (SP)
```

### 典型函数 prologue/epilogue

```asm
func:
    stp x29, x30, [sp, #-16]!   ; 保存 FP + LR，SP -= 16
    mov x29, sp                  ; 设新 FP = 当前 SP
    sub sp, sp, #32              ; 局部变量空间
    ; ... 函数体 ...
    add sp, sp, #32              ; 恢复 SP
    ldp x29, x30, [sp], #16     ; 恢复 FP + LR，SP += 16
    ret                          ; 跳回 X30
```

### 栈回溯

从当前 FP(X29) 沿链表回溯：`FP → 保存的 LR → 上一个 FP → ...`，可打印完整调用栈。这是 GDB `backtrace` 和内核 `dump_stack()` 的原理。

## HFT 关联

理解栈帧对 HFT 调试至关重要：当交易系统在低延迟路径上 crash 时，通过 FP 链表回溯调用栈定位问题。但 `-fomit-frame-pointer` 优化会丢弃 FP，导致无法回溯——HFT 生产环境通常保留 FP（`-fno-omit-frame-pointer`）。此外，callee-saved 寄存器（X19-X28）在热路径函数中应尽量不使用，避免 prologue 中额外的 STP 开销（每对 STP 约 1-2 周期）。

## 自测题

1. **callee-saved 和 caller-saved 的区别是什么？为什么上下文切换只需保存 callee-saved？**

<details>
<summary>答案</summary>

**callee-saved**（X19-X28）：被调用函数负责保存——如果函数要用这些寄存器，必须在 prologue 保存、epilogue 恢复。**caller-saved**（X0-X18）：调用者负责保存——调用其他函数前如果要保留值，调用者自己保存。上下文切换时只需保存 callee-saved，因为 caller-saved 的值在函数调用边界本来就不保证保留——切换等价于一次函数调用，caller-saved 寄存器的值是"调用者已保存或不再需要"的。
</details>

2. **FP(X29) 在栈帧中指向哪里？如何用 FP 做栈回溯？**

<details>
<summary>答案</summary>

FP(X29) 指向**栈帧中保存的上一个 FP 的位置**。在该位置往上 8 字节是保存的 LR（返回地址）。栈回溯过程：读当前 FP → 从 [FP] 读上一个 FP → 从 [FP+8] 读 LR（即调用者地址）→ 打印该地址 → 跳到上一个 FP 继续 → 直到 FP 为 NULL 或到达栈底。
</details>

3. **X8 寄存器在两种场景下分别有什么用途？**

<details>
<summary>答案</summary>

两种用途：(1) **间接结果寄存器**——当函数返回大 struct（超过 16 字节）时，X8 指向调用者分配的内存缓冲区，被调用者通过 X8 写入返回值。(2) **syscall 号**——Linux AArch64 的 SVC 调用约定中，X8 存放系统调用号（不同于 x86 的 EAX），X0-X5 传参数，X0 返回返回值。
</details>

## 参考与延伸

- [§21.3 进程控制块 PCB](03-pcb.md) — PCB 中保存哪些寄存器
- [§21.5 上下文切换](05-context-switch.md) — switch_to 汇编实现
- [Ch07 A64 工程陷阱](../../chapter-07-a64-traps/notes/section-0-本章完整概述.md) — 调用约定相关的汇编陷阱
