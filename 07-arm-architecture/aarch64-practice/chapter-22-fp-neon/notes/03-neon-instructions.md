# §22.3 常用 NEON 指令

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

NEON 常用指令：数据加载/存储（LD1/LD2/LD3/ST1）、算术运算（ADD/MUL/FADD/FMUL/FMLA）、交错加载在数据处理中的优势。

## 核心要点

### 数据加载/存储

| 指令 | 行为 | 字节数 | 典型用途 |
|------|------|--------|----------|
| `LD1 {V0.4s}, [X0]` | 加载 4×32bit 到 V0 | 16 | 连续数据加载 |
| `LD1 {V0.4s}, [X0], #16` | 加载 + 后增址 | 16 | 循环遍历 |
| `LD2 {V0.4s,V1.4s}, [X0]` | 交错加载 2 通道 | 32 | 立体声分离 |
| `LD3 {V0.16b,V1.16b,V2.16b}, [X0]` | 交错加载 3 通道 | 48 | RGB 分离 |
| `LD4 {V0.4s,V1.4s,V2.4s,V3.4s}, [X0]` | 交错加载 4 通道 | 64 | RGBA/ARGB 分离 |
| `ST1 {V0.4s}, [X0]` | 存储 V0 | 16 | 连续数据存储 |
| `ST3 {V2.16b,V1.16b,V0.16b}, [X1]` | 交错存储 3 通道 | 48 | BGR 写回 |

### 交错加载可视化

```
内存布局：R0 G0 B0 R1 G1 B1 R2 G2 B2 ...  (RGB 交替)

LD1（连续加载）:
V0 = R0 G0 B0 R1  ← 通道混在一起，需要额外指令分离

LD3（交错加载）:
V0 = R0 R1 R2 ...  ← R 通道自动分离！
V1 = G0 G1 G2 ...  ← G 通道自动分离！
V2 = B0 B1 B2 ...  ← B 通道自动分离！

LD2（2路交错）: 适合立体声左右声道分离
V0 = L0 L1 L2 ...  ← 左声道
V1 = R0 R1 R2 ...  ← 右声道
```

### 算术运算

| 指令 | 行为 | 并行度(.4s) | 延迟(周期) |
|------|------|-------------|-----------|
| `ADD V0.4s, V1.4s, V2.4s` | 4 路整数加法 | 4× | 1-2 |
| `MUL V0.4s, V1.4s, V2.4s` | 4 路整数乘法 | 4× | 2-3 |
| `FADD V0.4s, V1.4s, V2.4s` | 4 路浮点加法 | 4× | 2-3 |
| `FMUL V0.4s, V1.4s, V2.4s` | 4 路浮点乘法 | 4× | 2-4 |
| `FMLA V0.4s, V1.4s, V2.4s` | 4 路乘加 V0+=V1×V2 | 4× | 3-4 |
| `FMLS V0.4s, V1.4s, V2.4s` | 4 路乘减 V0-=V1×V2 | 4× | 3-4 |
| `FDIV V0.4s, V1.4s, V2.4s` | 4 路浮点除法 | 4× | 8-15 |
| `FSQRT S0, S1` | 标量开方 | 1× | 15-30 |

> **FMLA 是矩阵乘法的核心**：一条指令完成 4 路乘加，吞吐量是标量的 4 倍。现代 ARM 核心有独立 FMA 单元，FMLA 吞吐可达每周期 1 条。

### 数据重排指令

| 指令 | 行为 | 用途 |
|------|------|------|
| `DUP V0.4s, V1.s[0]` | 标量广播到所有 lane | 常数广播 |
| `DUP V0.4s, #5` | 立即数广播 | 常数初始化 |
| `MOV V0.16b, V1.16b` | 寄存器复制 | 数据搬运 |
| `TBL V0.16b, {V1.16b}, V2.16b` | 查表重排 | 通道任意重排 |
| `REV16 V0.8h, V1.8h` | 字节序反转(16位) | 大小端转换 |
| `REV32 V0.4s, V1.4s` | 字节序反转(32位) | 大小端转换 |
| `UZP1/UZP2` | 解交错拆分 | 分离偶/奇 lane |
| `ZIP1/ZIP2` | 交错合并 | 合并两组 lane |
| `TRN1/TRN2` | 转置拆分 | 矩阵转置 |

### 比较与选择指令

```asm
; 逐 lane 比较，结果为全1(真)或全0(假)
FCMGT V0.4s, V1.4s, V2.4s   ; V1 > V2 ? 0xFF : 0x00
FCMEQ V0.4s, V1.4s, V2.4s   ; V1 == V2 ?
FCMGE V0.4s, V1.4s, V2.4s   ; V1 >= V2 ?

; 条件选择（类似三目运算）
BIF V0.16b, V1.16b, V2.16b  ; if 条件: V0 = V1, else V0
; 等价于: V0 = V2 ? V1 : V0 (按位选择)
```

