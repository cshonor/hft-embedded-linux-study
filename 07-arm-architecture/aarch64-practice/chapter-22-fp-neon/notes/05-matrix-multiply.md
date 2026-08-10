# §22.5 矩阵乘法加速

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

用 NEON 的 FMLA 指令加速 4×4 矩阵乘法：标量需要 64 次乘法 + 48 次加法，NEON 只需 16 条 FMUL/FMLA 指令（4 路并行），加速约 4 倍。

## 核心要点

### 标量 vs NEON

| 方面 | 标量 C | NEON .4s |
|------|--------|----------|
| 乘法次数 | 64 | 16 条 FMUL/FMLA |
| 加法次数 | 48 | 包含在 FMLA 中 |
| 总指令数 | ~112 | 16 |
| 并行度 | 1 | 4 |
| 理论加速 | 1× | ~7× |
| 实际加速 | 1× | ~3-5× |

### 4×4 矩阵乘法原理

```
C = A × B

A = | a00 a01 a02 a03 |    B = | b00 b01 b02 b03 |
    | a10 a11 a12 a13 |        | b10 b11 b12 b13 |
    | a20 a21 a22 a23 |        | b20 b21 b22 b23 |
    | a30 a31 a32 a33 |        | b30 b31 b32 b33 |

C[i][j] = Σ A[i][k] × B[k][j]   (k=0..3)

标量: 4×4×4 = 64 次乘法 + 4×4×3 = 48 次加法 = 112 条指令

NEON: 利用 by-element 寻址，4 路并行
     A 的一行(.4s) × B 的一个元素(.s[n])
     → 4 个 C 元素同时计算
```

### NEON 4×4 矩阵乘法

```asm
; A 在 v0-v3（每行一个 .4s）
; B 列在 v4-v7（每列一个 .4s）
; 结果 C 在 v8-v11

    ; C 的第 0 列 = A 的各行 × B[0][0..3]
    fmul v8.4s, v0.4s, v4.s[0]     ; C[0][0..3] = A[0] × B[0][0]
    fmla v8.4s, v1.4s, v4.s[1]     ; += A[1] × B[1][0]
    fmla v8.4s, v2.4s, v4.s[2]     ; += A[2] × B[2][0]
    fmla v8.4s, v3.4s, v4.s[3]     ; += A[3] × B[3][0]

    ; C 的第 1 列
    fmul v9.4s, v0.4s, v5.s[0]
    fmla v9.4s, v1.4s, v5.s[1]
    fmla v9.4s, v2.4s, v5.s[2]
    fmla v9.4s, v3.4s, v5.s[3]

    ; C 的第 2、3 列同理...
    ; 总计: 4 列 × 4 条 = 16 条指令
```

### intrinsics 版本

```c
#include <arm_neon.h>
void matmul4x4_neon(float *C, const float *A, const float *B) {
    // 加载 A 的 4 行
    float32x4_t a0 = vld1q_f32(A);
    float32x4_t a1 = vld1q_f32(A + 4);
    float32x4_t a2 = vld1q_f32(A + 8);
    float32x4_t a3 = vld1q_f32(A + 12);

    // 加载 B 的 4 列（需要转置或按列存储）
    float32x4_t b0 = vld1q_f32(B);
    float32x4_t b1 = vld1q_f32(B + 4);
    float32x4_t b2 = vld1q_f32(B + 8);
    float32x4_t b3 = vld1q_f32(B + 12);

    // C 的第 0 行
    float32x4_t c0 = vmulq_n_f32(a0, vgetq_lane_f32(b0, 0));
    c0 = vfmaq_n_f32(c0, a1, vgetq_lane_f32(b0, 1));
    c0 = vfmaq_n_f32(c0, a2, vgetq_lane_f32(b0, 2));
    c0 = vfmaq_n_f32(c0, a3, vgetq_lane_f32(b0, 3));

    // ... 其他行同理
    vst1q_f32(C, c0);
    // ...
}
```

### FMLA 优势分析

```
标量计算 C[i][j]（一个元素）:
  4 次乘法 + 3 次加法 = 7 条指令

NEON 计算 C 的 4 个元素（一行）:
  1 条 FMUL + 3 条 FMLA = 4 条指令

加速比 = 7 × 4 / 4 = 7×（理论）
实际 ~3-5×（受限于内存带宽和数据依赖）
```

### 更大矩阵的分块策略

```
8×8 矩阵乘法 → 分为 4 个 4×4 子块

A = | A00 A01 |    B = | B00 B01 |    C = | C00 C01 |
    | A10 A11 |        | B10 B11 |        | C10 C11 |

C00 = A00×B00 + A01×B10
C01 = A00×B01 + A01×B11
C10 = A10×B00 + A11×B10
C11 = A10×B01 + A11×B11

每个子块乘法用 4×4 NEON 内核
总计: 8 次 4×4 矩阵乘 = 8 × 16 = 128 条指令
```

| 矩阵大小 | 标量指令 | NEON 指令 | 加速比 | 策略 |
|----------|---------|-----------|--------|------|
| 4×4 | 112 | 16 | ~7× | 直接 FMLA |
| 8×8 | 896 | 128 | ~7× | 分块 4×4 |
| 16×16 | 14336 | 2048 | ~7× | 分块 + 预取 |
| 32×32 | 114688 | 16384 | ~5× | 分块 + tiling |

