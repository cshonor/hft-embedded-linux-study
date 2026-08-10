# §14.3 页表项（Descriptor）格式

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

页表项（descriptor）的位域格式：L0/L1/L2 用 Table 或 Block 类型，L3 只能用 Page 类型。关键字段包括 Type、属性索引、下一级地址、UXN/PXN 等。理解页表项格式是手动填充页表的基础。

## 核心要点

### L0/L1/L2 表项格式（64 位）

```
 63  62     55  54  53  52     48  47               12  11    8  7  6  5  4  3  2  1  0
┌────┬────────┬────┬────┬────────┬─────────────────────┬──────┬──┬──┬──┬──┬──┬─────┐
│ NS │ XPB    │UXN │PXN │ 保留   │  Next-level PA /    │AttrIdx│  │  │  │  │  │Type │
│    │        │    │    │        │  Block PA [47:12]   │[4:2] │nG│AF│SH0│AP│  │     │
└────┴────────┴────┴────┴────────┴─────────────────────┴──────┴──┴──┴──┴──┴──┴─────┘
```

| Bit | 字段 | 说明 |
|-----|------|------|
| [1:0] | Type | 0b11=Table（指向下一级），0b01=Block（直接映射） |
| [4:2] | AttrIndx | 属性索引，指向 MAIR_ELx 的 8 个属性 |
| [5] | NS | Non-Secure 位 |
| [6] | AP[1] | 访问权限 bit1（0=特权可写，1=用户可写） |
| [7] | AP[2] | 访问权限 bit2（0=读写，1=只读） |
| [8] | SH | Shareability（0b00=None, 0b10=Outer, 0b11=Inner） |
| [10] | AF | Access Flag（0=未访问，第一次访问触发 fault） |
| [11] | nG | non-Global（0=全局，1=进程私有，需 ASID） |
| [47:12] | PA | 下一级页表地址 / Block 物理地址（36 位，4KB 对齐） |
| [53] | PXN | 特权不可执行 |
| [54] | UXN | 用户不可执行 |
| [63] | NS | Non-Secure（安全扩展使用） |

### L3 表项格式（必须 Page）

| Bit | 字段 | 说明 |
|-----|------|------|
| [1:0] | Type | 0b11=Page（L3 只能映射页） |
| [4:2] | AttrIndx | 属性索引 |
| [6:7] | AP[2:1] | 访问权限 |
| [8] | SH | Shareability |
| [10] | AF | Access Flag |
| [11] | nG | non-Global |
| [47:12] | PA | 物理页地址（4KB 对齐） |
| [53] | PXN | 特权不可执行 |
| [54] | UXN | 用户不可执行 |

### 关键属性位详解

| 位 | 名称 | 值 | 说明 |
|----|------|-----|------|
| AF | Access Flag | 0 | 第一次访问触发 fault（懒加载） |
| | | 1 | 已访问，正常使用 |
| AttrIndx | 属性索引 | 0-7 | 索引到 MAIR_ELx 的 8 个属性字段 |
| AP[2:1] | 访问权限 | 00 | EL0/EL1 都可读写 |
| | | 01 | 仅 EL1 可读写 |
| | | 10 | EL0/EL1 都只读 |
| | | 11 | EL1 读写，EL0 只读 |
| UXN | 用户不可执行 | 0 | 用户可执行 |
| | | 1 | 用户不可执行 |
| PXN | 特权不可执行 | 0 | 内核可执行 |
| | | 1 | 内核不可执行 |
| nG | non-Global | 0 | 全局映射（所有进程共享） |
| | | 1 | 进程私有（需 ASID 标记） |
| SH | Shareability | 00 | Non-shareable |
| | | 10 | Outer Shareable |
| | | 11 | Inner Shareable |

### 典型页表项配置