### by-element 寻址

```asm
; 标量元素索引——用于矩阵乘法
FMUL V8.4s, V0.4s, V4.s[0]   ; V4 的 lane 0 广播乘 V0
; 等价于: V8[i] = V0[i] * V4[0]  (i=0..3)

FMLA V8.4s, V1.4s, V4.s[1]   ; V8 += V1 * V4[1] (广播)
; 等价于: V8[i] += V1[i] * V4[1]  (i=0..3)

; 适用场景：A 的一行 × B 的一个元素
; 对矩阵乘法 C = A × B 中的每一列
```

## HFT 关联

FMLA 的 4 路乘加在 HFT 算法中应用广泛：(1) Kalman 滤波的状态更新（矩阵乘法）可用 FMLA 加速 3-4 倍；(2) 多资产相关性计算的向量化；(3) 订单簿 Level-2 数据的批量处理。LD3/LD4 交错加载适合处理 NIC 收到的网络包中的多字段数据。

```c
// HFT Kalman 滤波 NEON 加速
#include <arm_neon.h>
// 状态预测: x = A * x + B * u
// A 是 4×4 矩阵，x 是 4×1 向量
void kalman_predict(float32x4_t *A, float32x4_t *x,
                     float32x4_t u) {
    float32x4_t result = vmulq_n_f32(A[0], vgetq_lane_f32(*x, 0));
    result = vfmaq_laneq_f32(result, A[1], *x, 1);
    result = vfmaq_laneq_f32(result, A[2], *x, 2);
    result = vfmaq_laneq_f32(result, A[3], *x, 3);
    *x = result;
}

// HFT 订单簿批量比较（4 路并行）
// 找最优价格
float32x4_t hft_find_best(const float *prices, int n) {
    float32x4_t best = vld1q_f32(prices);
    for (int i = 4; i < n; i += 4) {
        float32x4_t cur = vld1q_f32(prices + i);
        best = vmaxq_f32(best, cur);  // 4 路取最大
    }
    return best;
}
```

## 自测题

1. **FMLA V0.4s, V1.4s, V2.4s 做了什么操作？为什么适合矩阵乘法？**

<details>
<summary>答案</summary>

`FMLA V0.4s, V1.4s, V2.4s` 执行 **4 路乘加**：V0[lane] = V0[lane] + V1[lane] × V2[lane]（对 4 个 32 位通道并行）。矩阵乘法 C[i][j] = Σ A[i][k] × B[k][j] 的核心操作就是乘加。用 FMLA，一次计算 4 个 C 元素的乘加，将 4 次乘法 + 4 次加法（8 条指令）合并为 1 条指令。加上 FMA 单元通常有独立流水线，可以与其它指令并行执行，实际加速超过 4 倍。
</details>

2. **LD1 和 LD3 的区别是什么？处理 RGB 数据用哪个？**

<details>
<summary>答案</summary>

**LD1** 连续加载——按内存顺序填充寄存器，不改变数据排列。**LD3** 交错加载——将 3 路交错的数据（如 RGB 交替排列 R0G0B0R1G1B1...）自动分离到 3 个寄存器（V0=R0R1R2..., V1=G0G1G2..., V2=B0B1B2...）。处理 RGB 数据用 **LD3**，因为它自动解交织，省去手动用其他指令分离 R/G/B 的开销。存储时用 ST3 可重新交织。
</details>

3. **NEON 的 FADD 和标量 FADD 在精度上有区别吗？**

<details>
<summary>答案</summary>

**没有精度区别**——两者都遵循 IEEE 754 标准。但 NEON FADD 和标量 FADD 可能跑在**不同的执行单元**上（NEON 用 SIMD 单元，标量用 FP 单元），延迟和吞吐可能不同。另外，NEON 默认使用 **Flush-to-Zero**（FTZ）和 **Default-NaN**（DN）模式（`FPCR` 寄存器控制），不严格遵循 IEEE 754 的非正规数处理——如果需要精确处理非正规数，需配置 FPCR。HFT 场景中通常接受 FTZ（性能更好）。
</details>

## 参考与延伸

- [§22.4 RGB→BGR 转换](04-rgb-bgr.md) — LD3/ST3 实战
- [§22.5 矩阵乘法加速](05-matrix-multiply.md) — FMLA 在矩阵乘中的应用
- [§22.6 NEON 内建函数](06-intrinsics.md) — C 层面使用 NEON
