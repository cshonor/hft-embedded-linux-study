# §21.2 调用约定与栈帧 ⭐

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

AAPCS64（ARM Architecture Procedure Call Standard for AArch64）的调用约定：寄存器分配规则、栈帧结构、FP/LR 的作用、栈回溯原理。

## 核心要点

### 寄存器分配

| 寄存器 | 角色 | 调用约定 | 是否保存 |
|--------|------|----------|---------|
| X0-X7 | 参数 / 返回值 | caller-saved | 调用者负责 |
| X8 | 间接结果 / syscall 号 | caller-saved | 调用者负责 |
| X9-X15 | 临时寄存器 | caller-saved | 调用者负责 |
| X16-X17 | IP0/IP1，链接器 PLT 用 | caller-saved | 调用者负责 |
| X18 | 平台寄存器（Linux 用于 SCS） | 保留 | 不可使用 |
| X19-X28 | callee-saved | 被调用者保存 | prologue 保存 |
| X29 (FP) | 帧指针 | callee-saved | prologue 保存 |
| X30 (LR) | 返回地址 | callee-saved | prologue 保存 |
| SP | 栈指针 | 16 字节对齐 | — |

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

    ; epilogue
    add sp, sp, #32              ; 恢复 SP
    ldp x29, x30, [sp], #16     ; 恢复 FP + LR，SP += 16
    ret                          ; 跳回 X30

// 带 callee-saved 的完整版本
func_with_callee:
    stp x29, x30, [sp, #-48]!   ; FP+LR + 3 对 callee-saved
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    mov x29, sp
    ; ... 函数体 ...
    ldp x21, x22, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #48
    ret
```

### 栈回溯原理

从当前 FP(X29) 沿链表回溯：

```c
// 栈回溯的核心循环
void backtrace(uint64_t fp) {
    while (fp != 0) {
        uint64_t saved_fp = *(uint64_t *)fp;      // 上一个 FP
        uint64_t saved_lr = *(uint64_t *)(fp + 8); // 返回地址
        printf("  [0x%lx] %s\n", saved_lr,
               symbol_lookup(saved_lr));
        fp = saved_fp;
    }
}

// GDB backtrace 和内核 dump_stack() 都用这个原理
// 前提：FP 链没有被 -fomit-frame-pointer 破坏
```

| 回溯方式 | 需要的条件 | 优缺点 |
|----------|-----------|--------|
| FP 链表 | -fno-omit-frame-pointer | 简单快速，但依赖 FP 链完整 |
| DWARF .eh_frame | 编译器生成 unwind info | 最准确，但表很大 |
| ARM unwind tables | -fno-unwind-tables 可关 | 比 DWARF 紧凑，内核常用 |
| shadow stack (X18) | Linux SCS 保护 | 安全但只存返回地址 |

### caller-saved vs callee-saved 对比

| 维度 | caller-saved (X0-X18) | callee-saved (X19-X28) |
|------|----------------------|----------------------|
| 谁负责保存 | 调用者（如果需要保留） | 被调用者（如果要用） |
| 保存位置 | 调用者的栈帧 | 被调用者的 prologue |
| 跨函数调用后 | 值不保证保留 | 值保证不变 |
| 上下文切换 | 不需要保存 | 必须保存 |
| HFT 热路径 | 优先使用（无保存开销） | 避免（prologue 有 STP 开销） |

### 返回大 struct 的约定

```c
// 返回 ≤16 字节的 struct：通过 X0/X1 返回
struct small { int a; int b; };  // 8 字节，X0 返回

// 返回 >16 字节的 struct：通过 X8 间接结果寄存器
struct big { long a, b, c, c; };  // 32 字节
// 调用者分配栈空间，X8 指向该空间
// 被调用者通过 X8 写入返回值
// 相当于隐式传了一个 hidden 指针参数
struct big foo(void) {
    struct big r = {1, 2, 3, 4};
    return r;  // 编译器生成: STR 到 [X8]
}
```

## HFT 关联

理解栈帧对 HFT 调试至关重要：当交易系统在低延迟路径上 crash 时，通过 FP 链表回溯调用栈定位问题。但 `-fomit-frame-pointer` 优化会丢弃 FP，导致无法回溯——HFT 生产环境通常保留 FP（`-fno-omit-frame-pointer`）。此外，callee-saved 寄存器（X19-X28）在热路径函数中应尽量不使用，避免 prologue 中额外的 STP 开销（每对 STP 约 1-2 周期）。

```c
// HFT 热路径函数优化：避免使用 callee-saved
// 编译选项：-fno-omit-frame-pointer -O2

// 好的做法：参数和局部变量尽量用 X0-X15
// 这样 prologue 只需保存 FP/LR（1 条 STP）
void hft_process(int_fast64_t symbol_id,
                  int_fast64_t price,
                  int_fast64_t qty) {
    // 全部用 caller-saved，prologue 极简
    // STP X29, X30, [SP, #-16]!
    // MOV X29, SP
    // ... 无额外保存
    // LDP X29, X30, [SP], #16
    // RET
}

// 坏的做法：用了太多 callee-saved → prologue 变长
void hft_bad_process() {
    register long a asm("x19");  // 强制用 callee-saved
    register long b asm("x20");
    register long c asm("x21");
    // prologue 要保存 X19-X21（额外 2 条 STP）
    // epilogue 要恢复（额外 2 条 LDP）
}
```

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

4. **-fomit-frame-pointer 优化对调试有什么影响？HFT 为什么通常禁用？**

<details>
<summary>答案</summary>

`-fomit-frame-pointer` 将 X29 当作普通 callee-saved 寄存器使用，不再维护 FP 链表。影响：(1) GDB `backtrace` 可能断链；(2) 内核 `dump_stack` 无法回溯；(3) 性能分析工具（perf）的调用栈采样失败。HFT 生产环境禁用它（`-fno-omit-frame-pointer`），因为延迟尖峰时需要完整调用栈定位瓶颈。性能损失极小（多一个 STP/LDP 对，约 2 周期），但调试收益巨大。
</details>

## 参考与延伸

- [§21.3 进程控制块 PCB](03-pcb.md) — PCB 中保存哪些寄存器
- [§21.5 上下文切换](05-context-switch.md) — switch_to 汇编实现
- [Ch07 A64 工程陷阱](../../chapter-07-a64-traps/notes/section-0-本章完整概述.md) — 调用约定相关的汇编陷阱
