# 1.5-1.6 必背小结与思考题

> 来源：§1.5-1.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

第 1 章核心知识点的必背总结，配合综合自测题检验理解程度。

## 必背 8 条

### 1. 产品线定位
- 只有 **Cortex-A** 有 AArch64；Cortex-M / Cortex-R 没有
- Cortex-A = 应用处理器（跑 Linux），Cortex-M = MCU（裸机/RTOS），Cortex-R = 硬实时

### 2. 异常等级
- **EL0-EL3** 替代 ARMv7 的 7 种工作模式
- EL0（用户）→ EL1（内核）→ EL2（虚拟化）→ EL3（安全监控）
- 异常只能升 EL，ERET 才能降 EL

### 3. 寄存器
- **X0-X30**：31 个通用寄存器，64 位
- **W0-W30**：对应低 32 位，写 W 会**自动清零高 32 位**
- **XZR/WZR**：零寄存器，读出 0，写入丢弃
- **SP**：栈指针（独立于 X0-X30）
- **X30 = LR**（链接寄存器）
- **PC**：程序计数器（不能直接读写，用 ADR/ADR P 获取）

### 4. PSTATE
| 字段 | 含义 |
|------|------|
| N | 负数标志 |
| Z | 零标志 |
| C | 进位/借位 |
| V | 溢出 |
| D | Debug 异常屏蔽 |
| A | SError 屏蔽 |
| I | IRQ 屏蔽（`msr daifset, #2` 关 IRQ） |
| F | FIQ 屏蔽 |

### 5. 异常机制
- 发生异常 → **硬件自动保存**：PSTATE → SPSR_ELx，返回地址 → ELR_ELx
- **通用寄存器由软件保存**（异常向量中用 STP 保存 X0-X29）
- 异常向量表基址在 **VBAR_ELx**

### 6. ERET
- **ERET**：异常返回，原子恢复 SPSR → PSTATE + ELR → PC
- 从高 EL 回到低 EL 的唯一方式

### 7. A64 指令特点
- 定长 32 位（每条指令 4 字节）
- 取消条件执行（除少数指令），取消 IT 块
- 新增 CSEL/CSET 条件选择指令
- LDP/STP 双寄存器加载/存储

### 8. 内存访问
- **LDXR/STXR**：独占访问（原子操作基础）
- **LDAR/STLR**：acquire/release 语义
- **DMB/DSB/ISB**：内存屏障

## 速查表：核心寄存器

| 寄存器 | 读 | 写 | 用途 |
|--------|-----|-----|------|
| CurrentEL | `mrs x0, CurrentEL` | - | 当前 EL |
| DAIF | `mrs x0, daif` | `msr daif, x0` | 中断屏蔽 |
| NZCV | `mrs x0, nzcv` | `msr nzcv, x0` | 条件标志 |
| VBAR_EL1 | `mrs x0, VBAR_EL1` | `msr VBAR_EL1, x0` | 异常向量基址 |
| TTBR0_EL1 | `mrs x0, TTBR0_EL1` | `msr TTBR0_EL1, x0` | 用户页表 |
| TTBR1_EL1 | `mrs x0, TTBR1_EL1` | `msr TTBR1_EL1, x0` | 内核页表 |
| SCTLR_EL1 | `mrs x0, SCTLR_EL1` | `msr SCTLR_EL1, x0` | 系统控制（MMU/Cache） |
| CNTVCT_EL0 | `mrs x0, CNTVCT_EL0` | - | 虚拟计时器 |
| MPIDR_EL1 | `mrs x0, MPIDR_EL1` | - | CPU ID |

## 综合自测题

1. 画出 EL0-EL3 的权限层级，标注 Linux 各组件对应的 EL。
<details><summary>答案</summary>
```
EL3 ─── Secure Monitor (TrustZone)
  ↑
EL2 ─── Hypervisor (KVM/Xen)
  ↑
EL1 ─── Linux Kernel
  ↑
EL0 ─── User Application
```
普通 Linux 系统中：用户程序 EL0，内核 EL1。EL2/EL3 通常由 TF-A 固件管理，Linux 内核启动时从 EL2/EL3 降级到 EL1。
</details>

2. 以下代码执行后 X0 的值是什么？
```asm
mov x0, #0xFFFFFFFFFFFFFFFF
mov w0, #0x12345678
```
<details><summary>答案</summary>
X0 = 0x0000000012345678。第一行设 X0 全 1（64 位）。第二行写 W0（低 32 位）= 0x12345678，**同时自动清零高 32 位**。这是 AArch64 的关键规则：写 W 寄存器会清零对应 X 寄存器的高 32 位。
</details>

3. 中断屏蔽位 DAIF 中，哪个位控制 IRQ？在内核中如何关中断？
<details><summary>答案</summary>
I 位控制 IRQ。关中断用 `msr daifset, #2`（置 I 位，#2 = bit1 = I 位偏移）。开中断用 `msr daifclr, #2`。C 代码中对应 `local_irq_disable()` / `local_irq_enable()`。F 位控制 FIQ，A 位控制 SError，D 位控制 Debug 异常。
</details>

4. 为什么 AArch64 抛弃了 ARMv7 的 R0-R15 命名？
<details><summary>答案</summary>
v7 的 R0-R15 中 R13=SP、R14=LR、R15=PC，混用了通用和特殊寄存器，导致编程时容易混淆。v8 把 SP/LR/PC 独立出来（SP/X30/PC），X0-X30 纯通用寄存器，设计更清晰。PC 不能直接读写（防止自修改代码问题），用 ADR 获取当前地址。
</details>

5. 写出 AArch64 读系统计时器、关中断、开中断的内联汇编。
<details><summary>答案</summary>
```c
/* 读计时器 */
u64 t;
asm volatile("mrs %0, cntvct_el0" : "=r"(t));

/* 关中断 */
asm volatile("msr daifset, #2" ::: "memory");

/* 开中断 */
asm volatile("msr daifclr, #2" ::: "memory");
```
daifset/daifclr 是 ARMv8.1 引入的便捷指令，不需要先 MRS 读 DAIF 再修改再 MSR 写回。
</details>

6. 异常发生时，硬件自动保存什么？软件需要保存什么？
<details><summary>答案</summary>
**硬件自动保存**：PSTATE → SPSR_ELx，返回地址 → ELR_ELx，切换到对应 EL 的 SP。**软件需要保存**：X0-X30 通用寄存器（在异常向量入口用 STP 压栈），因为硬件不保存通用寄存器，而异常处理函数可能修改它们。
</details>

## 参考与延伸

- 原书 §1.5-1.6
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [AArch64 命名](../../AARCH64-NAMING.md)
- [1.1 ARM 架构历史](01-arm-history-profiles.md)
- [1.2 四个异常等级](02-exception-levels.md)
