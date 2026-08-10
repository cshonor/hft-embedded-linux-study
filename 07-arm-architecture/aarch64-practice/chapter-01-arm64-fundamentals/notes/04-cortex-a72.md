# 1.4 Cortex-A72 简述

> 来源：§1.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

树莓派 4B 的 Cortex-A72 处理器微架构简述，以及 Pi5（Cortex-A76/v9）的兼容性。理解微架构对低延迟优化的影响。

## Cortex-A72 微架构

Cortex-A72 是 ARM 2015 年发布的高性能核，用于树莓派 4B（BCM2711 SoC）。

### 流水线

| 特性 | Cortex-A72 | Cortex-A76 (Pi5) |
|------|-----------|-----------------|
| 架构 | ARMv8-A | ARMv8.2-A |
| 流水线 | 15 级顺序前端 + 乱序执行 | 乱序更深 |
| Issue width | 3-wide | 4-wide |
| ROB（重排序缓冲） | ~128 | ~200+ |
| 物理寄存器 | ~160 GP | ~256 GP |
| L1 I-Cache | 48KB | 64KB |
| L1 D-Cache | 32KB | 64KB |
| L2 Cache | 512KB-2MB | 256KB-1MB |
| 频率 | 1.5 GHz (Pi4) | 2.4 GHz (Pi5) |

### Cache 层次

```
┌─────────────────────────────────────────────┐
│  CPU Core                                    │
│  ┌─────────┐  ┌─────────┐                    │
│  │ L1 I$   │  │ L1 D$   │  ← ~4 cycles      │
│  │ 48KB    │  │ 32KB    │                    │
│  └────┬────┘  └────┬────┘                    │
│       └──────┬─────┘                         │
│         ┌────┴────┐                          │
│         │  L2 $   │  ← ~12 cycles            │
│         │ 1MB     │  (私有或共享)              │
│         └────┬────┘                          │
│              │                               │
├──────────────┼───────────────────────────────┤
│         ┌────┴────┐                          │
│         │  L3 $   │  ← ~40 cycles            │
│         │ (可选)   │  (共享)                   │
│         └────┬────┘                          │
│              │                               │
│         ┌────┴────┐                          │
│         │  DRAM   │  ← ~100-200 cycles       │
│         └─────────┘                          │
└─────────────────────────────────────────────┘
```

### 访问延迟

| 层级 | 延迟 (cycles) | 延迟 (ns @1.5GHz) |
|------|-------------|-------------------|
| L1 I-Cache | ~4 | ~2.7 |
| L1 D-Cache | ~4 | ~2.7 |
| L2 Cache | ~12 | ~8 |
| L3/内存 | ~40-200+ | ~27-130+ |

## Cortex-A72 内部功能块

| 功能 | 说明 |
|------|------|
| 分支预测器 | 2 级自适应，BTB+BHT |
| 指令解码 | 3-wide 解码 |
| 重命名 | 寄存器重命名消除假依赖 |
| 调度 | 乱序调度，8 个执行端口 |
| 加载/存储 | 2 个 AGU（地址生成单元） |
| 整数 ALU | 2 个 ALU |
| NEON/FPU | 128-bit NEON 单元 |
| MMU | 48 位 VA，4KB/64KB/2MB 页 |

## 树莓派 4B (BCM2711)

| 规格 | 参数 |
|------|------|
| CPU | 4x Cortex-A72 @ 1.5 GHz |
| L2 | 1MB 共享 |
| 内存 | 1/2/4/8 GB LPDDR4 |
| GPU | VideoCore VI |
| GIC | GIC-400 |

### Pi4 裸机开发注意事项

- **启动方式**：GPU 先启动 → 加载 start4.elf → 加载 kernel8.img → CPU 从 EL2 或 EL3 开始
- **UART**：PL011 (GPIO 14/15) 或 mini UART
- **中断控制器**：GIC-400（或 BCM2836 局部中断）
- **物理内存**：从 0x0 开始（或 0x3F000000 取决于配置）

## Cortex-A76 (Pi5/BCM2712)

