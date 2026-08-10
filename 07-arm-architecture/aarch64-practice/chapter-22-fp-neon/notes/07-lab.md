# §22.7 实验要点

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Ch22 的实验在 Linux 用户态完成（非 QEMU 裸机），通过 C/汇编/intrinsics 三种方式实现同一算法，对比 NEON 加速效果。

## 核心要点

### 实验列表

| 实验 | 内容 | 平台 | 关键知识点 |
|------|------|------|-----------|
| 22-1 | 浮点运算 | Linux | FP 寄存器、FPCR |
| 22-2 | RGB24→BGR32（C / NEON汇编 / intrinsics） | Linux | LD3/ST3、intrinsics |
| 22-3 | 8×8 矩阵乘法运算 | Linux | FMLA、分块策略 |

### 实验 22-2 要点

三种实现对比：
1. **纯 C**：逐像素循环，基线
2. **NEON 汇编**：LD3 + ST3，手写汇编
3. **NEON intrinsics**：`vld3q_u8` + `vst3q_u8`，C 接口

测量方法：`clock_gettime(CLOCK_MONOTONIC)` 前后计时，对比三者的吞吐量。

### 实验 22-3 要点

8×8 矩阵乘法需要分块（blocking）策略：
- 8×8 分为 4 个 4×4 子块
- 每个子块用 NEON 4×4 矩阵乘法
- 注意数据布局（行优先 vs 列优先）影响加载效率

## HFT 关联

实验 22-2 的三实现对比是 HFT 性能优化的典型方法：先写正确的 C 版本（基线），再写汇编/intrinsics 加速版本，最后用 benchmark 验证加速效果。HFT 代码中常见的模式：`#ifdef __ARM_NEON` 选择 NEON 版本，x86 上选 AVX 版本，fallback 到纯 C。实验 22-3 的分块策略在 HFT 大矩阵运算（如协方差矩阵）中直接使用。

## 自测题

1. **实验 22-2 中三种实现的性能排序是什么？为什么？**

<details>
<summary>答案</summary>

性能排序：**NEON 汇编 ≈ intrinsics > 纯 C**。NEON 汇编和 intrinsics 性能接近（intrinsics 可能略慢 5-10%，因为编译器可能插入额外 register move），两者都比纯 C 快 10-30 倍。纯 C 慢因为：(1) 逐像素处理，没有 SIMD 并行；(2) 每次循环有分支开销；(3) 编译器自动向量化（auto-vectorization）可能不成功或不是最优。实际项目中 intrinsics 是最佳选择——性能接近汇编且可维护。
</details>

2. **8×8 矩阵乘法为什么要分块？直接展开不行吗？**

<details>
<summary>答案</summary>

直接展开也可以，但分块的优势：(1) **复用 4×4 代码**——先写好测试过的 4×4 NEON 内核，8×8 只需调用 4 次；(2) **寄存器压力**——8×8 全展开需要 8 个寄存器存 A 行 + 8 个存 B 列 + 8 个累加器 = 24 个寄存器，接近 32 个上限，可能 spill 到栈。分块后每块只需 4+4+1=9 个寄存器，余量充足。(3) **cache 友好**——分块可以控制工作集大小在 L1 cache 内。实际 GPU 上的矩阵乘也用分块（tiling）策略。
</details>

## 参考与延伸

- [§22.8 易错点清单](08-pitfalls.md) — 实验中的常见错误
- [§22.4 RGB→BGR 转换](04-rgb-bgr.md) — 实验 22-2 的核心算法
- [§22.5 矩阵乘法加速](05-matrix-multiply.md) — 实验 22-3 的核心算法
