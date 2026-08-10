# §12.3 通用定时器中断

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8 每个 CPU 核内置通用定时器（Generic Timer），通过设置 CNTP_CTL/TVAL 系统寄存器产生定时中断。本节展示定时器中断的设置和 ISR 处理模式。

## 通用定时器系统寄存器

| 系统寄存器 | 作用 | 读写 |
|-----------|------|------|
| `CNTFRQ_EL0` | 定时器频率（Hz） | R/W（EL0 限读） |
| `CNTP_CTL_EL0` | 物理定时器控制（Enable/Imask/Status） | R/W |
| `CNTP_TVAL_EL0` | 定时器倒计值（相对） | R/W |
| `CNTP_CVAL_EL0` | 定时器比较值（绝对） | R/W |
| `CNTPCT_EL0` | 当前物理计数（只读） | R |
| `CNTV_CTL_EL0` | 虚拟定时器控制 | R/W |
| `CNTV_TVAL_EL0` | 虚拟定时器倒计值 | R/W |
| `CNTVCT_EL0` | 当前虚拟计数（只读） | R |

### 物理定时器 vs 虚拟定时器

| 类型 | 寄存器前缀 | 用途 |
|------|----------|------|
| 物理定时器 | CNTP_* | EL1/EL3 使用（内核） |
| 虚拟定时器 | CNTV_* | EL0/EL1 使用（用户态可访问） |
| 安全物理定时器 | CNTPS_* | EL3 使用（安全世界） |

## 设置定时器中断

### 方式1：TVAL 倒计模式

```asm
setup_timer_tval:
    // 1. 读定时器频率
    mrs x0, CNTFRQ_EL0
    // x0 = 频率（如 62500000 = 62.5MHz）

    // 2. 设置倒计值（如 1 秒后中断）
    mov x1, x0              // 1 秒 = 频率值
    msr CNTP_TVAL_EL0, x1   // TVAL = 频率值，从此刻开始递减

    // 3. 使能定时器
    mov x0, #1              // Enable=1, Imask=0
    msr CNTP_CTL_EL0, x0
    isb
    ret
```

### 方式2：CVAL 绝对比较模式

```asm
setup_timer_cval:
    // 1. 读当前计数
    mrs x0, CNTPCT_EL0

    // 2. 加上周期值
    mrs x1, CNTFRQ_EL0      // 1 秒的 tick 数
    add x0, x0, x1          // 目标 = 当前 + 1秒

    // 3. 设置比较值
    msr CNTP_CVAL_EL0, x0

    // 4. 使能
    mov x0, #1
    msr CNTP_CTL_EL0, x0
    isb
    ret
```

## CNTP_CTL 字段

```
CNTP_CTL_EL0:
  bit[2]  Status  (只读)  - 1=定时器条件满足（待处理中断）
  bit[1]  Imask           - 1=屏蔽中断, 0=允许中断
  bit[0]  Enable          - 0=禁制定时器, 1=使能
```

| 操作 | CNTP_CTL 值 | 说明 |
|------|------------|------|
| 启动定时器（开中断） | 0b001 | Enable=1, Imask=0 |
| 暂停定时器 | 0b000 | Enable=0 |
| 屏蔽定时器中断 | 0b011 | Enable=1, Imask=1 |
| 检查中断是否 pending | 读 bit[2] | Status=1 表示中断待处理 |

## 定时器中断 ISR

```c
void timer_irq_handler(void) {
    // 1. 读 GIC 中断确认寄存器（IAR）→ 获取中断号
    uint32_t irq = read_gic_iar();

    // 2. 判断是定时器中断（PPI #30 = IRQ 30）
    if (irq == 30) {
        // 3. 重设 TVAL（下次中断）
        //    或用 CVAL 模式：CVAL += period
        u64 period = read_sysreg(CNTFRQ_EL0);  // 1秒
        write_sysreg(period, CNTP_TVAL_EL0);

        // 4. 可选：读取当前时间戳
        u64 now = read_sysreg(CNTPCT_EL0);

        // 5. 调用定时器回调
        timer_callback(now);
    }

    // 6. 写 GIC EOIR → 通知 GIC 处理完毕
    write_gic_eoir(irq);
}
```

### CVAL 模式的 ISR（无累积误差）