| 特性 | A72 (Pi4) | A76 (Pi5) |
|------|-----------|-----------|
| 架构 | ARMv8.0-A | ARMv8.2-A |
| Issue width | 3 | 4 |
| 频率 | 1.5 GHz | 2.4 GHz |
| L1 I$ | 48KB | 64KB |
| L1 D$ | 32KB | 64KB |
| L2 | 1MB | 512KB/1MB |
| AArch32 | ✓ 支持 | ✗ 砍掉 |
| IPC | 基准 | ~+25% |

**关键**：EL / 寄存器 / PSTATE 概念完全兼容，Pi4 的 AArch64 代码可在 Pi5 直接运行。

## 微架构对 HFT 的影响

| 微架构特性 | 对延迟的影响 | HFT 策略 |
|-----------|------------|---------|
| 乱序执行 | 指令并行 → 减少延迟 | 避免长依赖链 |
| 分支预测 | 预测错误 → 流水线冲刷 ~15 cycles | 减少分支，用无分支代码 |
| L1 Cache | 命中 ~4 cycles vs miss ~100+ | 数据结构 <32KB |
| NEON | 128-bit 向量并行 | 期权定价矩阵向量化 |
| Store Buffer | 写不阻塞后续读 | 减少写后读依赖 |

## HFT 关联

理解微架构对低延迟优化至关重要：
- **乱序执行**：指令可以并行执行，减少延迟 → 但需注意数据依赖
- **Cache 层次**：L1 命中 ~4 cycles，L2 ~12 cycles，L3/内存 ~40-100+ cycles → HFT 数据需 L1 友好
- **A76 的 wider issue width 和 deeper ROB** 比 A72 有更高 IPC，适合计算密集型交易逻辑
- **内置 GIC** 意味着中断控制器延迟可预测（vs 外置 GIC 的总线往返）
- **分支预测器** → 热循环用倒计数（SUBS+B.NE）减少预测错误

## 自测题

1. Cortex-A72 和 Cortex-A76 分别是什么架构？
<details><summary>答案</summary>
A72 是 ARMv8.0-A 架构。A76 是 ARMv8.2-A 架构（兼容 v8.0 功能，额外支持 LSE 原子指令等）。Pi4=A72，Pi5=A76。两者都支持 AArch64 执行状态，但 A76 砍掉了 AArch32。
</details>

2. Pi5 能运行 Pi4 的 AArch64 裸机程序吗？
<details><summary>答案</summary>
能。A76 兼容 v8 的 AArch64 指令集。EL/寄存器/PSTATE 概念完全通用。只是微架构不同（更宽的 issue、更深 ROB、更大 cache），性能特征不同。代码不需要修改，但启动地址和外设地址可能不同（BCM2711 vs BCM2712）。
</details>

3. Cortex-A72 的 L1 D-Cache 多大？对 HFT 数据结构设计有什么影响？
<details><summary>答案</summary>
L1 D-Cache = 32KB。HFT 热数据结构（如订单簿）应尽量控制在 32KB 内，保证全部 L1 命中（~4 cycles）。超出则 L2 miss（~100+ cycles），延迟差 25 倍。设计时应避免不必要字段、用紧凑数据布局（SoA vs AoS）。
</details>

4. 乱序执行对 HFT 代码编写有什么影响？
<details><summary>答案</summary>
（1）避免长依赖链（如连续 ADD）：拆成独立计算让乱序并行（2）避免 LOAD 后立即 USE（load-use latency）：插入无关指令掩盖延迟（3）分支预测错误代价高（~15 cycles 冲刷）：减少不可预测分支，用无分支代码（CSEL/位操作）替代 if-else。
</details>

5. 树莓派 4B 的 CPU 上电后从哪个 EL 开始执行？
<details><summary>答案</summary>
取决于配置。默认 Pi4 的 GPU 固件加载 kernel8.img 后，CPU 从 EL3 开始（如果有 TF-A）或 EL2 开始。Linux 内核 head.S 会检查 CurrentEL，如果在 EL2/EL3 则降级到 EL1 后再运行内核。裸机程序如果不用 TF-A，需要在启动代码中手动降级到 EL1。
</details>

## 参考与延伸

- 原书 §1.4
- [Pi5 适配](../../PI5-ADAPT.md)
- [Ch15 Cache 基础](../../chapter-15-cache-basics/notes/section-0-本章完整概述.md)
- Cortex-A72 Technical Reference Manual
