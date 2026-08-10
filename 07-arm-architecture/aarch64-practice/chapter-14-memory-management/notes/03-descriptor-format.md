# §14.3 页表项（Descriptor）格式

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

页表项（descriptor）的位域格式：L0/L1/L2 用 Table 或 Block 类型，L3 只能用 Page 类型。关键字段包括 Type、属性索引、下一级地址、UXN/PXN 等。

## 核心要点

### L0/L1/L2 表项

| Bit | 字段 | 说明 |
|-----|------|------|
| [1:0] | Type | 0b11=Table（指向下一级），0b01=Block（直接映射） |
| [11:2] | Lower attributes | 访问权限、属性索引 |
| [47:12] | Next-level PA / Block PA | 下一级页表地址 / 块物理地址 |
| [63:51] | Upper attributes | XPB、PXLN 等 |

### L3 表项（必须 Page）

| Bit | 字段 | 说明 |
|-----|------|------|
| [1:0] | Type | 0b11=Page（L3 只能映射页） |
| [11:2] | Lower attributes | AP、AttrIndx、AF 等 |
| [47:12] | PA | 物理页地址 |
| [53] | PXN | 特权不可执行 |
| [54] | UXN | 用户不可执行 |
| [63] | NS | Non-Secure |

### 关键属性位

| 位 | 名称 | 说明 |
|----|------|------|
| AF | Access Flag | 0=未访问，第一次访问触发 fault |
| AttrIndx[2:0] | 属性索引 | 索引到 MAIR_ELx 的 8 个属性 |
| AP[2:1] | 访问权限 | 控制读写权限 |
| UXN | 用户不可执行 | 防止用户态执行内核代码 |
| PXN | 特权不可执行 | 防止内核执行用户代码（ret2usr 防护） |

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

## 参考与延伸

- [§14.2 四级页表](02-four-level-page-table.md) — Table vs Block 的映射大小
- [§14.4 内存属性](04-memory-attributes.md) — AttrIndx 和 MAIR 的关系
- [§14.5 访问权限](05-access-permission.md) — AP 字段详解