```c
void timer_irq_handler_cval(void) {
    uint32_t irq = read_gic_iar();
    if (irq == 30) {
        // CVAL += period，不依赖当前时间
        u64 period = read_sysreg(CNTFRQ_EL0);
        u64 cval = read_sysreg(CNTP_CVAL_EL0);
        write_sysreg(cval + period, CNTP_CVAL_EL0);

        timer_callback();
    }
    write_gic_eoir(irq);
}
```

## TVAL vs CVAL 对比

| 特性 | TVAL（倒计） | CVAL（绝对比较） |
|------|-------------|----------------|
| 设置方式 | 写相对值 | 写绝对值 |
| 累积误差 | 有（ISR 延迟影响） | 无 |
| 适合场景 | 单次定时 | 周期性定时 |
| ISR 中操作 | 重写 TVAL | CVAL += period |
| 精度 | 受 ISR 延迟影响 | 精确 |

## HFT 关联

通用定时器是 HFT 系统的心跳——用于超时检测、心跳包、延迟测量。`CNTPCT_EL0`（当前计数）是 ARM64 上最高精度的时钟源，在 Pi5（A76）上分辨率约 16ns（62.5MHz）。HFT 中常用 `mrs x0, CNTPCT_EL0` 做纳秒级时间戳，比 x86 的 TSC 更简单直接。定时器中断的设置方式（TVAL 倒计 vs CVAL 绝对比较）影响中断精度：CVAL 模式可以避免累积误差。

## 自测题

1. **如何用通用定时器实现每 1 秒一次的中断？需要写哪些寄存器？**
<details><summary>答案</summary>
（1）`mrs x0, CNTFRQ_EL0` 读频率（如 62.5MHz）（2）`msr CNTP_TVAL_EL0, x0` 设倒计值 = 频率值（1 秒后触发）（3）`msr CNTP_CTL_EL0, #1` 使能定时器（Enable=1, Imask=0）。中断 ISR 中必须**重设 TVAL**（否则只触发一次）+ **写 GIC EOIR**（否则 GIC 卡住）。
</details>

2. **CNTP_CTL 的 Imask 位和 PSTATE 的 I 位有什么区别？**
<details><summary>答案</summary>
**Imask**（CNTP_CTL bit[1]）：定时器级别的屏蔽，只屏蔽**当前定时器**的中断信号。**I 位**（PSTATE.DAIF bit[7]）：CPU 级别的屏蔽，屏蔽**所有 IRQ** 中断。两者都需要清除才能让定时器中断到达 CPU：Imask=0 且 PSTATE.I=0。只清一个中断仍无法到达。
</details>

3. **TVAL 模式和 CVAL 模式有什么区别？哪个更适合精确周期中断？**
<details><summary>答案</summary>
**TVAL**：倒计值，每次写入后从该值递减到 0 触发。如果 ISR 有延迟，下次中断时间会偏移（累积误差）。**CVAL**：绝对比较值，与 CNTPCT 比较。可以在 ISR 中 `CVAL += period`，不受 ISR 延迟影响。**CVAL 更适合精确周期中断**，因为没有累积误差。
</details>

4. **CNTPCT_EL0 和 CNTVCT_EL0 有什么区别？HFT 中用哪个？**
<details><summary>答案</summary>
CNTPCT_EL0 = 物理计数（Physical Count），不受虚拟化影响。CNTVCT_EL0 = 虚拟计数（Virtual Count），可能被 Hypervisor 偏移（EL2 可设 offset）。裸机 HFT 用 CNTPCT_EL0（物理计数，确定性好）。在虚拟机中 CNTVCT_EL0 可能被 Hypervisor 操纵。Linux 用户态通常用 CNTVCT_EL0（EL0 可访问）。
</details>

5. **定时器中断的 PPI 编号是多少？为什么是 PPI 而不是 SPI？**
<details><summary>答案</summary>
通用定时器中断通常是 PPI #30（中断号 30）。是 PPI（Private Peripheral Interrupt）因为每个 CPU 核有自己独立的定时器 → 中断只送到对应的 CPU，不需要共享。如果是 SPI（共享），多个 CPU 的定时器中断会竞争同一中断号 → 需要额外路由逻辑。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — 定时器中断在完整流程中的位置
- [§12.2 中断屏蔽](02-daif.md) — PSTATE.I 位影响定时器中断能否到达 CPU
- [Ch13 GIC-V2](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — GIC 如何管理定时器中断
