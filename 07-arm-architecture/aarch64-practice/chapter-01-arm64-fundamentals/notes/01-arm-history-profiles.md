# 1.1 ARM 架构历史与产品线

> 来源：§1.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ARM 从 v1 到 v9 的演进脉络，Cortex-A/R/M 三条产品线的定位，以及 ARMv8-A 引入 AArch64 的重大变革。

## ARM 架构演进史

| 版本 | 年份 | 里程碑 | 关键特性 |
|------|------|--------|---------|
| ARMv1 | 1985 | 首个 ARM | 26 位地址，26 条指令 |
| ARMv3 | 1992 | 32 位 | 32 位地址空间，ARM6 |
| ARMv4T | 1995 | Thumb | 16 位 Thumb 指令集，ARM7TDMI |
| ARMv5 | 2002 | DSP/Jazelle | ARM9，XScale |
| ARMv6 | 2002 | SIMD | ARM11，NEON 前身 |
| ARMv7-A | 2005 | Cortex 时代 | Cortex-A8/A9/A15，NEON，VFP |
| ARMv8-A | 2011 | **64 位** | AArch64/A64 指令集，Cortex-A53/A57 |
| ARMv8.1-8.7 | 2016-2020 | 增强 | LSE 原子指令、PAN、BTI |
| ARMv9-A | 2021 | SVE2 | 向量扩展、CCA、MTE |

### 关键转折点

- **ARMv4T (1995)**：引入 Thumb 16 位指令集，降低代码体积 → ARM7TDMI 成为最畅销 ARM 核
- **ARMv7-A (2005)**：Cortex 品牌发布，引入 NEON SIMD → 智能手机时代
- **ARMv8-A (2011)**：**最重要的变革** → 64 位 AArch64 执行状态，全新 A64 指令集
- **ARMv9-A (2021)**：SVE2 可变长度向量 → HPC/AI 场景

## 三条产品线

| 产品线 | 角色 | MMU | 典型应用 | 代表核 |
|--------|------|-----|---------|--------|
| Cortex-A | 应用处理器 | ✓ | 手机/服务器（跑 Linux） | A72/A76/A78/X2 |
| Cortex-R | 硬实时 | ✗（MPU） | 汽车/工业安全 | R52/R82 |
| Cortex-M | MCU | ✗（MPU） | IoT/穿戴/电机 | M0/M4/M7/M55 |

### A vs R vs M 的区别

| 特性 | Cortex-A | Cortex-R | Cortex-M |
|------|---------|---------|---------|
| 架构 | ARMv8/v9-A | ARMv8-R | ARMv7-M/v8-M |
| 位数 | 64 位 (AArch64) | 32 位 | 32 位 |
| MMU | ✓（虚拟内存） | ✗（MPU） | ✗（MPU） |
| Cache | L1/L2/L3 | L1/L2 | 无或 L1 |
| OS | Linux/Android | RTOS（QNX/VxWorks） | FreeRTOS/裸机 |
| 中断 | GIC（向量中断） | 低延迟中断 | NVIC |
| 乱序 | ✓（大核） | ✗ 或简单 | ✗ |
| 典型频率 | 2-3 GHz | 1 GHz | 100-500 MHz |

## AArch64 vs AArch32

ARMv8-A 同时支持两套执行状态：

| 特性 | AArch64 | AArch32 |
|------|---------|---------|
| 位数 | 64 位 | 32 位 |
| 指令集 | A64（全新，定长 32 位） | A32 + T32（兼容 v7） |
| 通用寄存器 | X0-X30（31 个 64 位） | R0-R14（15 个 32 位） |
| 异常等级 | EL0-EL3 | EL0-EL3（但映射到 v7 模式） |
| 可与对方互转 | ✓（AArch32↔AArch64 切换） | ✓ |

### A64 指令集的特点

| 特性 | 说明 |
|------|------|
| 定长 32 位 | 每条指令正好 4 字节，简化取指 |
| 不支持条件执行 | 除 B.cond/CSEL/CBZ 等少数指令，不能像 A32 那样每条指令加条件后缀 |
| 取消 IT 块 | A32 的 IT（If-Then）块在 A64 中取消 |
| 新增指令 | LDP/STP（双寄存器加载/存储）、CSEL（条件选择） |

