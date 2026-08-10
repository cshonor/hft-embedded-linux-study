# §12.3 通用定时器中断

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8 每个 CPU 核内置通用定时器（Generic Timer），通过设置 CNTP_CTL/TVAL 系统寄存器产生定时中断。本节展示定时器中断的设置和 ISR 处理模式。

## 核心要点

### 通用定时器系统寄存器

| 系统寄存器 | 作用 |
|-----------|------|
| `CNTFRQ_EL0` | 定时器频率（Hz） |
| `CNTP_CTL_EL0` | 物理定时器控制（Enable/Imask/Status） |
| `CNTP_TVAL_EL0` | 定时器倒计值 |
| `CNTP_CVAL_EL0` | 定时器比较值（绝对） |
| `CNTPCT_EL0` | 当前物理计数（只读） |

### 设置定时器中断

```asm
setup_timer:
    // 1. 读定时器频率
    mrs x0, CNTFRQ_EL0
    // x0 = 频率（如 62500000 = 62.5MHz）

    // 2. 设置倒计值（如 1 秒后中断）
    mov x1, x0              // 1 秒 = 频率值
    msr CNTP_TVAL_EL0, x1

    // 3. 使能定时器
    mov x0, #1              // Enable=1, Imask=0
    msr CNTP_CTL_EL0, x0
    ret
```

### 定时器中断 ISR

```c
void timer_irq_handler(void) {
    // 1. 读 GIC 中断确认寄存器（IAR）→ 获取中断号
    uint32_t irq = GIC_READ(IAR);

    // 2. 判断是定时器中断
    if (irq == TIMER_IRQ) {
        // 3. 清定时器中断状态
        //    重设 TVAL → 下次中断
        write_sysreg(new_tval, CNTP_TVAL_EL0);
    }

    // 4. 写 GIC EOIR → 通知 GIC 处理完毕
    GIC_WRITE(EOIR, irq);
}
```

### CNTP_CTL 字段

| 位 | 名称 | 说明 |
|----|------|------|
| 0 | Enable | 0=禁制定时器，1=使能 |
| 1 | Imask | 0=允许中断，1=屏蔽中断 |
| 2 | Status | 只读，1=定时器条件满足（待处理中断） |

> **必须写 EOIR**：不写则 GIC 认为中断未处理完，不再发后续中断。
> **必须重设 TVAL**：不重设则定时器只触发一次。

## HFT 关联

通用定时器是 HFT 系统的心跳——用于超时检测、心跳包、延迟测量。`CNTPCT_EL0`（当前计数）是 ARM64 上最高精度的时钟源，在 Pi5（A76）上分辨率约 16ns（62.5MHz）。HFT 中常用 `mrs x0, CNTPCT_EL0` 做纳秒级时间戳，比 x86 的 TSC 更简单直接。定时器中断的设置方式（TVAL 倒计 vs CVAL 绝对比较）影响中断精度：CVAL 模式可以避免累积误差。

## 自测题

1. **如何用通用定时器实现每 1 秒一次的中断？需要写哪些寄存器？**

<details>
<summary>答案</summary>

1. `mrs x0, CNTFRQ_EL0` 读频率（如 62.5MHz）
2. `msr CNTP_TVAL_EL0, x0` 设倒计值 = 频率值（1 秒后触发）
3. `msr CNTP_CTL_EL0, #1` 使能定时器（Enable=1, Imask=0）

中断 ISR 中必须**重设 TVAL**（否则只触发一次）+ **写 GIC EOIR**（否则 GIC 卡住）。
</details>

2. **CNTP_CTL 的 Imask 位和 PSTATE 的 I 位有什么区别？**

<details>
<summary>答案</summary>

- **Imask**（CNTP_CTL bit[1]）：定时器级别的屏蔽，只屏蔽**当前定时器**的中断
- **I 位**（PSTATE.DAIF bit[1]）：CPU 级别的屏蔽，屏蔽**所有 IRQ** 中断

两者都需要清除才能让定时器中断到达 CPU：Imask=0 且 PSTATE.I=0。
</details>

3. **TVAL 模式和 CVAL 模式有什么区别？哪个更适合精确周期中断？**

<details>
<summary>答案</summary>

- **TVAL**：倒计值，每次写入后从该值递减到 0 触发。如果 ISR 有延迟，下次中断时间会偏移（累积误差）
- **CVAL**：绝对比较值，与 CNTPCT 比较。可以在 ISR 中 `CVAL += period`，不受 ISR 延迟影响

**CVAL 更适合精确周期中断**，因为没有累积误差。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — 定时器中断在完整流程中的位置
- [§12.2 中断屏蔽](02-daif.md) — PSTATE.I 位影响定时器中断能否到达 CPU
- [Ch13 GIC-V2](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — GIC 如何管理定时器中断
