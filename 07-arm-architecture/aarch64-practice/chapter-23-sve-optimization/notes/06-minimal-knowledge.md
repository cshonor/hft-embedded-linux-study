# §23.6 精简要点（跳过级别的最小知识）

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

首遍跳过 SVE 时需要记住的最小知识集——4 条核心概念，确保有 NEON 基础后回来学时有锚点。

## 核心要点

### SVE 最小知识集

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | SVE = 可变长 + 谓词 | 同一代码适配不同向量长度 |
| 2 | 谓词 P0-P15 | 每通道条件执行，无需 branch |
| 3 | SVE2 = SVE + NEON 合并 | ARMv9 标准 SIMD |
| 4 | 首遍跳过 | 有 NEON 基础后回来学 |

### 为什么首遍可以跳过

1. **NEON 足够入门**——128 位固定向量，概念简单，工具链成熟
2. **SVE 硬件不普及**——目前只有少数 ARMv8.2-A+ / ARMv9 芯片支持
3. **概念跨度大**——谓词、VQ 无关编程、gather/scatter 是新概念，需要 NEON 基础
4. **实际项目用 NEON**——当前 ARM 平台的 SIMD 代码以 NEON 为主

### 回来学 SVE 的前置条件

- 熟练掌握 NEON intrinsics 和汇编
- 理解矩阵乘法等典型 NEON 优化模式
- 有 SVE 硬件或 QEMU `-cpu max` 环境可用

## HFT 关联

HFT 开发者的 SVE 知识需求取决于部署平台：如果部署在 Cortex-A72/A76（只有 NEON），SVE 可跳过；如果未来迁移到 AWS Graviton（支持 SVE）或 ARMv9 服务器，需要回来学。最小知识集确保你知道 SVE 存在、它的核心特性（可变长 + 谓词），在需要时能快速上手。HFT 的 NEON 代码迁移到 SVE 通常需要重写 intrinsics（`arm_neon.h` → `arm_sve.h`），但算法思路不变。

## 自测题

1. **SVE 和 NEON 的最大区别用一个词概括是什么？**

<details>
<summary>答案</summary>

**可变长**（Vector-Length Agnostic）。NEON 固定 128 位，SVE 支持 128-2048 位可变长。软件用 `svcntb()` 查询向量长度，代码自动适配不同硬件——这是 SVE 与 NEON 最本质的区别。其他区别（谓词、gather/scatter）都是建立在这个基础上的功能扩展。
</details>

2. **SVE2 相比 SVE 增加了什么？为什么 ARMv9 选 SVE2 作为标准？**

<details>
<summary>答案</summary>

SVE2 = SVE + **NEON 的数据处理功能**（TBL 查表、复数运算、AES/SHA 密码学等）。ARMv9 选 SVE2 作为标准因为：(1) SVE2 包含 NEON 的全部功能——向后兼容现有 NEON 代码；(2) SVE2 继承 SVE 的可变长和谓词——面向未来更宽的向量；(3) 统一 SIMD 编程模型——ARMv9 开发者只需学 SVE2，不需要同时维护 NEON 和 SVE 两套代码。ARMv9 芯片支持 SVE2（含 NEON 兼容模式）。
</details>

3. **首遍跳过 SVE 后，回来学的前置条件是什么？**

<details>
<summary>答案</summary>

三个前置条件：(1) **熟练掌握 NEON**——intrinsics 和汇编，能写矩阵乘法等典型优化代码；(2) **理解 NEON 优化的模式**——分块、向量化、寄存器分配、尾部处理；(3) **有 SVE 环境**——SVE 硬件（如 Pi5 Cortex-A76）或 QEMU `-cpu max` 模拟。有了 NEON 基础，SVE 的学习曲线主要在新概念：谓词驱动的条件执行、VQ 无关编程模型、gather/scatter 访存模式。这些概念不难，但需要实践才能掌握。
</details>

## 参考与延伸

- [§23.1 SVE 核心特性](01-sve-features.md) — SVE 完整特性
- [§23.3 SVE vs NEON](03-sve-vs-neon.md) — 何时用 SVE 何时用 NEON
- [Ch22 NEON 指令](../../chapter-22-fp-neon/notes/section-0-本章完整概述.md) — SVE 的前置基础
