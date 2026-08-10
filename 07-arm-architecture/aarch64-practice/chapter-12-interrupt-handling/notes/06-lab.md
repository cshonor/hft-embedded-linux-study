# §12.6 实验要点

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：通用定时器中断和汇编保存恢复中断现场。从 QEMU 上验证定时器中断，到手写汇编保存/恢复代码。

## 实验列表

| 实验 | 内容 | 平台 | Pi5 适配 |
|------|------|------|----------|
| 12-1 | 通用定时器中断 | Pi4B | 定时器基址→BCM2712 |
| 12-2 | 汇编保存恢复中断现场 | QEMU | — |

---

## 实验 12-1：通用定时器中断

### 目标

设置通用定时器产生周期性中断，在 ISR 中计数并打印。

### 代码

```asm
// timer_init.S
timer_init:
    // 1. 读定时器频率
    mrs x0, CNTFRQ_EL0
    // x0 = 62500000 (62.5MHz)

    // 2. 设置 1 秒倒计值
    msr CNTP_TVAL_EL0, x0

    // 3. 使能定时器 (Enable=1, Imask=0)
    mov x0, #1
    msr CNTP_CTL_EL0, x0
    isb
    ret
```

```c
// timer_isr.c
static volatile int tick_count = 0;

void timer_irq_handler(void) {
    u32 irq = gic_read_iar();      // 读 GIC IAR

    if (irq == 30) {               // PPI #30 = 定时器
        // 重设 TVAL
        u64 freq;
        asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(freq));
        asm volatile("msr CNTP_TVAL_EL0, %0" :: "r"(freq));

        tick_count++;
        printf("Tick #%d\n", tick_count);
    }

    gic_write_eoir(irq);           // 写 GIC EOIR
}
```

### 操作步骤

```bash
# 编译
aarch64-linux-gnu-gcc -c timer_init.S -o timer_init.o
aarch64-linux-gnu-gcc -c timer_isr.c -o timer_isr.o
aarch64-linux-gnu-gcc -T linker.ld -nostdlib -o timer.elf *.o

# QEMU 运行（GICv2 模式）
qemu-system-aarch64 -M virt,gic-version=2 -cpu cortex-a57 \
    -kernel timer.elf -nographic

# 期望输出：
#   Tick #1
#   Tick #2
#   Tick #3
#   ...
```

### 常见问题

| 问题 | 原因 | 修复 |
|------|------|------|
| 只触发一次 | 没重设 TVAL | ISR 中 `msr CNTP_TVAL_EL0` |
| 完全不触发 | GIC 没使能定时器中断 | GIC 使能 PPI #30 |
| 间隔不对 | CNTFRQ 没读或频率值不对 | `mrs x0, CNTFRQ_EL0` |
| 触发后卡死 | 没写 EOIR | ISR 末尾 `gic_write_eoir(irq)` |

---

## 实验 12-2：汇编保存恢复中断现场

### 目标

手写 IRQ 入口的寄存器保存/恢复代码，验证正确性。

### 验证方法

```asm
// 主程序：给寄存器赋已知值
main:
    mov x0, #0xAAAAAAAA
    mov x1, #0xBBBBBBBB
    mov x2, #0xCCCCCCCC
    // ... x0-x30 = 已知值

    // 触发定时器中断
    bl timer_init

wait_loop:
    // 中断会打断这里
    // ISR 可能修改 x0-x30
    b wait_loop

    // 中断返回后检查 x0-x30 是否恢复正确
```

### 操作步骤

```bash
# 编译链接
aarch64-linux-gnu-gcc -T linker.ld -nostdlib -o irq_test.elf *.o

# GDB 调试
qemu-system-aarch64 -M virt,gic-version=2 -cpu cortex-a57 \
    -kernel irq_test.elf -nographic -S -gdb tcp::1234

aarch64-linux-gnu-gdb
(gdb) target remote :1234
(gdb) b irq_entry
(gdb) c
# 中断触发时：
(gdb) info registers x0-x30    # 检查保存前的值
(gdb) si                         # 单步执行 STP
# ... 执行完保存后
(gdb) info registers             # 检查栈上的值
(gdb) do_irq                     # ISR 执行
(gdb) eret                        # 恢复
(gdb) info registers x0-x30    # 应恢复为原值
```

