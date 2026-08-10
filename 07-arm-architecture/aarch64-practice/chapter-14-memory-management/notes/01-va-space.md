# §14.1 虚拟地址空间

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8-A 支持 48 位虚拟地址（可配 52 位 LPA），分为低位用户空间（TTBR0）和高位内核空间（TTBR1）。VA 高位决定走哪个页表基址。

## 核心要点

### 地址空间布局

```
0xFFFF_FFFF_FFFF_FFFF
├── 高位空间 (TTBR1)：内核空间
│   0xFFFF_0000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF
├── ...
├── 低位空间 (TTBR0)：用户空间
│   0x0000_0000_0000_0000 ~ 0x0000_FFFF_FFFF_FFFF
0x0000_0000_0000_0000
```

### 关键寄存器

| 寄存器 | 管理范围 |
|--------|----------|
| **TTBR0_EL1** | 用户空间页表基址（低地址） |
| **TTBR1_EL1** | 内核空间页表基址（高地址） |
| **TCR_EL1** | 翻译控制（VA 宽度、ASID、walk 等） |
| **SCTLR_EL1** | 系统控制（M=MMU 开关、C=DCache 开关） |

> VA 高位 bit[63:48] 全 1 → 走 TTBR1（内核）；全 0 → 走 TTBR0（用户）。

### TCR_EL1 关键字段

| 字段 | 作用 |
|------|------|
| T0SZ/T1SZ | TTBR0/TTBR1 的 VA 宽度（48位时=16） |
| IRGN0/1 | Inner Cacheable 属性 |
| ORGN0/1 | Outer Cacheable 属性 |
| SH0/1 | Shareability 属性 |
| AS | ASID 宽度（0=8bit, 1=16bit） |

## HFT 关联

HFT 系统通常运行在 EL1 内核态，所有代码和数据在 TTBR1 高位空间。理解 TTBR0/TTBR1 分离有助于设计裸金属 HFT 平台——可以直接用 TTBR0 映射 MMIO 区域（避免内核页表污染），用 TTBR1 映射代码和数据。TCR 的 cache 属性设置直接影响页表 walk 的性能：如果页表本身可缓存，TLB miss 时 MMU walker 可以从 L2/L3 读页表而非 DRAM。

## 自测题

1. **VA 高位 bit[63:48] 全 1 时走哪个 TTBR？为什么？**

<details>
<summary>答案</summary>

走 **TTBR1**（内核空间）。ARM 硬件根据 VA 高位判断：bit[63:48] 全 1 → TTBR1（高地址/内核），全 0 → TTBR0（低地址/用户）。这是硬件行为，不需要软件判断。
</details>

2. **TTBR0 和 TTBR1 分别管什么地址空间？进程切换时哪个会变？**

<details>
<summary>答案</summary>

- TTBR0：用户空间（低地址 0x0000...），每个进程不同 → 进程切换时**改变**
- TTBR1：内核空间（高地址 0xFFFF...），所有进程共享 → 进程切换时**不变**

进程切换只需换 TTBR0_EL1，TTBR1_EL1 保持不变。
</details>

3. **48 位 VA 的用户空间和内核空间各有多大？**

<details>
<summary>答案</summary>

48 位 VA 总共 256TB。用户空间（TTBR0）：0x0000_0000_0000_0000 ~ 0x0000_FFFF_FFFF_FFFF = **128TB**。内核空间（TTBR1）：0xFFFF_0000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF = **128TB**。
</details>

## 参考与延伸

- [§14.2 四级页表](02-four-level-page-table.md) — VA 如何翻译为 PA
- [§14.6 开 MMU 流程](06-enable-mmu.md) — 设置 TTBR 的完整步骤
- [Ch17 TLB 管理](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — TTBR 切换与 TLB 维护
