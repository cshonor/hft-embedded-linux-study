# §14.4 内存属性

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

页表项的 AttrIndx 字段索引到 MAIR_ELx 的 8 个属性字段，决定内存的 cache 行为。Normal 属性可缓存，Device 属性不可缓存且严格保序。MMIO 必须用 Device 属性。

## 核心要点

### AttrIndx → MAIR_ELx 映射

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

### Device 类型详解

| 类型 | 全称 | 保序级别 |
|------|------|----------|
| nGnRnE | non-Gathering, non-Reordering, no Early write | 最强（严格顺序、不合并、不等确认） |
| nGnRE | non-Gathering, non-Reordering, Early write | 允许提前确认 |
| nGRE | non-Gathering, Reordering, Early write | 允许重排 |
| GRE | Gathering, Reordering, Early write | 最弱（允许合并/重排） |

## HFT 关联

内存属性是 HFT 延迟确定性的基础。MMIO 寄存器（如网卡门铃寄存器）必须映射为 Device-nGnRE，确保写操作严格保序、不被合并。交易数据用 Normal-WB 充分利用 cache。DMA buffer 如果是 coherent 的可以用 Normal-NC（Non-Cacheable）避免手动 flush。错误的属性设置会导致隐蔽的 bug：MMIO 被缓存导致寄存器读写不生效，或 Normal 数据被标记为 Device 导致性能暴跌。

## 自测题

1. **MMIO 寄存器应该用 Normal 还是 Device 属性？为什么？**

<details>
<summary>答案</summary>

必须用 **Device** 属性。因为 MMIO 寄存器的读写有副作用（如读中断状态寄存器会清除中断标志），不能被缓存（需要直接到达设备）、不能被合并（两次写寄存器必须分别执行）、不能被重排（顺序敏感）。Normal 属性允许缓存/合并/重排，会导致 MMIO 行为未定义。
</details>

2. **AttrIndx=2 在上面的 MAIR 设置中对应什么属性？**

<details>
<summary>答案</summary>

AttrIndx=2 对应 MAIR_EL1 的第 2 个属性字段（bit[23:16]）= **0x00** = **Device-nGnRE**。页表项的 AttrIndx 字段是 3 位索引，指向 MAIR_EL1 的 8 个 8 位属性字段之一。
</details>

3. **Normal-WB（0xFF）和 Normal-NC（0x44）有什么区别？HFT 中分别用于什么？**

<details>
<summary>答案</summary>

- **Normal-WB（0xFF）**：Write-Back Cacheable，数据缓存在 CPU cache 中，写操作只更新 cache，脏数据延迟写回。用于代码和热数据（如订单簿）。
- **Normal-NC（0x44）**：Non-Cacheable，数据不缓存，每次访问直接到内存。用于 DMA buffer（非 coherent 场景）或需要立即对其他观察者可见的数据。

HFT 中交易热数据用 WB（利用 cache 低延迟），DMA buffer 用 NC（避免 cache 一致性问题）。
</details>

## 参考与延伸

- [§14.3 页表项格式](03-descriptor-format.md) — AttrIndx 在页表项中的位置
- [§14.6 开 MMU 流程](06-enable-mmu.md) — 设置 MAIR_EL1 的步骤
- [Ch15 §15.5 DMA 与 Cache](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md) — DMA 场景的 cache 属性
