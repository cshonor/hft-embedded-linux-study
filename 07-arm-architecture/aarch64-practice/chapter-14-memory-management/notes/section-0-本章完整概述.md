# Ch14 完整总结 · 页表与 MMU

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

ARMv8 MMU 的 4 级页表、地址翻译、内存属性配置。从 TTBR/TCR/SCTLR 到 BenOS 建页表开 MMU 的完整实践。

---

## 14.1 虚拟地址空间

ARMv8-A 支持 **48 位虚拟地址**（可配 52 位 LPA）：

```
0xFFFF_FFFF_FFFF_FFFF
├── 高位空间 (TTBR1)：内核空间
│   0xFFFF_0000_0000_0000 ~ 0xFFFF_FFFF_FFFF_FFFF
├── ...
├── 低位空间 (TTBR0)：用户空间
│   0x0000_0000_0000_0000 ~ 0x0000_FFFF_FFFF_FFFF
0x0000_0000_0000_0000
```

| 寄存器 | 管理范围 |
|--------|----------|
| **TTBR0_EL1** | 用户空间页表基址（低地址） |
| **TTBR1_EL1** | 内核空间页表基址（高地址） |
| **TCR_EL1** | 翻译控制（VA 宽度、ASID、 walk 等） |
| **SCTLR_EL1** | 系统控制（M=MMU 开关、C=DCache 开关） |

> VA 高位 bit[63:48] 全 1 → 走 TTBR1（内核）；全 0 → 走 TTBR0（用户）。

---

## 14.2 四级页表 ⭐

48 位 VA = 4 级页表 × 9 位索引 + 12 位页内偏移：

```
VA [47:39] → L0 索引 (512 entries)
VA [38:30] → L1 索引 (512 entries)
VA [29:21] → L2 索引 (512 entries)
VA [20:12] → L3 索引 (512 entries)
VA [11:0]  → 页内偏移 (4KB)
```

| 级别 | 表项大小 | 每表项映射 | 表大小 |
|------|----------|-----------|--------|
| L0 | 8 字节 | 512GB | 4KB |
| L1 | 8 字节 | 1GB | 4KB |
| L2 | 8 字节 | 2MB | 4KB |
| L3 | 8 字节 | 4KB | 4KB |

> 每级 512 表项 × 8 字节 = 4KB，恰好一页。  
> 可用 **Block descriptor** 在 L1(1GB) 或 L2(2MB) 层直接映射大块，省一层查找。

---

## 14.3 页表项（Descriptor）格式

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

---

## 14.4 内存属性 ⭐

### AttrIndx → MAIR_ELx 映射

页表项的 AttrIndx 字段（3位）索引到 **MAIR_ELx** 的 8 个属性字段：

```c
// 典型 MAIR_EL1 设置
#define MT_NORMAL_NOCACHE  0x44  // Normal, Non-cacheable
#define MT_NORMAL_WB       0xFF  // Normal, Write-Back (Cacheable)
#define MT_DEVICE_NGNRE    0x00  // Device-nGnRE (设备内存)

MAIR_EL1 = (MT_NORMAL_NOCACHE << 0)  |  // AttrIndx=0
           (MT_NORMAL_WB << 8)      |  // AttrIndx=1
           (MT_DEVICE_NGNRE << 16);    // AttrIndx=2
```

### Normal vs Device

| 属性 | Normal | Device |
|------|--------|--------|
| 缓存 | 可缓存 | **不可缓存** |
| 乱序 | 可乱序/可合并 | **严格保序、不合并** |
| 预取 | 可预取 | **不可预取** |
| 用途 | 代码、数据 | MMIO 寄存器 |

> **MMIO 必须用 Device 属性**：外设寄存器读写不能被缓存/合并/重排。  
> 常见 Device 类型：nGnRnE（最强保序）、nGnRE（允许重试）、nGRE、GRE。

---

## 14.5 访问权限（AP）

