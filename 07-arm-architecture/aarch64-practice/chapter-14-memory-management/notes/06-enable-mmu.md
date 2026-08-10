# §14.6 开 MMU 流程

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

开启 MMU 的完整步骤：设 MAIR → 设 TCR → 设 TTBR → clean cache → 设 SCTLR.M=1 → ISB。开 MMU 前必须有恒等映射（VA=PA）保证取指继续。这是裸金属系统从物理地址切换到虚拟地址的关键步骤。

## 核心要点

### 开 MMU 完整代码

```asm
setup_mmu:
    // 0. 确保已有恒等映射页表（VA=PA）
    //    在链接脚本中预留页表区域
    adrp x0, l0_table         // L0 页表基址

    // 1. 设置 MAIR_EL1（内存属性表）
    ldr x1, =mair_value
    msr MAIR_EL1, x1
    // mair_value = (0x44 << 0) | (0xFF << 8) | (0x00 << 16)
    //              Normal-NC  Normal-WB  Device-nGnRnE

    // 2. 设置 TCR_EL1（VA 宽度、walk cache 属性）
    ldr x1, =tcr_value
    msr TCR_EL1, x1
    // tcr_value: T0SZ=16 | T1SZ=16 | IRGN0=WB | ORGN0=WB | SH0=Inner

    // 3. 设置 TTBR0_EL1（页表基址）
    //    恒等映射使用 TTBR0（低地址空间）
    adrp x0, l0_table
    msr TTBR0_EL1, x0

    // 4. 刷新 cache（页表写在 cacheable 区域时）
    //    确保页表已写回内存，MMU walker 能读到最新值
    adrp x0, l0_table
    dc  civac, x0              // clean + invalidate
    dsb sy                     // 等待写回完成

    // 5. 开 MMU（SCTLR_EL1.M=1）
    mrs x0, SCTLR_EL1
    orr x0, x0, #1             // M bit = 1 (开 MMU)
    msr SCTLR_EL1, x0
    isb                         // 必须跟 ISB！冲刷流水线

    // 6. 此时 PC 还是物理地址
    //    恒等映射保证取指继续
    //    后续可以跳转到虚拟地址

    // 7. （可选）开 D-Cache
    mrs x0, SCTLR_EL1
    orr x0, x0, #4             // C bit = 1 (开 D-Cache)
    msr SCTLR_EL1, x0
    isb
```

### 恒等映射（Identity Mapping）

```
开 MMU 前：PC = 0x60000000（物理地址）
           CPU 直接用物理地址取指

开 MMU 后：CPU 取指需要通过 MMU 翻译 VA → PA
           页表中必须有 VA=0x60000000 → PA=0x60000000
           否则第一条指令就 page fault → 死机

恒等映射：VA = PA（同一地址值映射到自己）
           开 MMU 后 PC 所在的页必须有恒等映射
```

```
链接脚本中的恒等映射区域：
  . = 0x60000000;           // 代码加载在物理地址 0x60000000
  .text : { *(.text) }      // 同时 VA=0x60000000（恒等映射）

  页表区域：
  . = ALIGN(4096);
  l0_table : { *(.l0_table) }  // 4KB L0 页表
```

### SCTLR_EL1 关键位

| 位 | 名称 | 说明 |
|----|------|------|
| M | MMU 使能 | 0=关 MMU（物理地址直接使用），1=开 MMU |
| C | D-Cache 使能 | 0=关 D-Cache，1=开 D-Cache（必须 M=1 后才能开） |
| I | I-Cache 使能 | 0=关 I-Cache，1=开 I-Cache（可独立于 M 开启） |
| A | Alignment Check | 0=不对齐不检查，1=严格对齐检查 |
| SA | SP Alignment Check | 0=不检查 SP 对齐 |

### 步骤总结

| 步骤 | 寄存器 | 作用 | 必须性 | 忘记的后果 |
|------|--------|------|--------|----------|
| 1 | MAIR_EL1 | 内存属性表 | 必须 | 所有内存属性错误 |
| 2 | TCR_EL1 | VA 宽度/cache 属性 | 必须 | 页表 walk 异常 |
| 3 | TTBR0/1_EL1 | 页表基址 | 必须 | MMU 找不到页表 |
| 4 | dc civac | 清页表 cache | 如果页表在 cacheable 区域 | MMU 读旧页表 |
| 5 | SCTLR_EL1.M=1 | 开 MMU | 必须 | — |
| 6 | ISB | 冲刷流水线 | **必须** | 流水线行为不可预测 |

### 开 MMU 前后的状态变化

```
开 MMU 前：
  - 所有地址是物理地址
  - CPU 直接访问物理内存
  - 没有 cache 属性控制（全部不可缓存）
  - 没有访问权限控制
  - 没有 VA/PA 转换

开 MMU 后：
  - 所有地址通过 MMU 翻译
  - CPU 使用虚拟地址
  - 页表项的 AttrIndx 控制 cache 属性
  - AP 控制访问权限
  - TLB 缓存翻译结果
```

### Linux head.S 中的 MMU 开启流程

Linux 内核启动时的 MMU 开启流程：
1. 先在汇编中建立恒等映射 + 内核映射
2. 设 MAIR_EL1, TCR_EL1, TTBR_EL1
3. clean cache
4. 设 SCTLR.M=1 + ISB
5. 跳转到虚拟地址（_primary_entry 的虚拟地址）
6. 后续可以删除恒等映射

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

4. **为什么建议先开 MMU 再开 D-Cache？**

<details>
<summary>答案</summary>

先开 MMU 确认页表正确，再开 D-Cache。如果先开 D-Cache：1) MMU 还没开，cache 属性由默认配置控制，可能不符合预期；2) 如果页表属性设置错误（如 MMIO 被标记为 cacheable），开 D-Cache 会导致 MMIO 读写被缓存 → 行为未定义。先开 MMU → 验证页表正确 → 再开 D-Cache → 安全。

I-Cache 可以独立开启（SCTLR.I），因为它不受 MMU 控制，可以在开 MMU 前就开。
</details>

## 参考与延伸

- [§14.1 虚拟地址空间](01-va-space.md) — TTBR/TCR/SCTLR 的作用
- [§14.4 内存属性](04-memory-attributes.md) — MAIR 设置
- [Ch17 §17.3 TLB 刷新指令](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — 开 MMU 后的 TLB 维护
