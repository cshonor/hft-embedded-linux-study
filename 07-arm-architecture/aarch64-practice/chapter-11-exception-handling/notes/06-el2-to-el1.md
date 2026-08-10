# §11.6 EL2 → EL1 实验

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

BenOS 启动时可能在 EL2 或 EL3（取决于平台），需要降到 EL1 才能运行普通内核代码。本节展示如何检测当前 EL 并通过配置系统寄存器 + ERET 降级。

## 检测当前 EL

```asm
check_el:
    mrs x0, CurrentEL        // 读当前异常等级
    lsr x0, x0, #2           // EL 在 bit[3:2]，右移 2 位
    cmp x0, #3
    b.eq from_el3            // 从 EL3 降级
    cmp x0, #2
    b.eq from_el2            // 从 EL2 降级
    b   in_el1               // 已经在 EL1，跳过
```

### CurrentEL 寄存器格式

```
 63                    4 3 2 1 0
┌──────────────────────┬─┬─┬───┐
│        Reserved       │EL│ 0 │
└──────────────────────┴─┬─┴───┘
                         └ bit[3:2] = EL (0/1/2/3)
```

## 从 EL3 降到 EL1

EL3 → EL2 → EL1，必须逐级降，不能跳级。

```asm
from_el3:
    // 配置 SCR_EL3（安全配置寄存器）
    mov x0, #0x5b1           // NS=1(非安全), HCE=1(允许HVC), SMD=1
    msr SCR_EL3, x0

    // 设置 SPSR_EL2（目标是 EL1，AArch64）
    mov x0, #0x3c5           // EL1h, IRQ/FIQ/SError masked
    msr SPSR_EL2, x0

    // 设置 ELR_EL2 = EL2 入口
    adr x0, from_el2
    msr ELR_EL2, x0

    eret                     // EL3 → EL2
```

## 从 EL2 降到 EL1

```asm
from_el2:
    // 配置 HCR_EL2（Hypervisor 配置寄存器）
    // 关键：禁用 EL2 陷阱，让 EL1 直接运行
    mov x0, #(1 << 31)       // RW=1 (EL1 用 AArch64)
    msr HCR_EL2, x0

    // 配置 SCTLR_EL1（系统控制寄存器）
    // 清除特定位，准备好 EL1 的 MMU/Cache 状态
    mrs x0, SCTLR_EL1
    bic x0, x0, #(1 << 0)    // M=0 (关 MMU)
    bic x0, x0, #(1 << 12)   // I=0 (关 I-Cache)
    msr SCTLR_EL1, x0

    // 设置 SPSR_EL2（目标是 EL1，AArch64）
    mov x0, #0x3c5           // EL1h, DAIF masked
    msr SPSR_EL2, x0

    // 设置 ELR_EL2 = EL1 入口地址
    adr x0, in_el1
    msr ELR_EL2, x0

    eret                     // EL2 → EL1

in_el1:
    // 现在运行在 EL1
    // 设置 VBAR_EL1（异常向量表）
    adrp x0, vector_table
    add  x0, x0, #:lo12:vector_table
    msr  VBAR_EL1, x0
    isb

    // 设置 SP_EL1
    mov sp, #0x80000          // 栈顶地址
```

## SPSR_EL2 的 M[3:0] 字段

| M[3:0] | 目标 EL | SP 选择 |
|--------|---------|--------|
| 0b0000 | EL0 | SP_EL0 |
| 0b0100 | EL0 | SP_EL0 (AArch32) |
| 0b0101 | EL1 | SP_EL0 |
| 0b0101 | EL1 | - |
| 0b1001 | EL1 | SP_EL1 (EL1h) |
| 0b1010 | EL2 | SP_EL0 |
| 0b1011 | EL2 | SP_EL2 (EL2h) |

> `EL1h` 表示使用 SP_EL1，`EL1t` 表示使用 SP_EL0。通常内核用 EL1h。

## 降级流程总览

| 步骤 | 操作 | 寄存器 |
|------|------|--------|
| 1 | 读 CurrentEL 确定当前等级 | CurrentEL |
| 2 | 如果 EL3：配 SCR_EL3，ERET 到 EL2 | SCR_EL3, SPSR_EL2, ELR_EL2 |
| 3 | 如果 EL2：配 HCR_EL2，ERET 到 EL1 | HCR_EL2, SPSR_EL2, ELR_EL2 |
| 4 | 在 EL1：设 VBAR_EL1、SP_EL1 | VBAR_EL1, SP_EL1 |

