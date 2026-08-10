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
// acc[i] += a[i] * b[i]  (4 路并行)

// by-element 版本（矩阵乘法用）
acc = vfmaq_n_f32(acc, a, 3.14f);     // FMLA V0.4s, V1.4s, V2.s[0]
// acc[i] += a[i] * 3.14f  (广播标量)
```

### intrinsics 命名规则

```
v [操作] [q] _ [类型]

v     = NEON intrinsic 前缀
操作  = add/mul/fmla/max/min/ld1/st1/...
q     = quad (128位)，省略 = 64位
类型  = f32/f64/u8/u16/u32/u64/s8/s16/s32/s64

示例:
vaddq_f32   → FADD V0.4s   (128位, 4×float32)
vadd_f32    → FADD V0.2s   (64位, 2×float32)
vaddq_u8    → ADD V0.16b   (128位, 16×uint8)
vmlaq_s16   → MLA V0.8h    (128位, 8×int16)
```

| 前缀 | 含义 | 示例 |
|------|------|------|
| `v` | NEON intrinsic | `vaddq_f32` |
| `q` | 128位（quad） | `vaddq_f32` → ADD .4s |
| (无q) | 64位 | `vadd_f32` → ADD .2s |
| `_f32` | 32位浮点 | float32x4_t |
| `_f64` | 64位浮点 | float64x2_t |
| `_u8` | 8位无符号 | uint8x16_t |
| `_s32` | 32位有符号 | int32x4_t |

### 常用 intrinsics 速查

| 操作 | intrinsic | 指令 | 说明 |
|------|-----------|------|------|
| 加载 | `vld1q_f32` | LD1 | 连续加载 4×float32 |
| 存储 | `vst1q_f32` | ST1 | 连续存储 |
| 加法 | `vaddq_f32` | FADD | 4 路加 |
| 乘法 | `vmulq_f32` | FMUL | 4 路乘 |
| 乘加 | `vfmaq_f32` | FMLA | 4 路 V0+=V1×V2 |
| 广播 | `vdupq_n_f32` | DUP | 标量→4 lane |
| 取 lane | `vgetq_lane_f32` | MOV | 取第 n lane |
| 设 lane | `vsetq_lane_f32` | MOV | 设第 n lane |
| 最大值 | `vmaxq_f32` | FMAX | 4 路取最大 |
| 最小值 | `vminq_f32` | FMIN | 4 路取最小 |
| 水平加 | `vaddvq_f32` | ADDV | 4 lane 求和 |
| 交错加载 | `vld3q_u8` | LD3 | 3 路解交织 |
| 交错存储 | `vst3q_u8` | ST3 | 3 路交织存储 |

### intrinsics vs 汇编

| 方面 | Intrinsics | 汇编 |
|------|-----------|------|
| 可读性 | 好（C 函数名） | 差（助记符） |
| 寄存器分配 | 编译器优化 | 手动 |
| 跨平台 | `#ifdef __ARM_NEON` | 架构绑定 |
| 调试 | 有符号名 | 汇编级 |
| 控制力 | 受编译器影响 | 完全控制 |
| 性能 | 接近汇编（可能略差） | 最优 |
| 指令调度 | 编译器决定 | 手动 |

### 跨平台条件编译

```c
// 跨架构 SIMD 代码模板
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  #include <arm_neon.h>
  #define SIMD_ADD4(a, b) vaddq_f32(a, b)
  #define SIMD_LOAD(p)    vld1q_f32(p)
  #define SIMD_STORE(p,v) vst1q_f32(p, v)

#elif defined(__SSE2__)
  #include <emmintrin.h>
  #define SIMD_ADD4(a, b) _mm_add_ps(a, b)
  #define SIMD_LOAD(p)    _mm_load_ps(p)
  #define SIMD_STORE(p,v) _mm_store_ps(p, v)

#elif defined(__AVX2__)
  #include <immintrin.h>
  // AVX 支持 256 位（8×float32）

#else
  // 纯 C fallback
  #define SIMD_ADD4(a, b) scalar_add4(a, b)
#endif

// 使用——统一接口
void process(float *data, int n) {
    for (int i = 0; i < n; i += 4) {
        SIMD_VEC a = SIMD_LOAD(data + i);
        SIMD_VEC b = SIMD_LOAD(data + i + 4);
        SIMD_VEC c = SIMD_ADD4(a, b);
        SIMD_STORE(data + i, c);
    }
}
```

## HFT 关联

HFT 代码中 NEON intrinsics 比 asm 更实用：(1) 编译器可以做内联和寄存器分配优化；(2) 可以和 C 代码混用；(3) 跨平台。但要注意：(1) 不同编译器（GCC/Clang）的 intrinsics 优化质量不同；(2) intrinsics 可能产生比手写汇编更多的 register move 指令。

```c
// HFT NEON 优化验证流程
// 1. 写 intrinsics 版本
// 2. 编译: gcc -O3 -mfpu=neon -S
// 3. 检查生成指令: objdump -d
// 4. 确认无多余 register move
// 5. benchmark 对比

// HFT 订单簿 NEON 加速模板
#include <arm_neon.h>

// 批量更新订单价格（4 路并行）
static inline void hft_update_4_prices(
    float *prices, float32x4_t delta) {
    float32x4_t p = vld1q_f32(prices);
    p = vaddq_f32(p, delta);
    vst1q_f32(prices, p);
}

// 批量比较找最优价（4 路并行）
static inline float32x4_t hft_best_price(
    const float *prices, int n) {
    float32x4_t best = vld1q_f32(prices);
    for (int i = 4; i < n; i += 4) {
        float32x4_t cur = vld1q_f32(prices + i);
        best = vmaxq_f32(best, cur);
    }
    return best;
}

// 编译选项检查
#if !defined(__ARM_NEON)
  #error "NEON not enabled! Use -mfpu=neon or -march=armv8-a"
#endif
```

## 自测题

1. **`vaddq_f32` 中的 q 和 f32 分别代表什么？不加 q 的版本有什么区别？**

<details>
<summary>答案</summary>

`q` 代表 **quad**（128 位），操作 4 个 float32 通道（.4s）。不加 `q` 的 `vadd_f32` 操作 64 位，只有 2 个 float32 通道（.2s）。区别：`vaddq_f32` → `FADD V0.4s`（4 路并行），`vadd_f32` → `FADD V0.2s`（2 路并行）。类型也不同：`vaddq_f32` 返回 `float32x4_t`（128位），`vadd_f32` 返回 `float32x2_t`（64位）。通常用 `q` 版本以获得最大并行度。
</details>

2. **intrinsics 和内联汇编相比，有什么优势和劣势？**

<details>
<summary>答案</summary>

**优势**：(1) 编译器做寄存器分配——不用手动管理 V0-V31；(2) 可内联到 C 代码中——无函数调用开销；(3) 编译器可跨 intrinsics 优化——如合并指令、消除冗余 move；(4) 调试信息更好——有符号名和类型信息。**劣势**：(1) 编译器可能生成多余的 register move（不如手写汇编精简）；(2) 对指令序列的控制力不如 asm；(3) 某些特殊指令可能没有对应 intrinsic。HFT 热点通常用 intrinsics，用 `objdump` 验证生成代码质量。
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
