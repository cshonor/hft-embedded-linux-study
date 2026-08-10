# §21.1 AArch64 C 语言陷阱

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

在 AArch64 裸机/内核开发中常见的 C 语言陷阱：栈对齐、函数指针调用、可变参数传递规则、volatile 在 MMIO 场景下的必要性。

## 核心要点

### 栈对齐

AAPCS64 要求 SP **16 字节对齐**。编译器在函数 prologue 自动保证，但裸机/汇编中手动操作 SP 时必须注意：

```asm
; 错误：只减 8 字节，SP 不再 16 字节对齐
sub sp, sp, #8
str x0, [sp]

; 正确：减 16 字节
sub sp, sp, #16
str x0, [sp]
```

### 函数指针

```c
void (*handler)(void) = irq_handler;  // 函数指针存地址
handler();  // 编译为 BLR 调用
```

AArch64 函数指针就是普通 64 位地址，没有 x86 的 thunk/间接跳转层。

### 可变参数

| 参数序号 | 传递方式 |
|----------|----------|
| 1-8 | X0-X7 寄存器 |
| 9+ | 栈传递 |

`va_list` 在 AArch64 上实现复杂：前 8 个参数可能同时在寄存器和栈保存区中，需要跟踪剩余寄存器数量。

### volatile

裸机/驱动中 `volatile` 必不可少——MMIO 寄存器、中断共享变量：

```c
volatile uint32_t *uart_dr = (uint32_t *)0x09000000;
*uart_dr = 'A';  // 每次都真正写内存，不被优化掉
```

| 场景 | 需要 volatile | 原因 |
|------|--------------|------|
| MMIO 寄存器 | ✅ | 硬件可能随时改变值 |
| 中断共享变量 | ✅ | ISR 修改的变量编译器看不到 |
| DMA buffer | ✅ | 硬件 DMA 引擎会修改 |
| 普通局部变量 | ❌ | 编译器优化是安全的 |

## HFT 关联

HFT 系统中网卡寄存器映射（如 Solarflare/Mellanox NIC 的 doorbell 寄存器）必须用 volatile 写入。但 volatile 不是内存屏障——它只防止编译器优化，不防止 CPU 乱序。对 MMIO 的访问需要用 `__iowmb()` / `__iowmb64()` 等屏障确保写入顺序。在用户态 DPDK 中，MMIO 写用 `rte_write64()` 内联汇编 + DSB。

## 自测题

1. **AAPCS64 要求 SP 对齐到多少字节？手写汇编时违反会怎样？**

<details>
<summary>答案</summary>

AAPCS64 要求 SP **16 字节对齐**。违反时：某些指令（如 `STP X0, X1, [SP]`）会触发对齐异常（Synchronous Data Abort，ESR.EC=0x24）。编译器生成的代码自动保证对齐，但手写汇编中 `SUB SP, SP, #8` 这样非 16 倍数的操作会导致后续 STP 崩溃。
</details>

2. **volatile 能替代内存屏障吗？为什么？**

<details>
<summary>答案</summary>

**不能**。volatile 只阻止**编译器**优化（如合并多次读写、消除冗余读），但不阻止 **CPU** 乱序执行。例如 `volatile` 写 A 后写 B，CPU 可能先执行写 B。需要 DMB/DSB 等硬件屏障来保证 CPU 层面的顺序。MMIO 通常映射为 Device-nGnRnE 内存（MAIR 属性），硬件保证该类型内存的访问顺序，但跨不同外设仍需屏障。
</details>

3. **AArch64 可变参数第 9 个参数怎么传？**

<details>
<summary>答案</summary>

前 8 个参数走 X0-X7 寄存器，**第 9 个及以后**走栈传递（调用者负责压栈，被调用者从栈上读取）。`va_list` 内部维护一个"剩余寄存器计数器"——如果可变参数从第 6 个开始（前 5 个是固定参数），则前 3 个可变参数还在寄存器中（X5-X7），从第 4 个可变参数开始走栈。
</details>

## 参考与延伸

- [§21.2 调用约定与栈帧](02-calling-convention.md) — AAPCS64 完整栈帧结构
- [Ch10 GCC 内联汇编](../../chapter-10-gcc-inline-asm/notes/section-0-本章完整概述.md) — volatile asm 与编译器优化的关系
