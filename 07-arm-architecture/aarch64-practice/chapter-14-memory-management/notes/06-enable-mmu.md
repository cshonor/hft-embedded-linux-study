# §14.6 开 MMU 流程

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

开启 MMU 的完整步骤：设 MAIR → 设 TCR → 设 TTBR → clean cache → 设 SCTLR.M=1 → ISB。开 MMU 前必须有恒等映射（VA=PA）保证取指继续。

## 核心要点

### 开 MMU 代码

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

### 步骤总结

| 步骤 | 寄存器 | 作用 | 必须性 |
|------|--------|------|--------|
| 1 | MAIR_EL1 | 内存属性表 | 必须 |
| 2 | TCR_EL1 | VA 宽度/cache 属性 | 必须 |
| 3 | TTBR0/1_EL1 | 页表基址 | 必须 |
| 4 | dc civac | 清页表 cache | 如果页表在 cacheable 区域 |
| 5 | SCTLR_EL1.M=1 | 开 MMU | 必须 |
| 6 | ISB | 冲刷流水线 | **必须** |

## HFT 关联

开 MMU 是裸金属 HFT 系统启动的关键步骤。恒等映射确保从物理地址到虚拟地址的平滑切换。在 HFT 中，MMU 开启后应立即配置大页映射（2MB Block）减少 TLB miss。注意 SCTLR 的 C 位（D-Cache 使能）可以和 M 位（MMU 使能）一起开，但建议先开 MMU 确认页表正确，再开 D-Cache——如果页表属性设置错误，开 D-Cache 可能导致 cache 一致性问题。

## 自测题

1. **开 MMU 后为什么要跟 ISB 指令？不跟会怎样？**

<details>
<summary>答案</summary>

ISB 冲刷流水线，确保后续指令在 MMU 开启后的新状态下**重新取指**。不跟 ISB → 流水线中可能有 MMU 开启前取的指令（使用物理地址），执行后行为不可预测。MMU 开启是系统状态变更，必须用 ISB 强制流水线同步。
</details>

2. **开 MMU 前为什么要建恒等映射？**

<details>
<summary>答案</summary>

开 MMU 前 PC 是物理地址（如 0x60000000）。开 MMU 后，CPU 取指需要通过 MMU 翻译 VA→PA。如果页表中没有 VA=0x60000000 → PA=0x60000000 的映射，取指会 page fault → 死机。恒等映射保证开 MMU 后当前 PC 地址仍能正确翻译，取指继续。等后续跳转到真正的虚拟地址后，恒等映射可以删除。
</details>

3. **为什么步骤 4 要 clean cache？不 clean 会怎样？**

<details>
<summary>答案</summary>

页表写在 cacheable 区域时，新写的页表项可能还在 D-cache 中，没有写回内存。MMU walker 从内存读页表 → 读到旧值 → 翻译错误。`dc civac` 强制写回 + 作废，确保内存中的页表是最新的。如果不 clean，MMU 可能用旧页表翻译，导致 page fault 或访问错误地址。
</details>

## 参考与延伸

- [§14.1 虚拟地址空间](01-va-space.md) — TTBR/TCR/SCTLR 的作用
- [§14.4 内存属性](04-memory-attributes.md) — MAIR 设置
- [Ch17 §17.3 TLB 刷新指令](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — 开 MMU 后的 TLB 维护
