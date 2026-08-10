# §14.1 虚拟地址空间

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8-A 支持 48 位虚拟地址（可配 52 位 LPA），分为低位用户空间（TTBR0）和高位内核空间（TTBR1）。VA 高位决定走哪个页表基址。理解虚拟地址空间布局是配置 MMU 的前提。

## 核心要点

### 地址空间布局

```
0xFFFF_FFFF_FFFF_FFFF  +------------------+
                      |                  |
                      |  TTBR1 内核空间   |  128TB (48位VA)
                      |  (高地址区)       |
0xFFFF_0000_0000_0000  +------------------+
                      |    非 canonical   |  ← 禁止使用
                      |    地址区(空洞)    |     访问触发 fault
0x0000_FFFF_FFFF_FFFF  +------------------+
                      |                  |
                      |  TTBR0 用户空间   |  128TB (48位VA)
                      |  (低地址区)       |
0x0000_0000_0000_0000  +------------------+
```

> ARMv8 要求 VA 高位必须和 bit[47] 一致（canonical form），否则触发 fault。
> 这和 x86-64 的 canonical address 类似。

### 关键寄存器

| 寄存器 | 管理范围 | EL | 说明 |
|--------|----------|-----|------|
| **TTBR0_EL1** | 用户空间页表基址（低地址） | EL1 | 进程切换时更新 |
| **TTBR1_EL1** | 内核空间页表基址（高地址） | EL1 | 所有进程共享，不变 |
| **TCR_EL1** | 翻译控制（VA 宽度、ASID、walk 等） | EL1 | 配置一次 |
| **SCTLR_EL1** | 系统控制（M=MMU 开关、C=DCache 开关） | EL1 | M位控制MMU开关 |
| TTBR0_EL2 | Hypervisor 空间页表基址 | EL2 | 虚拟化使用 |
| VTTBR_EL2 | Guest OS 页表基址 | EL2 | 虚拟化 Stage-2 |

> VA 高位 bit[63:48] 全 1 → 走 TTBR1（内核）；全 0 → 走 TTBR0（用户）。
> 这是**硬件自动判断**，不需要软件干预。

### TCR_EL1 关键字段

| 字段 | 位 | 作用 | 典型值 |
|------|-----|------|--------|
| T0SZ | [5:0] | TTBR0 的 VA 宽度（48位时=16，表示空出16位） | 16 |
| T1SZ | [21:16] | TTBR1 的 VA 宽度（48位时=16） | 16 |
| IRGN0 | [9:8] | TTBR0 页表 Inner Cacheable 属性 | 0b01 (WB) |
| ORGN0 | [11:10] | TTBR0 页表 Outer Cacheable 属性 | 0b01 (WB) |
| SH0 | [13:12] | TTBR0 页表 Shareability | 0b11 (Inner) |
| TG0 | [15:14] | TTBR0 页大小（0=4KB） | 0 |
| IRGN1 | [25:24] | TTBR1 页表 Inner Cacheable 属性 | 0b01 (WB) |
| ORGN1 | [27:26] | TTBR1 页表 Outer Cacheable 属性 | 0b01 (WB) |
| SH1 | [29:28] | TTBR1 页表 Shareability | 0b11 (Inner) |
| TG1 | [31:30] | TTBR1 页大小（2=4KB） | 2 |
| AS | [36] | ASID 宽度（0=8bit, 1=16bit） | 0 |
| IPS | [34:32] | 中间物理地址宽度（PA 宽度） | 取决于平台 |

### 52 位 VA（LPA2）

| 特性 | 48 位 VA | 52 位 VA (ARMv8.2-LPA) |
|------|----------|----------------------|
| VA 宽度 | 48 位 | 52 位 |
| 地址空间 | 2 × 128TB | 2 × 4PB |
| T0SZ/T1SZ | 16 | 12 |
| 页表级数 | 4 级 (L0-L3) | 5 级 (L-1→L3) |
| 支持 | ARMv8.0+ | ARMv8.2-LPA+ |

> 52 位 VA 需要硬件支持（ARMv8.2-LPA 扩展），Pi4B/Pi5 的 Cortex-A72/A76 支持。
> 但大多数 64 位 Linux 内核仍使用 48 位 VA（足够 256TB）。

### Linux 内核地址空间布局（48 位 VA）

```
0xFFFF_FFFF_FFFF_FFFF  ┌──────────────────┐
                       │  vmalloc 区      │  虚拟连续物理不连续
                       ├──────────────────┤
                       │  vmemmap 区      │  struct page 数组
                       ├──────────────────┤
                       │  PCI I/O 区      │  PCI 设备 MMIO
                       ├──────────────────┤
                       │  linear mapping  │  __va()/__pa() 线性映射
                       ├──────────────────┤
0xFFFF_0000_0000_0000  └──────────────────┘

0x0000_FFFF_FFFF_FFFF  ┌──────────────────┐
                       │  用户空间(堆/栈/代码)│
0x0000_0000_0000_0000  └──────────────────┘
```

## HFT 关联

HFT 系统通常运行在 EL1 内核态，所有代码和数据在 TTBR1 高位空间。理解 TTBR0/TTBR1 分离有助于设计裸金属 HFT 平台——可以直接用 TTBR0 映射 MMIO 区域（避免内核页表污染），用 TTBR1 映射代码和数据。TCR 的 cache 属性设置直接影响页表 walk 的性能：如果页表本身可缓存，TLB miss 时 MMU walker 可以从 L2/L3 读页表而非 DRAM。

在裸金属 HFT 中可以只用 TTBR0（不用 TTBR1），所有地址在低地址区，简化页表管理。

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

4. **TCR_EL1 的 T0SZ=16 表示什么？如果要用 52 位 VA，T0SZ 应该设多少？**

<details>
<summary>答案</summary>

T0SZ 表示 TTBR0 地址空间中"空出的高位数量"。48 位 VA 时 T0SZ=16（64-48=16），表示 bit[63:48] 用于 canonical 检查。52 位 VA 时 T0SZ=12（64-52=12），表示 bit[63:52] 用于 canonical 检查。

T0SZ 越小 → VA 空间越大。T0SZ=0 → 全 64 位 VA（但 ARMv8 不支持完整 64 位 VA）。
</details>

## 参考与延伸

- [§14.2 四级页表](02-four-level-page-table.md) — VA 如何翻译为 PA
- [§14.6 开 MMU 流程](06-enable-mmu.md) — 设置 TTBR 的完整步骤
- [Ch17 TLB 管理](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — TTBR 切换与 TLB 维护