```c
// 页表项构造宏
#define PTE_TYPE_TABLE   (0b11)
#define PTE_TYPE_BLOCK   (0b01)
#define PTE_TYPE_PAGE    (0b11)

// 属性索引（对应 MAIR_ELx 中的定义）
#define MT_NORMAL_NC     0   // AttrIndx=0: Normal Non-cacheable
#define MT_NORMAL_WB     1   // AttrIndx=1: Normal Write-Back
#define MT_DEVICE_NGNRE  2   // AttrIndx=2: Device-nGnRE

// 构造 L3 Page 表项（内核代码页）
#define PTE_KERNEL_CODE(pa) \
    ((pa & 0x0000FFFFFFFFF000ULL) |  /* PA[47:12] */  \
     PTE_TYPE_PAGE |                  /* Type=Page */   \
     (MT_NORMAL_WB << 2) |            /* AttrIndx=1 */  \
     (0b11 << 8) |                    /* SH=Inner */    \
     (1 << 10) |                      /* AF=1 */        \
     (0 << 11) |                      /* nG=0 (全局) */ \
     (0 << 54))                       /* UXN=0 (可执行) */

// 构造 L2 Block 表项（2MB MMIO 区域）
#define PTE_MMIO_BLOCK(pa) \
    ((pa & 0x0000FFFFFFFFE000ULL) |  /* PA[47:21] */   \
     PTE_TYPE_BLOCK |                 /* Type=Block */   \
     (MT_DEVICE_NGNRE << 2) |         /* AttrIndx=2 */   \
     (0b00 << 8) |                    /* SH=None */      \
     (1 << 10) |                      /* AF=1 */         \
     (1 << 54) |                      /* UXN=1 */        \
     (1 << 53))                       /* PXN=1 */
```

### 典型页面配置表

| 页面类型 | AttrIndx | AP | AF | nG | UXN | PXN | SH |
|----------|----------|-----|-----|-----|-----|-----|-----|
| 内核代码 | Normal-WB(1) | 10(RO) | 1 | 0 | 0 | 0 | 11 |
| 内核数据 | Normal-WB(1) | 01(KR/W) | 1 | 0 | 1 | 1 | 11 |
| 用户代码 | Normal-WB(1) | 00(RW) | 1 | 1 | 0 | 1 | 11 |
| 用户数据 | Normal-WB(1) | 00(RW) | 1 | 1 | 1 | 1 | 11 |
| MMIO | Device(2) | 01(KR/W) | 1 | 0 | 1 | 1 | 00 |
| 只读数据 | Normal-WB(1) | 10(RO) | 1 | 0 | 1 | 1 | 11 |

## HFT 关联

页表项的 AttrIndx 字段决定了内存的 cache 属性——这对 HFT 至关重要。MMIO 寄存器必须映射为 Device 属性（AttrIndx 指向 MAIR 中的 Device 类型），否则缓存会导致寄存器读写行为未定义。交易数据应映射为 Normal-WB（Write-Back cacheable）。AF 位要设为 1，否则第一次访问会触发 Access Flag fault，引入不可预期的异常开销。

## 自测题

1. **L3 表项的 Type 字段只能是 0b11（Page）吗？为什么？**

<details>
<summary>答案</summary>

**是的**，L3 只能是 Page（0b11）。因为 L3 是最后一级页表，必须映射到 4KB 物理页，不能指向下一级页表（没有下一级）。L1/L2 才可以用 Block（0b01）直接映射大块。
</details>

2. **UXN 和 PXN 分别防止什么？内核页应该怎么设？**

<details>
<summary>答案</summary>

- **UXN**（User eXecute Never）：用户态不能执行此页
- **PXN**（Privileged eXecute Never）：内核态不能执行此页

内核页：UXN=**1**（用户不可执行），PXN=**0**（内核可执行）。用户页：UXN=**0**（用户可执行），PXN=**1**（内核不可执行，防止 ret2usr 攻击）。
</details>

3. **AF 位（Access Flag）为 0 会怎样？**

<details>
<summary>答案</summary>

AF=0 时，第一次访问该页会触发 **Access Flag fault**（同步异常）。内核的缺页处理程序将 AF 设为 1 并返回。这是一种"懒加载"机制——内核可以推迟分配实际物理页直到第一次访问。在裸金属 HFT 系统中，应直接设 AF=1 避免意外的异常开销。
</details>

4. **nG 位为 1 和为 0 有什么区别？什么时候用 1？**

<details>
<summary>答案</summary>

- nG=0：**全局映射**，所有进程共享此页表项，TLB 中不需要 ASID 标记
- nG=1：**进程私有映射**，TLB 中带 ASID 标记，进程切换时不需要 flush

内核页通常 nG=0（所有进程共享）。用户页通常 nG=1（进程私有），配合 ASID 实现 TLB 跨进程切换不 flush。

在裸金属 HFT 系统中（没有进程概念），所有页都可以设 nG=0。
</details>

## 参考与延伸

- [§14.2 四级页表](02-four-level-page-table.md) — Table vs Block 的映射大小
- [§14.4 内存属性](04-memory-attributes.md) — AttrIndx 和 MAIR 的关系
- [§14.5 访问权限](05-access-permission.md) — AP 字段详解
