# §23.3 SVE vs NEON

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

SVE 和 NEON 的全面对比：向量长度、谓词、条件执行、加载方式、适用场景、可用性。理解两者差异有助于选择合适的 SIMD 方案。

## 核心要点

### 对比表

| 特性 | NEON | SVE |
|------|------|-----|
| 向量长度 | 固定 128 位 | 可变（128-2048） |
| 谓词 | 无 | P0-P15 |
| 聚集加载 | 无 | 有（gather/scatter） |
| 条件执行 | branch 或位运算 | 谓词 |
| 适用 | 图像/音频/矩阵 | HPC/科学计算/AI |
| 可用性 | ARMv7+（几乎所有 ARM） | ARMv8.2-A SVE / ARMv9 SVE2 |
| 寄存器 | V0-V31（128位） | Z0-Z31（可变长） |
| 编程模型 | 固定通道数 | VQ 无关 |

### NEON 优势

- 普及度高——所有 ARMv7-A/ARMv8-A 芯片都支持
- 工具链成熟——intrinsics、auto-vectorization 完善好
- 128 位够用——图像/音频/矩阵乘的典型数据量

### SVE 优势

- 向量长度无关——代码可移植到未来更宽的硬件
- 谓词驱动——无 branch 条件计算
- Gather/Scatter——非连续内存批量访问
- 更宽的向量——256/512 位提供更高吞吐

### SVE2 = SVE + NEON 功能合并

SVE2 在 SVE 基础上增加了 NEON 的数据处理指令（如复数运算、查表、AES 等），是 ARMv9 的标准 SIMD 扩展。ARMv9 芯片同时支持 SVE2 和 NEON。

## HFT 关联

当前 HFT 在 ARM 上的 SIMD 选择：**以 NEON 为主**，原因：(1) 所有 ARM 芯片都支持，部署无兼容性问题；(2) NEON intrinsics 工具链成熟，编译器优化好；(3) 128 位对于 HFT 常见的 4-8 路并行（如 4 个 symbol 的价格更新）已足够。SVE 作为未来路径：当 ARM 服务器普及 VQ≥2（256位+）时，SVE 的自动加倍并行度有吸引力。短期可在测试环境评估 SVE 的 gather/scatter 在稀疏数据场景的性能。

## 自测题

1. **NEON 和 SVE 的向量长度有什么区别？这对软件开发有什么影响？**

<details>
<summary>答案</summary>

NEON **固定 128 位**——代码中通道数硬编码（如 `.4s` = 4 通道），换到更宽的硬件也不会变。SVE **可变长**（128-2048 位）——代码用 `svcntw()` 查询通道数，循环自动适配。对软件开发的影响：NEON 代码需要为不同向量长度写不同版本（或固定 128 位），SVE 写一份代码在所有 VQ 上运行。NEON 代码移植到 256 位硬件需要重写；SVE 代码在 256 位硬件上自动获得 2 倍吞吐——无需重编译。
</details>

2. **SVE2 相比 SVE 增加了什么？为什么说 SVE2 = SVE + NEON？**

<details>
<summary>答案</summary>

SVE2 = SVE + **NEON 的数据处理功能**。原始 SVE 主要面向 HPC/科学计算，缺少 NEON 的一些实用指令（如 TBL 查表、复数运算、字符串处理、密码学 AES/SHA）。SVE2 把这些 NEON 指令"移植"到 SVE 的向量长度无关框架中。所以 ARMv9 芯片只需支持 SVE2（包含 SVE 全部功能 + NEON 对应功能），不再需要单独的 NEON 单元。ARMv9 = AArch64 + SVE2（含 NEON 兼容模式）。
</details>

3. **当前 HFT 为什么以 NEON 为主而不是 SVE？**

<details>
<summary>答案</summary>

三个原因：(1) **可用性**——NEON 在所有 ARMv7+ 芯片上都有，SVE/SVE2 只在 ARMv8.2-A+/ARMv9 芯片上。HFT 部署的 ARM 芯片（如 Cortex-A72/A76）大多只有 NEON。(2) **工具链成熟度**——NEON intrinsics 和 auto-vectorization 经过十几年优化，编译器支持完善。SVE intrinsics 较新，编译器优化质量可能不如 NEON。(3) **性能收益有限**——HFT 常见的 4-8 路并行用 NEON 128 位已够。SVE 的优势在 HPC 级别的数千路并行，HFT 数据量通常不大。SVE 作为未来升级路径，等 ARM 服务器普及后再评估。
</details>

## 参考与延伸

- [§23.1 SVE 核心特性](01-sve-features.md) — SVE 整体架构
- [§23.2 谓词](02-predicate.md) — 谓词 vs NEON 位掩码
- [Ch22 NEON 指令](../../chapter-22-fp-neon/notes/section-0-本章完整概述.md) — NEON 的完整指令集