### 测量中断延迟

```c
// 在触发定时器前记录时间戳
u64 t1 = read_sysreg(CNTPCT_EL0);
timer_init();

// 在 ISR 入口记录时间戳
void timer_irq_handler(void) {
    u64 t2 = read_sysreg(CNTPCT_EL0);
    u64 delay = (t2 - t1) * 1000000000ULL / CNTFRQ;  // ns
    printf("IRQ latency: %llu ns\n", delay);
    // ...
}
```

---

## HFT 关联

实验 12-1 是 HFT 定时器的基础——很多 HFT 功能依赖精确定时（如每微秒检查订单队列）。实验 12-2 的保存/恢复代码是中断延迟的瓶颈，理解每一行代码的开销有助于优化。建议在 QEMU 上完成两个实验后，用 `CNTPCT_EL0` 测量从中断触发到 ISR 开始执行的精确延迟。

## 自测题

1. **实验 12-1 中，定时器中断只触发一次就不再触发，最可能的原因是什么？**
<details><summary>答案</summary>
两个可能原因：（1）ISR 中**没有重设 CNTP_TVAL_EL0** → TVAL 减到 0 后不自动重载，定时器不再触发（2）**忘记写 GIC EOIR** → GIC 认为中断未处理完，不再送后续中断。修复：ISR 中先重设 TVAL，再写 EOIR。
</details>

2. **实验 12-2 中，如何验证保存/恢复代码是否正确？**
<details><summary>答案</summary>
方法：在触发中断前给 X0-X30 赋已知值（如 0xAAAA...、0xBBBB...），触发中断后 ISR 修改这些寄存器（如全清零），ERET 返回后检查 X0-X30 是否恢复为原值。如果所有寄存器值正确，说明保存/恢复代码无误。也可以用 GDB 在 ERET 前后对比寄存器值。
</details>

3. **Pi5 上做实验 12-1 需要修改什么？**
<details><summary>答案</summary>
（1）GIC 从 GICv2 改为 **GICv3**（寄存器和初始化流程不同）（2）定时器中断号可能不同（需查 BCM2712 数据手册，通常仍是 PPI #30）（3）GIC 基址不同（GICv3 的 Distributor/Redistributor 地址）（4）中断确认从读 GICC_IAR (MMIO) 改为读 ICC_IAR1_EL1 (系统寄存器)（5）中断结束改为 ICC_EOIR1_EL1。
</details>

4. **如何测量从中断触发到 ISR 开始执行的中断延迟？**
<details><summary>答案</summary>
在触发定时器前用 `mrs x0, CNTPCT_EL0` 记录 t1，在 ISR 入口（保存完现场后、调用 C 函数前）记录 t2。延迟 = (t2 - t1) / CNTFRQ * 1e9 ns。注意 CNTPCT 是自由运行的计数器，t2-t1 包含了从定时器触发到硬件保存到软件保存的全部延迟。
</details>

5. **QEMU 上用 `gic-version=2` 和 `gic-version=3` 的实验结果有区别吗？**
<details><summary>答案</summary>
功能上两者都能完成实验。延迟上有差异：GICv3 用系统寄存器（ICC_*_EL1）访问 GIC，GICv2 用 MMIO 访问。在 QEMU 上两者延迟差异不明显（QEMU 是软件模拟），但在真实硬件上 GICv3 更快。建议先在 GICv2 上验证逻辑正确性（代码简单），再迁移到 GICv3。
</details>

## 参考与延伸

- [§12.3 通用定时器中断](03-timer-interrupt.md) — 实验 12-1 的核心知识
- [§12.4 中断现场保存](04-context-save.md) — 实验 12-2 的核心代码
- [§12.7 易错点](07-pitfalls.md) — 实验中常见错误
