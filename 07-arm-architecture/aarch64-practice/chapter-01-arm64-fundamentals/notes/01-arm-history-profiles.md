# 1.1 ARM 架构历史与产品线

> 来源：§1.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ARM 从 v1 到 v9 的演进脉络，Cortex-A/R/M 三条产品线的定位，以及 ARMv8-A 引入 AArch64 的重大变革。

## 核心要点

| 产品线 | 角色 | MMU | 典型 |
|--------|------|-----|------|
| Cortex-A | 应用处理器，跑 Linux | 有 | 手机 SoC、树莓派 |
| Cortex-R | 硬实时 | 无 | 汽车刹车、工业安全 |
| Cortex-M | MCU | 无 | STM32、FreeRTOS |

- ARMv8-A 两套执行状态：**AArch64**（64 位 A64 指令）+ **AArch32**（兼容 A32/T32）
- ARMv9-A 在 v8 上增 SVE2、CCA、安全扩展；部分 v9 核砍掉 AArch32
- **架构规范**（ARMv8-A）≠ **具体 CPU 核**（Cortex-A72/A76）

## HFT 关联

HFT 交易系统通常跑在 x86 服务器上，但 ARM Cortex-A 正在进入低延迟领域（如 AWS Graviton、Ampere Altra）。理解 AArch64 的执行状态和产品线定位，有助于评估 ARM 平台对 HFT 的适用性：Cortex-A 有 MMU 和完整 OS 支持，适合跑交易栈；Cortex-R 的确定性中断响应适合飞控/硬实时，但不适合通用交易系统。ARMv9 的 SVE2 向量扩展在批量数据处理（如期权定价矩阵）中有潜力。

## 自测题

1. Cortex-M 支持 AArch64 吗？为什么？
<details><summary>答案</summary>
不支持。Cortex-M 是 32 位 MCU 线，不升级到 AArch64。只有 Cortex-A 才有 AArch64 执行状态。
</details>

2. 树莓派 5（Cortex-A76）能跑老的 32 位 ARM 用户程序吗？
<details><summary>答案</summary>
不能。A76 是 ARMv9 核，部分 v9 核砍掉了 AArch32 执行状态。Pi5 只能跑 AArch64 用户程序。
</details>

3. ARMv8-A 和 Cortex-A72 是什么关系？
<details><summary>答案</summary>
ARMv8-A 是架构规范（说明书），Cortex-A72 是按该规范实现的具体 CPU 核。一个架构可以被多个 CPU 核实现。
</details>

## 参考与延伸

- 原书 §1.1
- [AArch64 命名辨析](../../AARCH64-NAMING.md)
- ARM Architecture Reference Manual (ARMv8-A) §A1.1
