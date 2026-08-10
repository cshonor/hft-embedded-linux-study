# §12.2 中断屏蔽（DAIF）

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

PSTATE 中的 DAIF 位控制 4 种异常的屏蔽：D（Debug）、A（SError）、I（IRQ）、F（FIQ）。进入异常时硬件自动设 DAIF 屏蔽中断，Linux 的 local_irq_disable/enable 底层操作 DAIF。

## DAIF 位详解

```
PSTATE:
  ... D A I F ...
       │ │ │ │
       │ │ │ └─ F: FIQ 屏蔽
       │ │ └─── I: IRQ 屏蔽（最常用）
       │ └───── A: SError 屏蔽
       └─────── D: Debug 异常屏蔽
```

| 位 | 含义 | 屏蔽的异常 | bit 位置 |
|----|------|-----------|---------|
| D | Debug 异常 | 断点、单步调试、watchpoint | bit[9] |
| A | SError | 系统总线异步错误 | bit[8] |
| **I** | **IRQ** | **外部硬件中断** | **bit[7]** |
| F | FIQ | 快速中断 | bit[6] |

## 操作指令

### ARMv8.1 便捷指令

```asm
// 关中断（屏蔽 IRQ）
msr DAIFSet, #0xf       // 屏蔽全部（D+A+I+F）
msr DAIFSet, #0x2       // 只设 I 位（屏蔽 IRQ）

// 开中断
msr DAIFClr, #0xf       // 清除全部
msr DAIFClr, #0x2       // 只清 I 位（允许 IRQ）
```

| DAIFSet/Clr 立即数 | 屏蔽/清除的位 |
|-------------------|-------------|
| #0x1 | F 位（FIQ） |
| #0x2 | I 位（IRQ） |
| #0x4 | A 位（SError） |
| #0x8 | D 位（Debug） |
| #0xf | 全部（DAIF） |

### 传统 MRS/MSR 方式

```asm
// 读 DAIF
mrs x0, DAIF

// 修改后写回
orr x0, x0, #(1 << 7)   // 设 I 位（关 IRQ）
msr DAIF, x0

// 或直接写
msr DAIF, #0x3c0         // 全屏蔽（DAIF 都设 1）
```

## 关键规则

| 场景 | DAIF 状态 | 说明 |
|------|-----------|------|
| 正常用户态运行 | I=0, F=0 | 中断开启 |
| 进入 IRQ 异常 | I=1, F=1 | **硬件自动设**，防止嵌套 |
| ERET 返回 | 恢复异常前状态 | 从 SPSR 恢复 DAIF |
| `local_irq_disable()` | I=1 | `msr DAIFSet, #0x2` |
| `local_irq_enable()` | I=0 | `msr DAIFClr, #0x2` |
| `local_irq_save(flags)` | I=1（保存旧值） | 读 DAIF 后设 I |
| `local_irq_restore(flags)` | 恢复保存值 | 按保存值设 DAIF |

## 异常进入时的 DAIF 行为

```
异常发生前：PSTATE.I=0 (IRQ 开启)
    ↓
异常发生 → 硬件保存 PSTATE 到 SPSR_ELx
    ↓
硬件设置 PSTATE：
  - I=1 (自动屏蔽 IRQ)
  - F=1 (自动屏蔽 FIQ) [可选，取决于异常类型]
  - A=1 (自动屏蔽 SError) [某些异常]
    ↓
异常处理中（IRQ 被屏蔽，不会嵌套）
    ↓
ERET → 从 SPSR_ELx 恢复 PSTATE
  - I=0 (恢复到异常前的状态)
```

## 中断嵌套

```asm
irq_handler:
    // 硬件已自动屏蔽 IRQ (I=1)
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    // ... 保存现场

    // 如果需要允许更高优先级中断嵌套
    msr DAIFClr, #0x2       // 手动开 IRQ
    // 此时更高优先级的 IRQ 可以打断

    bl do_irq

    // ISR 返回后，可能被更高优先级中断打断过
    msr DAIFSet, #0x2       // 重新关 IRQ（安全恢复）
    // ... 恢复现场
    add sp, sp, #272
    eret
```

