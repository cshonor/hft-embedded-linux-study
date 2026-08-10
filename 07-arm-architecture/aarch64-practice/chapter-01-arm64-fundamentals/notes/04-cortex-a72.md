# 1.4 Cortex-A72 简述

> 来源：§1.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

树莓派 4B 的 Cortex-A72 处理器微架构简述，以及 Pi5（Cortex-A76/v9）的兼容性。

## 核心要点

Cortex-A72（Pi4B）：
- 乱序超标量、多核
- I-Cache / D-Cache / L2
- 内置 MMU、GIC
- 支持 TrustZone

Cortex-A76（Pi5）= ARMv9 核：
- 微架构升级（更宽的乱序窗口、更大 cache）
- **EL / 寄存器 / PSTATE 概念完全兼容** A72
- 砍掉 AArch32 执行状态

## HFT 关联

理解微架构对低延迟优化至关重要：
- 乱序执行：指令可以并行执行，减少延迟 → 但需注意数据依赖
- Cache 层次：L1 命中 ~4 cycles，L2 ~12 cycles，L3/内存 ~40-100+ cycles → HFT 数据需 L1 友好
- A76 的 wider issue width 和 deeper ROB 比 A72 有更高 IPC，适合计算密集型交易逻辑
- 内置 GIC 意味着中断控制器延迟可预测（vs 外置 GIC 的总线往返）

## 自测题

1. Cortex-A72 和 Cortex-A76 分别是什么架构？
<details><summary>答案</summary>
A72 是 ARMv8-A 架构。A76 是 ARMv9-A 架构（兼容 v8 功能）。Pi4=A72，Pi5=A76。
</details>

2. Pi5 能运行 Pi4 的 AArch64 裸机程序吗？
<details><summary>答案</summary>
能。A76 兼容 v8 的 AArch64 指令集。EL/寄存器/PSTATE 概念完全通用。只是微架构不同，性能特征不同。
</details>

## 参考与延伸

- 原书 §1.4
- [Pi5 适配](../../PI5-ADAPT.md)
- [Ch15 Cache 基础](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md)
