# Ch12 完整总结 · 中断处理

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

Ch11 建立了异常框架，本章聚焦 **IRQ 中断**的完整处理流程：中断源 → GIC → CPU → 保存现场 → ISR → 清中断 → 恢复现场。以通用定时器中断为案例。

---

## 12.1 中断处理全流程

```
硬件产生中断信号
  → GIC（中断控制器）仲裁、优先级
  → CPU IRQ 线拉高
  → 当前指令执行完后，CPU 响应
  → 硬件保存 ELR+SPSR，切到 EL1（如果来自 EL0）
  → 跳到 VBAR + 0x280（当前EL SPx IRQ）
  → 软件保存 X0-X30
  → 读 GIC 中断号 → 调用 ISR
  → ISR 处理 → 写 GIC EOIR（End of Interrupt）
  → 恢复 X0-X30
  → ERET
```

---

## 12.2 中断屏蔽（DAIF）

PSTATE 中的 **DAIF** 控制中断屏蔽：

| 位 | 含义 |
|----|------|
| D | Debug 异常屏蔽 |
| A | SError（异步错误）屏蔽 |
| **I** | **IRQ 屏蔽** ← 最常用 |
| F | FIQ 屏蔽 |

```asm
// 关中断（屏蔽 IRQ）
msr DAIFSet, #0xf       // 屏蔽全部（D+A+I+F）
// 或只屏蔽 IRQ
msr DAIFSet, #0x2       // 只设 I 位

// 开中断
msr DAIFClr, #0x2       // 清 I 位（允许 IRQ）
```

> **异常入口硬件自动设 DAIF**：进入异常时 PSTATE 的 I/F 位被设为 1（自动关中断）。  
> **Linux 的 `local_irq_disable()` / `local_irq_enable()`** 底层就是操作 DAIF。

---

## 12.3 通用定时器中断

ARMv8 每个 CPU 核有内置通用定时器（Generic Timer）：

| 系统寄存器 | 作用 |
|-----------|------|
| `CNTFRQ_EL0` | 定时器频率（Hz） |
| `CNTP_CTL_EL0` | 物理定时器控制（Enable/Imask/Status） |
| `CNTP_TVAL_EL0` | 定时器倒计值 |
| `CNTP_CVAL_EL0` | 定时器比较值（绝对） |
| `CNTPCT_EL0` | 当前物理计数（只读） |

```asm
// 设置定时器中断
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

> **必须写 EOIR**：不写则 GIC 认为中断未处理完，不再发后续中断。

---

## 12.4 中断现场保存/恢复 ⭐

```asm
// IRQ 入口（向量表跳来）
irq_entry:
    sub sp, sp, #272          // 预留空间
    stp x0,  x1,  [sp, #0]    // 保存 x0-x1
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    stp x30, xzr, [sp, #240]  // x30(LR) + padding

    // 调用 C 处理函数
    bl  do_irq

    // 恢复
    ldp x0,  x1,  [sp, #0]
    // ... 恢复 x0-x30
    ldp x28, x29, [sp, #224]
    ldp x30, xzr, [sp, #240]
    add sp, sp, #272

    eret
```

> 也可以用 `stp x29, x30, [sp, #240]` 保存 FP+LR，取决于是否需要调试栈回溯。

---

## 12.5 中断控制器演进

| 平台 | 中断控制器 | 版本 |
|------|-----------|------|
| Pi4B (BCM2711) | GIC-400 | **GICv2** |
| Pi5 (BCM2712) | GIC-600 | **GICv3** |
| QEMU `-M virt` | 默认 | **GICv3**（可配置 GICv2） |

> **Pi5 适配坑**：原书 GICv2 代码（寄存器映射、初始化流程）不能直接用在 Pi5 上。  
> 建议先在 QEMU `virt`（支持 GICv3）上做实验，再上 Pi5。详见 Ch13。

---

## 12.6 实验要点

| 实验 | 内容 | 平台 | Pi5 适配 |
|------|------|------|----------|
| 12-1 | 通用定时器中断 | Pi4B | 定时器基址→BCM2712 |
| 12-2 | 汇编保存恢复中断现场 | QEMU | — |

---

## 12.7 易错点清单

1. **忘写 EOIR** → GIC 不再发后续中断。
2. **中断中没重设 TVAL** → 定时器只触发一次。
3. **ISR 用了浮点** → 异常入口没保存 FP 状态，浮点寄存器损坏。
4. **中断嵌套没正确处理** → 默认异常入口硬件关 IRQ，要嵌套需手动开。
5. **Pi5 用 GICv3 但代码写 GICv2** → 寄存器完全不同。

---

## 书中思考题（自测）

1. 进入 IRQ 异常后，硬件自动做了什么？DAIF 的 I 位是什么状态？
2. 通用定时器中断怎么设置？需要写哪些系统寄存器？
3. 为什么中断处理完后必须写 EOIR？
4. 中断现场保存多少个寄存器？为什么 X0-X30 都要存？
5. Pi4B 和 Pi5 的中断控制器有什么不同？

**参考答案：**

1. 硬件保存 ELR+SPSR，切到目标 EL 的 SP；**I 位自动设 1**（关 IRQ）。  
2. `CNTFRQ_EL0`(频率) → `CNTP_TVAL_EL0`(倒计值) → `CNTP_CTL_EL0`(Enable=1)。  
3. 不写 EOIR → GIC 认为中断**未处理完**，不再发后续中断。  
4. **31 个**（X0-X30）；硬件不自动保存通用寄存器，ISR 可能覆盖。  
5. Pi4B=**GIC-400(GICv2)**，Pi5=**GIC-600(GICv3)**；寄存器映射和初始化流程不同。

---

上一章 [Ch11 异常处理](../../chapter-11-exception-handling/) · 下一章 [Ch13 GIC-V2](../../chapter-13-gic-v2/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
