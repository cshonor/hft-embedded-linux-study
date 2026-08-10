# §14.8 易错点清单

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

MMU 配置的常见错误总结：开 MMU 没恒等映射、MMIO 用 Normal 属性、忘记 ISB、页表没 clean cache、AF=0、大页 L3 误用 Block、属性索引错误。每个错误都有明确的症状和修复方法。

## 核心要点

### 7 大易错点

| # | 易错点 | 后果 | 症状 | 修复 |
|---|--------|------|------|------|
| 1 | 开 MMU 没恒等映射 | PC 还是物理地址，MMU 翻译失败 → 死机 | 开 MMU 后立即死机 | 开 MMU 前建 VA=PA 映射 |
| 2 | MMIO 用了 Normal 属性 | 寄存器被缓存，读写行为未定义 | MMIO 读写不生效或随机错误 | AttrIndx 指向 Device 属性 |
| 3 | 忘记 ISB | 开 MMU 后流水线中有旧指令，行为不可预测 | 不可预测的崩溃 | SCTLR.M=1 后立即 ISB |
| 4 | 页表不在内存中 | 页表缓存在 D-cache，MMU walker 从内存读旧值 | 随机 page fault | dc civac + dsb |
| 5 | AF=0 | 第一次访问触发 Access Flag fault | 随机同步异常 | 初始化时设 AF=1 |
| 6 | L3 误用 Block | L3 不支持 Block，Type=0b01 是保留值 | 翻译 fault | L3 只能用 Page (0b11) |
| 7 | MAIR 未设或设错 | 所有内存 cache 属性错误 | 性能暴跌或行为异常 | 先设 MAIR 再开 MMU |

### 调试技巧表

| 症状 | 可能原因 | 检查方法 |
|------|----------|----------|
| 开 MMU 后立即死机 | 没恒等映射 / 忘记 ISB | 检查页表是否有 VA=PA 映射，代码是否有 ISB |
| MMIO 读写不生效 | 用了 Normal 属性（被缓存） | 检查页表项 AttrIndx → MAIR 值是否为 Device |
| 随机 page fault | 页表没 clean cache / AF=0 | 加 dc civac，检查页表项 AF 位 |
| 代码执行了旧版本 | I-Cache 缓存旧指令 | invalidate I-Cache (`ic ialluis`) |
| 性能突然暴跌 | Normal 数据误标为 Device | 检查 AttrIndx 是否指向 Device 而非 Normal |
| 写只读页触发异常 | AP 设置错误 | 检查 AP[2:1] 值 |
| 进程切换后内存错乱 | TTBR0 没更新 / TLB 没 flush | 检查进程切换是否更新 TTBR0_EL1 |

### 常见代码错误对比

```c
// 错误 1: 没有恒等映射就开 MMU
void enable_mmu_wrong(void) {
    // 假设页表只有虚拟地址映射，没有 VA=PA 恒等映射
    msr TTBR0_EL1, l0_table
    mrs x0, SCTLR_EL1
    orr x0, x0, #1
    msr SCTLR_EL1, x0
    // → 立即 page fault，PC 无法翻译
}

// 正确：先建恒等映射
void enable_mmu_correct(void) {
    // 1. 建立恒等映射（VA=PA）
    setup_identity_mapping();
    // 2. clean cache
    dc civac, l0_table
    dsb sy
    // 3. 开 MMU + ISB
    mrs x0, SCTLR_EL1
    orr x0, x0, #1
    msr SCTLR_EL1, x0
    isb
}

// 错误 2: MMIO 用 Normal 属性
void setup_mmu_wrong(void) {
    // MMIO 页表项用了 Normal-WB 属性
    l2_table[i] = (mmio_pa)
        | PTE_TYPE_BLOCK
        | (MT_NORMAL_WB << 2)   // 错！MMIO 应该用 Device
        | (1 << 10);
    // → MMIO 寄存器被缓存，读写行为未定义
}

// 正确：MMIO 用 Device 属性
void setup_mmu_correct(void) {
    l2_table[i] = (mmio_pa)
        | PTE_TYPE_BLOCK
        | (MT_DEVICE_NGNRE << 2)  // 正确：Device-nGnRE
        | (1 << 10)
        | (1 << 54)   // UXN=1
        | (1 << 53);  // PXN=1
}

// 错误 3: AF=0
void setup_pages_wrong(void) {
    l3_table[i] = (pa) | PTE_TYPE_PAGE | (MT_NORMAL_WB << 2);
    // 忘记设 AF=1
    // → 第一次访问触发 Access Flag fault
}

// 正确：AF=1
void setup_pages_correct(void) {
    l3_table[i] = (pa) | PTE_TYPE_PAGE | (MT_NORMAL_WB << 2) | (1 << 10);
    // AF=1，避免 Access Flag fault
}
```

