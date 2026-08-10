# 7.6 Linux 启动汇编分析（大作业）

> 来源：§7.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

分析 Linux 内核启动汇编代码 head.S，综合应用前 6 章的指令知识。

## 核心要点

Linux ARM64 启动流程（`arch/arm64/kernel/head.S`）：
1. 检查当前 EL，从 EL3/EL2 降到 EL1
2. 设置初始栈（SP）
3. 建立恒等映射（Identity Mapping）
4. 开启 MMU
5. 跳转到虚拟地址执行（start_kernel）

关键指令综合应用：
```asm
// EL 检查
mrs x0, CurrentEL
lsr x0, x0, #2
cmp x0, #3
b.eq 1f   // 从 EL3 降级

// ADRP 获取符号地址
adrp x0, init_pg_dir

// MSR 设置系统寄存器
msr TTBR0_EL1, x0
msr SCTLR_EL1, x1   // 开 MMU
isb                   // 必须跟 ISB
```

## HFT 关联

理解内核启动对 HFT 的意义：
- 内核启动的 MMU 配置决定了后续所有内存访问的属性 → 影响 cache 行为
- 恒等映射和虚拟地址切换是理解内核地址空间的基础
- HFT 优化需要理解内核的页表布局（哪些是 Normal/Device）
- 启动代码中的屏障使用是正确内存序的最佳教材

## 自测题

1. Linux 启动时为什么要先建恒等映射再开 MMU？
<detail><summary>答案</summary>
开 MMU 前 PC 是物理地址。开 MMU 后，取指需要经过 MMU 翻译（VA→PA）。如果没有恒等映射（VA=PA 的映射），MMU 翻译当前 PC 地址会 page fault，CPU 挂死。恒等映射保证开 MMU 后取指继续正常工作，直到跳转到真正的虚拟地址。
</details>

2. 开 MMU 后为什么必须跟 ISB？
<detail><summary>答案</summary>
开 MMU（MSR SCTLR_EL1）改变了地址翻译行为。但流水线中可能有之前取的指令（基于旧 MMU 状态）。ISB 冲刷流水线，强制重新取指，确保后续指令在新的 MMU 状态下执行。不跟 ISB 可能导致取到旧地址翻译的指令，行为不可预测。
</details>

3. 启动代码中从 EL3 降到 EL1 的过程是什么？
<detail><summary>答案</summary>
1. 在 EL3 配置 SCR_EL3（允许 HVC，设为非安全）
2. 设置 ELR_EL3 = EL2 入口地址，SPSR_EL3 = 目标 PSTATE
3. ERET → 降到 EL2
4. 在 EL2 配置 HCR_EL2（设为非虚拟化模式）
5. 设置 ELR_EL2 = EL1 入口地址
6. ERET → 降到 EL1
7. 在 EL1 设置 VBAR/SP，开始正常执行
</details>

## 参考与延伸

- 原书 §7.6
- [6.1 ADR/ADRP](../../chapter-06-a64-other-instructions/notes/section-0-本章完整概述.md)
- [Ch14 MMU 开启流程](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