### 关键系统寄存器

| 寄存器 | 配置内容 |
|--------|---------|
| SCR_EL3 | 安全状态、HVC/SMC 使能 |
| HCR_EL2 | EL2 陷阱控制、EL1 RW 位 |
| SCTLR_EL1 | MMU/Cache 开关（降级前关闭） |
| SPSR_EL2 | 目标 PSTATE（EL、DAIF） |
| ELR_EL2 | 降级后的入口地址 |
| VBAR_EL1 | EL1 异常向量表 |

## Linux 内核 head.S 的降级

Linux 内核 `arch/arm64/kernel/head.S` 的启动流程：

```
1. 检查 CurrentEL
2. 如果在 EL3：
   - 配置 SCR_EL3
   - ERET → EL2
3. 如果在 EL2：
   - 配置 HCR_EL2（禁用虚拟化陷阱）
   - 配置 CNTHCTL_EL2（物理计时器访问）
   - 配置 HSTR_EL2（禁用 trap）
   - ERET → EL1
4. 在 EL1：
   - 设置 VBAR_EL1
   - 初始化页表
   - 开启 MMU
   - start_kernel()
```

## HFT 关联

在树莓派或 QEMU 上做裸金属 HFT 开发时，必须先完成 EL 降级。如果从 EL2 启动但没有正确配置 HCR_EL2（如没禁用虚拟化陷阱），后续的 MMIO 操作可能被 trap 到 EL2，引入不可预期的延迟。确保运行在 EL1 且没有未清除的 trap，是 HFT 延迟确定性的前提。

## 自测题

1. **为什么 BenOS 启动时需要从 EL2/EL3 降到 EL1？**
<details><summary>答案</summary>
因为 BenOS 是普通裸金属内核，设计运行在 EL1。EL2 是 Hypervisor 层（有不同的系统寄存器集），EL3 是 Secure Monitor 层。如果不降级，某些操作（如 MMU 配置、中断处理）的行为与 EL1 不同——可能被 trap 到更高 EL，引入不可预期的延迟。降到 EL1 后才能使用标准的 EL1 系统寄存器集。
</details>

2. **从 EL2 降到 EL1 的 ERET 是怎么实现降级的？**
<details><summary>答案</summary>
设置 SPSR_EL2 的 M[3:0] 字段为目标 EL（EL1h = 0b1001），设置 ELR_EL2 为降级后的入口地址。执行 ERET 时，硬件从 SPSR_EL2 恢复 PSTATE（包含 EL=EL1），从 ELR_EL2 恢复 PC。CPU 就在 EL1 继续执行了。ERET 是原子操作，不会被中断。
</details>

3. **CurrentEL 寄存器的 EL 字段在哪些位？怎么提取？**
<details><summary>答案</summary>
EL 在 **bit[3:2]**。提取：`mrs x0, CurrentEL; lsr x0, x0, #2`，x0 即当前 EL 值（0/1/2/3）。CurrentEL 是只读寄存器，不能通过 MSR 修改——EL 只能通过异常进入和 ERET 返回来切换。
</details>

4. **HCR_EL2 的 RW 位（bit[31]）有什么作用？不设置会怎样？**
<details><summary>答案</summary>
RW 位决定 EL1 使用 AArch64（RW=1）还是 AArch32（RW=0）。如果不设置 RW=1，EL1 默认可能用 AArch32 执行状态，导致 A64 指令无法执行。裸金属 AArch64 程序必须在降级时设 HCR_EL2.RW=1 确保 EL1 用 AArch64。
</details>

5. **为什么从 EL3 不能直接一步降到 EL1？**
<details><summary>答案</summary>
ARM 架构限制：ERET 只能恢复到 SPSR 中指定的 EL，且不能跨多级。从 EL3 ERET 可以到 EL2 或 EL3（SPSR_EL3 中指定），但不能直接到 EL1。所以必须 EL3→EL2→EL1 两步降级。每步设置目标 EL 的 SPSR 和 ELR。
</details>

## 参考与延伸

- [§11.2 异常等级切换](02-el-switch.md) — EL 切换的基本原理
- [§11.3 异常向量表](03-vector-table.md) — 降级到 EL1 后第一件事是设 VBAR
- [§11.7 实验要点](07-lab.md) — 实验 11-1 切换到 EL1
