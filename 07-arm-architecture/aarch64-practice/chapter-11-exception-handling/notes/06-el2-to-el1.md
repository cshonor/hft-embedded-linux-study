# §11.6 EL2 → EL1 实验

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

BenOS 启动时可能在 EL2 或 EL3（取决于平台），需要降到 EL1 才能运行普通内核代码。本节展示如何检测当前 EL 并通过配置系统寄存器 + ERET 降级。

## 核心要点

### 检测当前 EL

```asm
check_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2       // CurrentEL 的 EL 在 bit[3:2]
    cmp x0, #3
    b.eq from_el3
    cmp x0, #2
    b.eq from_el2
    b   in_el1
```

### 从 EL3 降到 EL1

```asm
from_el3:
    // 配置 SCR_EL3 允许 HVC 和非安全
    // 设置 EL2 的入口状态
    // ERET 到 EL2
```

### 从 EL2 降到 EL1

```asm
from_el2:
    // 配置 HCR_EL2（如禁用 EL2 虚拟化）
    // 设置 EL1 的 SCTLR/HCR
    // 设置 SPSR_EL2 = 目标 PSTATE
    // 设置 ELR_EL2 = in_el1 的地址
    // eret  →  降级到 EL1
in_el1:
    // 设置 VBAR_EL1, SP_EL1
```

### 降级流程

| 步骤 | 操作 |
|------|------|
| 1 | 读 CurrentEL 确定当前等级 |
| 2 | 如果 EL3：配 SCR_EL3，ERET 到 EL2 |
| 3 | 如果 EL2：配 HCR_EL2，ERET 到 EL1 |
| 4 | 在 EL1：设 VBAR_EL1、SP_EL1 |

> 树莓派启动通常从 EL3 开始。Linux 启动代码 `head.S` 会处理降级。

## HFT 关联

在树莓派或 QEMU 上做裸金属 HFT 开发时，必须先完成 EL 降级。如果从 EL2 启动但没有正确配置 HCR_EL2（如没禁用虚拟化陷阱），后续的 MMIO 操作可能被 trap 到 EL2，引入不可预期的延迟。确保运行在 EL1 且没有未清除的 trap，是 HFT 延迟确定性的前提。

## 自测题

1. **为什么 BenOS 启动时需要从 EL2/EL3 降到 EL1？**

<details>
<summary>答案</summary>

因为 BenOS 是普通裸金属内核，运行在 EL1。EL2 是 Hypervisor 层，EL3 是 Secure Monitor 层。如果从 EL2/EL3 启动但不降级，某些操作（如 MMU 配置、中断处理）的行为与 EL1 不同（可能被 trap）。降到 EL1 后才能使用标准的 EL1 系统寄存器集。
</details>

2. **从 EL2 降到 EL1 的 ERET 是怎么实现降级的？**

<details>
<summary>答案</summary>

设置 SPSR_EL2 的 M[3:0] 字段为目标 EL（EL1 = 0b0101），设置 ELR_EL2 为降级后的入口地址。执行 ERET 时，硬件从 SPSR_EL2 恢复 PSTATE（包含 EL=EL1），从 ELR_EL2 恢复 PC。这样 CPU 就在 EL1 继续执行了。
</details>

3. **CurrentEL 寄存器的 EL 字段在哪些位？怎么提取？**

<details>
<summary>答案</summary>

EL 在 **bit[3:2]**。提取：`mrs x0, CurrentEL; lsr x0, x0, #2`，x0 即当前 EL 值（0/1/2/3）。
</details>

## 参考与延伸

- [§11.2 异常等级切换](02-el-switch.md) — EL 切换的基本原理
- [§11.3 异常向量表](03-vector-table.md) — 降级到 EL1 后第一件事是设 VBAR
- [§11.7 实验要点](07-lab.md) — 实验 11-1 切换到 EL1