## HFT 关联

矩阵乘法在 HFT 中的直接应用：(1) **Kalman 滤波**的预测和更新步骤需要矩阵乘法和加法，4×4 或 8×8 矩阵用 NEON 可获得显著加速；(2) **主成分分析（PCA）**的投影计算；(3) **风险模型**的因子暴露度计算。

```c
// HFT Kalman 滤波 NEON 加速
// 状态预测: x_pred = A × x + B × u
// A: 4×4 状态转移矩阵
// x: 4×1 状态向量
// u: 4×1 控制输入
void kalman_predict_neon(
    const float A[4][4], const float x[4],
    const float B[4][4], const float u[4],
    float x_pred[4]) {

    float32x4_t xv = vld1q_f32(x);
    float32x4_t uv = vld1q_f32(u);
    float32x4_t result = vdupq_n_f32(0.0f);

    // A × x (4×4 × 4×1 = 4×1)
    for (int i = 0; i < 4; i++) {
        float32x4_t arow = vld1q_f32(A[i]);
        float dot = vaddvq_f32(vmulq_f32(arow, xv));
        result = vsetq_lane_f32(dot, result, i);
    }

    // B × u (同理，累加到 result)
    for (int i = 0; i < 4; i++) {
        float32x4_t brow = vld1q_f32(B[i]);
        float dot = vaddvq_f32(vmulq_f32(brow, uv));
        result = vsetq_lane_f32(
            vgetq_lane_f32(result, i) + dot, result, i);
    }

    vst1q_f32(x_pred, result);
}

// HFT 协方差矩阵计算
// Cov = X^T × X / N  (X 是 N×4 数据矩阵)
void hft_covariance_neon(const float *data, int n,
                          float cov[4][4]) {
    float32x4_t sum0 = vdupq_n_f32(0);
    float32x4_t sum1 = vdupq_n_f32(0);
    float32x4_t sum2 = vdupq_n_f32(0);
    float32x4_t sum3 = vdupq_n_f32(0);

    for (int i = 0; i < n; i++) {
        float32x4_t row = vld1q_f32(data + i * 4);
        sum0 = vfmaq_laneq_f32(sum0, row, row, 0);
        sum1 = vfmaq_laneq_f32(sum1, row, row, 1);
        sum2 = vfmaq_laneq_f32(sum2, row, row, 2);
        sum3 = vfmaq_laneq_f32(sum3, row, row, 3);
    }
    // sum0/sum1/sum2/sum3 是协方差矩阵的 4 列
    // 除以 N 得到最终结果
}
```

## 自测题

1. **4×4 矩阵乘法标量需要多少次乘法？NEON 需要多少条指令？**

<details>
<summary>答案</summary>

4×4 矩阵乘法有 4×4×4 = **64 次乘法**和 4×4×3 = **48 次加法**（标量）。NEON 用 `.4s` 后缀 4 路并行：每列 1 条 FMUL + 3 条 FMLA = 4 条指令，4 列共 **16 条指令**。理论加速比 = (64+48)/16 = 7×（未考虑指令级并行和流水线）。实际加速取决于数据是否在 cache 中、FMLA 吞吐率（Cortex-A76 上 FMLA 吞吐约每周期 2 条）。
</details>

2. **FMLA 中的 .s[0] / .s[1] 是什么语法？**

<details>
<summary>答案</summary>

这是 NEON 的**标量元素索引**（by-element）寻址。`v4.s[0]` 取 V4 寄存器的第 0 个 32 位通道作为标量，与 V0.4s 的 4 个通道分别相乘。这种模式适合矩阵乘法：A 的一行（.4s）乘以 B 的一个元素（.s[n]），4 路并行。`FMUL V8.4s, V0.4s, V4.s[0]` 等价于：V8[0]=V0[0]×V4[0], V8[1]=V0[1]×V4[0], V8[2]=V0[2]×V4[0], V8[3]=V0[3]×V4[0]——用标量广播乘法。
</details>

3. **为什么矩阵乘法适合用 NEON？什么样的算法不适合？**

<details>
<summary>答案</summary>

矩阵乘法适合 NEON 因为：(1) **数据规则排列**——连续内存、对齐访问；(2) **计算密集**——算术/访存比高，NEON 的并行度能充分发挥；(3) **无分支**——不含条件跳转，流水线不断。不适合 NEON 的算法：(1) 有大量条件分支的算法（如二叉搜索）；(2) 访存密集型（如随机内存访问——gather load 效率低）；(3) 数据依赖链长（前一条结果是后一条输入——无法并行）。
</details>

## 参考与延伸

- [§22.3 常用 NEON 指令](03-neon-instructions.md) — FMUL/FMLA 指令
- [§22.6 NEON 内建函数](06-intrinsics.md) — C 层面矩阵乘
- [Ch23 SVE 优化](../../chapter-23-sve-optimization/notes/section-0-本章完整概述.md) — SVE 的更宽向量乘法
