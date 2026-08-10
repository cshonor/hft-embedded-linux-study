# §22.6 NEON 内建函数（Intrinsics）

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

NEON intrinsics 是编译器提供的 C 函数接口，映射到 NEON 指令——兼具汇编的控制力和 C 的可读性，编译器可以做寄存器分配优化。

## 核心要点

### 基本用法

```c
#include <arm_neon.h>

// 4 路浮点加法
float32x4_t a = vld1q_f32(ptr_a);     // LD1 V0.4s, [ptr_a]
float32x4_t b = vld1q_f32(ptr_b);     // LD1 V1.4s, [ptr_b]
float32x4_t c = vaddq_f32(a, b);      // FADD V2.4s, V0.4s, V1.4s
vst1q_f32(ptr_c, c);                   // ST1 V2.4s, [ptr_c]
```

### 矩阵乘加

```c
float32x4_t acc = vdupq_n_f32(0.0f);  // 清零（DUP V0.4s, #0）
acc = vfmaq_f32(acc, a, b);           // FMLA V0.4s, V1.4s, V2.4s
```

### intrinsics 命名规则

| 前缀 | 含义 | 示例 |
|------|------|------|
| `v` | NEON intrinsic | `vaddq_f32` |
| `add` | 操作 | 加法 |
| `q` | 128位（quad） | `vaddq_f32` → ADD .4s |
| (无q) | 64位 | `vadd_f32` → ADD .2s |
| `_f32` | 32位浮点 | float32x4_t |
| `_u8` | 8位无符号 | uint8x16_t |

### intrinsics vs 汇编

| 方面 | Intrinsics | 汇编 |
|------|-----------|------|
| 可读性 | 好（C 函数名） | 差（助记符） |
| 寄存器分配 | 编译器优化 | 手动 |
| 跨平台 | `#ifdef __ARM_NEON` | 架构绑定 |
| 调试 | 有符号名 | 汇编级 |
| 控制力 | 受编译器影响 | 完全控制 |
| 性能 | 接近汇编（可能略差） | 最优 |

> **C++ NEON**：跨平台可用 `#ifdef __ARM_NEON` 条件编译，x86 平台 fallback 到 SSE/AVX。

## HFT 关联

HFT 代码中 NEON intrinsics 比 asm 更实用：(1) 编译器可以做内联和寄存器分配优化，减少数据搬运；(2) 可以和 C 代码混用，不需要单独的 .S 文件；(3) 跨平台——同一份代码在 Cortex-A72/A76/A78 上都能编译。但要注意：(1) 不同编译器（GCC/Clang）的 intrinsics 优化质量不同，需 benchmark；(2) intrinsics 可能产生比手写汇编更多的寄存器搬运指令（register move），热点代码可用 `objdump` 检查生成的指令。

## 自测题

1. **`vaddq_f32` 中的 q 和 f32 分别代表什么？不加 q 的版本有什么区别？**

<details>
<summary>答案</summary>

`q` 代表 **quad**（128 位），操作 4 个 float32 通道（.4s）。不加 `q` 的 `vadd_f32` 操作 64 位，只有 2 个 float32 通道（.2s）。区别：`vaddq_f32` → `FADD V0.4s`（4 路并行），`vadd_f32` → `FADD V0.2s`（2 路并行）。类型也不同：`vaddq_f32` 返回 `float32x4_t`（128位），`vadd_f32` 返回 `float32x2_t`（64位）。通常用 `q` 版本以获得最大并行度。
</details>

2. **intrinsics 和内联汇编相比，有什么优势和劣势？**

<details>
<summary>答案</summary>

**优势**：(1) 编译器做寄存器分配——不用手动管理 V0-V31；(2) 可内联到 C 代码中——无函数调用开销；(3) 编译器可跨 intrinsics 优化——如合并指令、消除冗余 move；(4) 调试信息更好——有符号名和类型信息。**劣势**：(1) 编译器可能生成多余的 register move（不如手写汇编精简）；(2) 对指令序列的控制力不如 asm（如无法强制指令调度）；(3) 某些特殊指令可能没有对应 intrinsic。HFT 热点通常用 intrinsics，用 `objdump` 验证生成代码质量。
</details>

3. **`#ifdef __ARM_NEON` 的作用是什么？HFT 代码为什么要用它？**

<details>
<summary>答案</summary>

`#ifdef __ARM_NEON` 检查编译器是否支持 NEON intrinsics。HFT 代码用它做**跨平台条件编译**：在 ARM 平台用 NEON 加速版本，在 x86 平台 fallback 到 SSE/AVX 版本或纯 C 版本。这样同一份代码可以在不同架构编译运行。实际项目中常见模式：`#if defined(__ARM_NEON) ... #elif defined(__SSE2__) ... #else ... #endif`。还可以用 `#ifdef __ARM_FEATURE_SVE` 检查 SVE 支持。
</details>

## 参考与延伸

- [§22.3 常用 NEON 指令](03-neon-instructions.md) — intrinsics 对应的底层指令
- [§22.5 矩阵乘法加速](05-matrix-multiply.md) — intrinsics 实现矩阵乘
- [Ch10 GCC 内联汇编](../../chapter-10-gcc-inline-asm/notes/section-0-本章完整概述.md) — asm volatile 语法