## 架构规范 vs CPU 核

```
架构规范（ARMv8-A）        具体实现（CPU 核）
  ┌──────────────┐         ┌──────────────┐
  │ 说明书         │         │ 芯片设计       │
  │ 定义指令集     │ ──实现→ │ Cortex-A53    │
  │ 定义寄存器     │         │ Cortex-A72    │
  │ 定义异常模型   │         │ Cortex-A76    │
  │ 定义内存模型   │         │ Neoverse N1   │
  └──────────────┘         │ Apple M1      │
                           └──────────────┘
```

**一个架构可被多个 CPU 核实现**，不同核微架构不同但指令兼容。

## ARM 生态中的厂商

| 厂商 | 角色 | 产品 |
|------|------|------|
| Arm Ltd. | 架构设计 + 参考核 | Cortex-A72/A76 |
| 高通 | 自研核 | Kryo |
| Apple | 自研核 | M1/M2（ARMv8 兼容） |
| Ampere | 服务器核 | Altra（ARMv8） |
| AWS | 服务器核 | Graviton（Neoverse） |
| 华为 | 自研核 | 鲲鹏（ARMv8） |

## HFT 关联

HFT 交易系统通常跑在 x86 服务器上，但 ARM Cortex-A 正在进入低延迟领域（如 AWS Graviton、Ampere Altra）。理解 AArch64 的执行状态和产品线定位，有助于评估 ARM 平台对 HFT 的适用性：Cortex-A 有 MMU 和完整 OS 支持，适合跑交易栈；Cortex-R 的确定性中断响应适合飞控/硬实时，但不适合通用交易系统。ARMv9 的 SVE2 向量扩展在批量数据处理（如期权定价矩阵）中有潜力。

## 自测题

1. Cortex-M 支持 AArch64 吗？为什么？
<details><summary>答案</summary>
不支持。Cortex-M 是 32 位 MCU 线（ARMv7-M/v8-M 架构），设计目标是低功耗、低成本，不需要 64 位地址空间。只有 Cortex-A（ARMv8-A/v9-A 架构）才有 AArch64 执行状态。Cortex-R 也是 32 位。
</details>

2. 树莓派 5（Cortex-A76）能跑老的 32 位 ARM 用户程序吗？
<details><summary>答案</summary>
不能。A76 是 ARMv8.2-A 架构但 Pi5 用的 SoC（BCM2712）配置为仅 AArch64。部分 v9 核砍掉了 AArch32 执行状态。Pi5 只能跑 AArch64 用户程序。Pi4（A72）仍支持 AArch32。
</details>

3. ARMv8-A 和 Cortex-A72 是什么关系？
<details><summary>答案</summary>
ARMv8-A 是架构规范（说明书），定义指令集、寄存器、异常模型等。Cortex-A72 是按 ARMv8-A 规范实现的具体 CPU 核（芯片设计）。一个架构可以被多个核实现：Cortex-A53/A57/A72/A76 都基于 ARMv8-A。
</details>

4. A64 指令为什么取消 IT 块（条件执行块）？
<details><summary>答案</summary>
（1）IT 块增加硬件复杂度（指令解码需跟踪 IT 状态）（2）现代乱序超标量中条件执行阻碍流水线优化（3）A64 用 CSEL/CSET/CSINC 等条件选择指令替代，实现无分支的条件逻辑，性能更好且更易乱序执行。
</details>

5. Apple M1 和 Cortex-A72 兼容吗？
<details><summary>答案</summary>
指令级兼容——都基于 ARMv8-A，M1 可以跑 A72 编译的二进制。但微架构完全不同（M1 是 Apple 自研的 Firestorm/Icestorm 核），cache 层次、流水线深度、频率都不同。性能特征差异很大。
</details>

## 参考与延伸

- 原书 §1.1
- [AArch64 命名辨析](../../AARCH64-NAMING.md)
- ARM Architecture Reference Manual (ARMv8-A) §A1.1
- [1.2 四个异常等级](02-exception-levels.md)