### 嵌套的风险

| 风险 | 说明 |
|------|------|
| 栈溢出 | 每层嵌套分配 272 字节栈 → 深嵌套消耗大量栈 |
| 优先级反转 | 低优先级 ISR 阻塞高优先级 → 需 GIC 优先级控制 |
| 死锁 | ISR 中开中断后获取锁 → 被同类型中断打断 → 死锁 |
| 状态不一致 | 嵌套时共享数据需额外保护 |

## Linux 中的 DAIF 操作

```c
// Linux 内核中的中断控制
local_irq_disable();     // msr DAIFSet, #0x2
local_irq_enable();      // msr DAIFClr, #0x2

// 保存+关闭，恢复
unsigned long flags;
local_irq_save(flags);   // 读 DAIF 到 flags, 然后 DAIFSet
// ... 临界区
local_irq_restore(flags); // 按 flags 恢复 DAIF
```

## HFT 关联

HFT 系统中，DAIF 控制是延迟确定性的核心。在交易关键路径上，通常 `DAIFSet #0xf` 屏蔽所有中断，确保交易逻辑不被打断。但屏蔽中断意味着网卡数据无法通过中断通知 CPU——需要用轮询模式。理解 DAIF 的硬件自动行为很重要：进入异常时硬件自动关 IRQ，所以 ISR 中默认不会嵌套中断，除非手动 `DAIFClr #0x2`。

## 自测题

1. **进入 IRQ 异常后，I 位的值是什么？为什么？**
<details><summary>答案</summary>
I 位被**自动设为 1**（屏蔽 IRQ）。这是硬件行为——进入异常时 PSTATE 的 I/F 位自动置 1，防止在处理当前中断时被新的 IRQ 打断（默认不嵌套）。ERET 时从 SPSR 恢复，I 位回到异常前的值。
</details>

2. **`msr DAIFSet, #0x2` 和 `msr DAIFClr, #0x2` 分别做什么？**
<details><summary>答案</summary>
`DAIFSet, #0x2` = **设 I 位**（bit[7] 置 1，屏蔽 IRQ，关中断）。`DAIFClr, #0x2` = **清 I 位**（bit[7] 清 0，允许 IRQ，开中断）。0x2 = bit[1] 对应 I 位的 DAIFSet/Clr 立即数编码。DAIFSet/Clr 是 ARMv8.1 引入的便捷指令，不需要 MRS+修改+MSR 三步。
</details>

3. **ERET 返回时 DAIF 会变成什么值？**
<details><summary>答案</summary>
ERET 从 **SPSR_ELx 恢复 PSTATE**，包括 DAIF。即 DAIF 回到**异常发生前的状态**。如果异常前 I=0（开中断），ERET 后 I=0；如果异常前 I=1（关中断），ERET 后 I=1。不需要软件手动恢复 DAIF。
</details>

4. **`local_irq_save(flags)` 和 `local_irq_disable()` 有什么区别？**
<details><summary>答案</summary>
`local_irq_disable()` 直接关中断，不保存之前的状态 → 适用于确定之前是开中断的场景。`local_irq_save(flags)` 先读 DAIF 保存到 flags，再关中断 → 适用于不确定之前是否已关中断的场景，配合 `local_irq_restore(flags)` 恢复到之前的状态。
</details>

5. **为什么中断嵌套需要额外小心？**
<details><summary>答案</summary>
（1）栈溢出：每层嵌套 272B 栈空间（2）死锁：ISR 中开中断后获取锁，被同类型中断打断 → 递归加锁（3）数据竞争：嵌套时多个 ISR 访问共享数据需额外保护（4）优先级反转：低优先级 ISR 开中断后被中优先级 ISR 打断，阻塞高优先级中断。所以 Linux 默认不允许中断嵌套。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — DAIF 在流程中的位置
- [§12.4 中断现场保存](04-context-save.md) — ISR 中如果需要嵌套中断
- [§12.7 易错点](07-pitfalls.md) — 中断嵌套的陷阱
