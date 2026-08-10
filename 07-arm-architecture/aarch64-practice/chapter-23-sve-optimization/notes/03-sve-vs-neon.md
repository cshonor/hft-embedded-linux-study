# §23.3 SVE vs NEON

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

SVE 和 NEON 的全面对比：向量长度、谓词、条件执行、加载方式、适用场景、可用性。理解两者差异有助于选择合适的 SIMD 方案。

## 核心要点

### 全面对比表

| 特性 | NEON | SVE | SVE2 |
|------|------|-----|------|
| 向量长度 | 固定 128 位 | 可变 128-2048 | 可变 128-2048 |
| 谓词寄存器 | 无 | P0-P15 | P0-P15 |
| 聚集加载 | 无 | gather/scatter | gather/scatter |
| 条件执行 | branch/位运算 | 谓词 | 谓词 |
| 寄存器 | V0-V31 (128b) | Z0-Z31 (可变) | Z0-Z31 + V0-V31 |
| 长度无关 | 否 | 是 | 是 |
| 查表(TBL) | 有 | 无 | 有 |
| 复数运算 | 有 | 无 | 有 |
| AES/SHA | 有 | 无 | 有 |
| 架构版本 | ARMv7+ | ARMv8.2-A+ | ARMv9 |
| 可用性 | 所有 ARM | 部分 ARM | ARMv9 芯片 |

### NEON 优势

- **普及度高**——所有 ARMv7-A/ARMv8-A 芯片都支持
- **工具链成熟**——intrinsics、auto-vectorization 完善好
- **128 位够用**——图像/音频/矩阵乘的典型数据量
- **代码可移植**——不依赖特定 VQ

### SVE 优势

- **向量长度无关**——代码可移植到未来更宽的硬件
- **谓词驱动**——无 branch 条件计算
- **Gather/Scatter**——非连续内存批量访问
- **更宽的向量**——256/512 位提供更高吞吐

### SVE2 = SVE + NEON 功能合并

```
SVE (ARMv8.2-A)     → 面向 HPC/科学计算
  + 谓词、可变长、gather/scatter

SVE2 (ARMv9)        → 统一 SIMD 标准
  = SVE 全部功能
  + NEON 数据处理指令（TBL、复数、AES/SHA）
  + 向后兼容 NEON（V0-V31 是 Z0-Z31 的低 128 位）

ARMv9 芯片：只需 SVE2，不再需要独立 NEON 单元
NEON 代码在 ARMv9 上通过 V0-V31 别名自动兼容
```

### NEON → SVE 迁移对照

| NEON 代码 | SVE 等价 | 说明 |
|----------|---------|------|
| `vld1q_f32(ptr)` | `svld1_f32(svptrue_b32(), ptr)` | 全向量加载 |
| `vaddq_f32(a, b)` | `svadd_f32_x(svptrue_b32(), a, b)` | 全向量加法 |
| `vmulq_f32(a, b)` | `svmul_f32_x(svptrue_b32(), a, b)` | 全向量乘法 |
| `vmaxq_f32(a, b)` | `svmax_f32_x(svptrue_b32(), a, b)` | 取最大 |
| `vaddvq_f32(v)` | `svaddv_f32(svptrue_b32(), v)` | 水平加 |
| `vld3q_u8(ptr)` | gather + 谓词 | 交错加载(更灵活) |
| `vbslq_f32(mask,a,b)` | 谓词控制 | 条件选择(更高效) |

### 编程模型对比

```c
// NEON：固定通道数，硬编码 4
void add_neon(float *a, float *b, float *c, int n) {
    int i;
    for (i = 0; i + 4 <= n; i += 4) {  // 硬编码 4
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        vst1q_f32(c + i, vaddq_f32(va, vb));
    }
    // 尾部处理（手动）
    for (; i < n; i++)
        c[i] = a[i] + b[i];
}

// SVE：自动适配通道数，无尾部代码
void add_sve(float *a, float *b, float *c, int64_t n) {
    int64_t i = 0;
    svbool_t pg = svwhilelt_b32_s64(i, n);
    while (svptest_first(svptrue_b32(), pg)) {
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        svst1_f32(pg, c + i, svadd_f32_x(pg, va, vb));
        i += svcntw();
        pg = svwhilelt_b32_s64(i, n);
    }
    // 无需尾部处理！谓词自动处理
}
```

### 适用场景对比

| 场景 | 推荐选择 | 原因 |
|------|---------|------|
| 图像处理 | NEON | 固定 128 位够用，LD3/ST3 交错加载 |
| 音频处理 | NEON | 同上，数据量适中 |
| 4×4 矩阵乘 | NEON | 4 路并行正好匹配 |
| 大矩阵(HPC) | SVE | 可变长自动获得更高并行 |
| 非连续数据 | SVE | gather/scatter |
| 条件过滤 | SVE | 谓词无 branch |
| 字符串处理 | SVE2 | 谓词检测 '\0' |
| 密码学 | SVE2/NEON | AES/SHA 指令 |

## HFT 关联

当前 HFT 在 ARM 上的 SIMD 选择：**以 NEON 为主**，原因：(1) 所有 ARM 芯片都支持，部署无兼容性问题；(2) NEON intrinsics 工具链成熟；(3) 128 位对于 HFT 常见的 4-8 路并行已足够。

```c
// HFT 跨平台 SIMD 选择策略
#if defined(__ARM_FEATURE_SVE2)
  // SVE2 可用（ARMv9 服务器）
  #include <arm_sve.h>
  #define SIMD_NAME "SVE2"
  // 用 SVE2 intrinsics

#elif defined(__ARM_NEON)
  // NEON 可用（所有 ARMv8）
  #include <arm_neon.h>
  #define SIMD_NAME "NEON"
  // 用 NEON intrinsics

#elif defined(__AVX2__)
  // x86 AVX2
  #include <immintrin.h>
  #define SIMD_NAME "AVX2"

#else
  // 纯 C fallback
  #define SIMD_NAME "Scalar"
#endif

// HFT 部署决策矩阵
// +-----------+--------+---------+----------+
// | 芯片       | NEON  | SVE    | 推荐     |
// +-----------+--------+---------+----------+
// | A72/A76   | ✅    | ❌     | NEON     |
// | Graviton3 | ✅    | VQ=2   | SVE 评估 |
// | Graviton4 | ✅    | VQ=2   | SVE      |
// | ARMv9     | 兼容  | SVE2   | SVE2     |
// +-----------+--------+---------+----------+
```

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
