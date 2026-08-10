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
str x0, [sp]        ; 单个 STR 不会崩，但后续 STP 会

; 正确：减 16 字节
sub sp, sp, #16
str x0, [sp]

; 最常见错误：异常入口手动保存寄存器时
; 错误：SP 可能不是 16 对齐
stp x0, x1, [sp, #-16]!  ; ← 如果进异常前 SP 不对齐，这里崩

; 正确：先对齐再操作
and sp, sp, #~0xF        ; 强制 16 字节对齐
stp x0, x1, [sp, #-16]!
```

| 场景 | 风险 | 修复方法 |
|------|------|----------|
| 手写汇编函数 | SP 非 16 对齐 | prologue 用 `sub sp, sp, #N`（N 是 16 倍数） |
| 异常入口 | 不知道被中断前 SP 状态 | 先 `and sp, sp, #~0xF` 对齐 |
| 中断嵌套 | SP 可能偏移 | 每级都保证 16 对齐 |
| 上下文切换 | 切换后 SP 来自 PCB | PCB 中保存的 SP 必须对齐 |

### 函数指针

```c
void (*handler)(void) = irq_handler;  // 函数指针存地址
handler();  // 编译为 BLR 调用

// AArch64 函数指针就是普通 64 位地址
// 没有 x86 的 thunk/间接跳转层
// BLR Xn: 跳到 Xn 指向的地址，LR = 下一条指令

// 函数指针表（中断分发常用）
typedef void (*irq_handler_t)(void);
irq_handler_t irq_table[64] = {
    [0] = timer_irq_handler,
    [1] = uart_irq_handler,
    // ...
};

void dispatch_irq(int irq_num) {
    if (irq_num < 64 && irq_table[irq_num])
        irq_table[irq_num]();  // BLR 调用
}
```

### 可变参数

| 参数序号 | 传递方式 | va_list 追踪 |
|----------|----------|-------------|
| 1-8 | X0-X7 寄存器 | 剩余寄存器计数器递减 |
| 9+ | 栈传递 | 从栈保存区读取 |

`va_list` 在 AArch64 上实现复杂——前 8 个参数可能同时在寄存器和栈保存区中：

```c
// AArch64 va_list 内部结构（简化）
typedef struct {
    void *__stack;     // 下一个栈参数地址
    void *__gr_top;    // 通用寄存器保存区顶部
    void *__vr_top;    // 浮点寄存器保存区顶部
    int   __gr_offs;   // 通用寄存器偏移（剩余字节数）
    int   __vr_offs;   // 浮点寄存器偏移
} va_list;

// 例如 printf(fmt, a, b, c, d, e, f, g, h, i, j)
// a-h 走 X0-X7，i/j 走栈
// va_start 后 __gr_offs = 8*8=64（8个寄存器用完）
// va_arg 读第 9 个参数时直接从 __stack 读
```

### volatile

裸机/驱动中 `volatile` 必不可少——MMIO 寄存器、中断共享变量：

```c
volatile uint32_t *uart_dr = (uint32_t *)0x09000000;
*uart_dr = 'A';  // 每次都真正写内存，不被优化掉

// 没有 volatile 的危险
uint32_t *status = (uint32_t *)0x09000018;
// 编译器可能优化为只读一次：
while ((*status & 0x1) == 0) {}  // ← 可能变成死循环！
// 编译器认为 *status 不会变，只读一次，永远循环

// volatile 强制每次重新读
volatile uint32_t *status = (uint32_t *)0x09000018;
while ((*status & 0x1) == 0) {}  // ← 每次循环都重新读
```

| 场景 | 需要 volatile | 原因 |
|------|--------------|------|
| MMIO 寄存器 | ✅ | 硬件可能随时改变值 |
| 中断共享变量 | ✅ | ISR 修改的变量编译器看不到 |
| DMA buffer | ✅ | 硬件 DMA 引擎会修改 |
| 多核共享变量 | ✅ | 其他核可能修改 |
| 普通局部变量 | ❌ | 编译器优化是安全的 |
| const 全局变量 | ❌ | 值不会改变 |

### volatile vs 内存屏障

```c
// volatile 只防编译器优化，不防 CPU 乱序
volatile uint32_t *doorbell = (uint32_t *)0x10000000;
volatile uint32_t *desc = (uint32_t *)0x10000100;

*desc = 0x1234;      // (1) 写描述符
*doorbell = 1;       // (2) 通知硬件

// 危险：CPU 可能先执行 (2) 再执行 (1)！
// volatile 保证两次写不被合并、不被消除
// 但不保证 CPU 执行顺序

// 正确：加 DMB 屏障
*desc = 0x1234;
asm volatile("dmb ishst" ::: "memory");  // Store-Store 屏障
*doorbell = 1;
```

| 层面 | 防什么 | 指令/关键字 |
|------|--------|------------|
| 编译器层 | 合并/消除/重排读写 | `volatile`, `barrier()` |
| CPU 层 | 乱序执行/写缓冲合并 | `DMB`, `DSB`, `ISB` |
| MMIO 层 | Device 内存属性保证顺序 | MAIR Device-nGnRnE |

## HFT 关联

HFT 系统中网卡寄存器映射（如 Solarflare/Mellanox NIC 的 doorbell 寄存器）必须用 volatile 写入。但 volatile 不是内存屏障——它只防止编译器优化，不防止 CPU 乱序。对 MMIO 的访问需要用 `__iowmb()` / `__iowmb64()` 等屏障确保写入顺序。在用户态 DPDK 中，MMIO 写用 `rte_write64()` 内联汇编 + DSB。

```c
// HFT DPDK 风格的 MMIO 写
static inline void rte_write64(uint64_t val, volatile void *addr) {
    asm volatile("str %0, [%1]"
                 : : "r"(val), "r"(addr) : "memory");
}

// 发送数据包：先写描述符，再写 doorbell
static inline void hft_tx_submit(volatile struct tx_desc *desc,
                                  uint32_t txq_id) {
    desc->addr = (uint64_t)pkt_buf;
    desc->len = pkt_len;
    // DMB 确保描述符写先于 doorbell 写
    asm volatile("dmb ishst" ::: "memory");
    rte_write64(1, NIC_BASE + TX_DOORBELL(txq_id));
}
```

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

4. **以下代码有什么问题？如何修复？**

```c
uint32_t *flag = (uint32_t *)MMIO_BASE;
*flag = CMD_START;
*flag = CMD_EXEC;
```

<details>
<summary>答案</summary>

两个问题：(1) **缺 volatile**——编译器可能合并两次写或消除第一次写（因为第二次覆盖了第一次的值），但硬件需要看到两次独立的写操作；(2) **缺屏障**——CPU 可能乱序执行两次写。修复：声明为 `volatile uint32_t *`，并在两次写之间加 `DMB` 屏障确保顺序。如果 MMIO 映射为 Device-nGnRnE，硬件保证同地址的顺序，但跨地址仍需屏障。
</details>

## 参考与延伸

- [§21.2 调用约定与栈帧](02-calling-convention.md) — AAPCS64 完整栈帧结构
- [Ch10 GCC 内联汇编](../../chapter-10-gcc-inline-asm/notes/section-0-本章完整概述.md) — volatile asm 与编译器优化的关系
- [Ch18 内存屏障](../../chapter-18-memory-barriers/notes/02-three-barriers.md) — DMB/DSB 屏障详解