### MMU 自检清单

```
开 MMU 前检查清单：
[ ] MAIR_EL1 已设置（Device + Normal-WB + Normal-NC）
[ ] TCR_EL1 已设置（T0SZ=16, T1SZ=16, cache 属性）
[ ] TTBR0_EL1 指向有效 L0 页表
[ ] 恒等映射已建立（当前 PC 所在页有 VA=PA 映射）
[ ] 页表已 clean cache（dc civac + dsb）
[ ] 准备好 ISB 跟在 SCTLR.M=1 后面

开 MMU 后检查清单：
[ ] ISB 已执行
[ ] 用 AT 指令验证地址翻译正确
[ ] MMIO 区域属性为 Device
[ ] 代码段属性为 Normal-WB
[ ] 所有页表项 AF=1
[ ] D-Cache 可以安全开启（先验证再开）
```

## HFT 关联

MMU 配置错误在 HFT 系统中是致命的。MMIO 用 Normal 属性是最隐蔽的 bug——寄存器读写被缓存，看起来"偶尔工作"但行为不可靠。HFT 系统上线前应该验证所有 MMIO 区域的页表属性为 Device-nGnRE。AF=0 导致的 Access Flag fault 在 HFT 中不可接受（引入异常处理开销），所有页表项初始化时必须设 AF=1。

## 自测题

1. **开 MMU 后立即死机，最可能的原因是什么？**

<details>
<summary>答案</summary>

最可能原因：**没有恒等映射**。开 MMU 前 PC 是物理地址，开 MMU 后 CPU 通过 MMU 翻译取指地址，如果没有 VA=PA 的映射，取指 page fault → 死机。第二可能：**忘记 ISB**，流水线中有 MMU 开启前的旧指令。
</details>

2. **MMIO 寄存器映射为 Normal 属性会有什么问题？**

<details>
<summary>答案</summary>

Normal 属性允许**缓存、合并、重排**。MMIO 寄存器读写有副作用（如读中断状态寄存器清除中断），被缓存后：1) 读寄存器返回 cache 中的旧值而非实际寄存器值；2) 两次写寄存器可能被合并为一次；3) 写操作可能不按顺序到达设备。行为完全不可预测。
</details>

3. **页表写在 cacheable 区域，开 MMU 前必须做什么？**

<details>
<summary>答案</summary>

必须 **clean cache**（`dc civac` + `dsb sy`）。页表项可能还在 D-cache 中没有写回内存，MMU walker 从内存读页表 → 读到旧值或零 → 翻译错误。`dc civac` 强制写回 + 作废，`dsb sy` 等待写回完成。
</details>

4. **L3 页表项能不能用 Block descriptor（Type=0b01）？为什么？**

<details>
<summary>答案</summary>

**不能**。L3 是最后一级页表，Type=0b01 在 L3 中是**保留值**（未定义行为），硬件可能将其视为无效表项或产生 fault。L3 只能用 Type=0b11（Page descriptor），映射 4KB 物理页。Block descriptor 只在 L1（1GB）和 L2（2MB）层有效。

如果误用了 Type=0b01 在 L3，MMU walker 会检测到格式错误并触发 Translation fault。
</details>

## 参考与延伸

- [§14.6 开 MMU 流程](06-enable-mmu.md) — 避免初始化错误
- [§14.4 内存属性](04-memory-attributes.md) — MMIO 属性设置
- [§14.3 页表项格式](03-descriptor-format.md) — Type 字段的有效值
- [Ch17 §17.7 易错点](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — TLB 相关错误