| AP[2:1] | EL0 | EL1+ | 说明 |
|---------|-----|------|------|
| 00 | R/W | R/W | 全部可读写 |
| 01 | None | R/W | 仅内核可读写 |
| 10 | R/O | R/O | 全部只读 |
| 11 | R/O | R/W | 内核读写，用户只读 |

> **UXN/PXN**：控制可执行性。内核页通常设 UXN=1（用户不可执行）+ PXN=0（内核可执行）。  
> 用户页通常设 PXN=1（内核不可执行，防止 ret2usr）。

---

## 14.6 开 MMU 流程

```asm
setup_mmu:
    // 1. 设置 MAIR_EL1
    ldr x0, =mair_value
    msr MAIR_EL1, x0

    // 2. 设置 TCR_EL1（VA 宽度、walk 等）
    ldr x0, =tcr_value
    msr TCR_EL1, x0

    // 3. 设置 TTBR0_EL1（页表基址）
    adrp x0, l0_table
    msr TTBR0_EL1, x0

    // 4. 刷新 cache（如果页表写在 cacheable 区域）
    dc  civac, x0          // clean+invalidate
    dsb sy

    // 5. 开 MMU（SCTLR.M=1）
    mrs x0, SCTLR_EL1
    orr x0, x0, #1         // M bit
    msr SCTLR_EL1, x0
    isb                     // 必须跟 ISB

    // 6. 此时 PC 还是物理地址
    //    必须有恒等映射（VA=PA）保证取指继续
```

### 恒等映射（Identity Mapping）

```
开 MMU 前：PC = 0x60000000（物理地址）
开 MMU 后：MMU 翻译 0x60000000 → 需要页表映射 VA=0x60000000 → PA=0x60000000
```

> 如果没有恒等映射，开 MMU 后第一条指令取指就 page fault。  
> Linux `head.S` 也是先建恒等映射再开 MMU。

---

## 14.7 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 14-1 | 建立恒等映射 | QEMU |
| 14-2 | 为什么 MMU 无法运行（排错） | QEMU |
| 14-3 | 页表转储功能 | QEMU |
| 14-4 | 修改页面属性导致死机 | QEMU |
| 14-5 | 汇编建恒等映射+开 MMU | QEMU |
| 14-6 | LDXR/STXR 在 MMU 下行为 | QEMU |
| 14-7 | AT 指令（地址翻译） | QEMU |

---

## 14.8 易错点清单

1. **开 MMU 没恒等映射** → PC 还是物理地址，MMU 翻译失败 → 死机。
2. **MMIO 用了 Normal 属性** → 寄存器被缓存，读写行为未定义。
3. **忘记 ISB** → 开 MMU 后流水线中有旧指令，行为不可预测。
4. **页表不在内存中** → 页表被缓存在 D-cache，但 MMU walker 可能从内存读旧值 → 需 clean cache。
5. **AF（Access Flag）= 0** → 第一次访问触发 Access Flag fault。

---

## 书中思考题（自测）

1. ARMv8 的 48 位 VA 分几级页表？每级多少表项？
2. TTBR0 和 TTBR1 分别管什么地址空间？如何选择？
3. Normal 和 Device 内存属性的区别？MMIO 寄存器应该用哪种？
4. 开 MMU 前为什么要先建恒等映射？
5. SCTLR 的 M 位是什么？开 MMU 后必须跟什么指令？

**参考答案：**

1. **4 级**（L0-L3），每级 **512 表项**（9 位索引），每级表 4KB。  
2. TTBR0=**用户空间**（低地址）；TTBR1=**内核空间**（高地址）。VA bit[63:48] 全 0→TTBR0，全 1→TTBR1。  
3. Normal 可缓存/可乱序；Device **不可缓存/严格保序**。MMIO 用 **Device**。  
4. 开 MMU 后 PC 还是物理地址，需要 VA=PA 的映射让取指继续。  
5. M=**MMU 使能位**；开 MMU 后必须跟 **ISB**（冲刷流水线）。

---

上一章 [Ch13 GIC-V2](../../chapter-13-gic-v2/) · 下一章 [Ch15 Cache基础](../../chapter-15-cache-basics/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
