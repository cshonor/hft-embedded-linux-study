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

### 实验 22-1：浮点运算

```c
// 验证 FP 寄存器行为
#include <stdio.h>
#include <arm_neon.h>

void test_fp_registers() {
    // 写 S0（32位），检查高位是否保留
    float32x4_t v = vdupq_n_f32(1.0f);  // V0 = {1,1,1,1}
    // 写 S0（低位 32 位）
    asm volatile("fmov s0, %w0" :: "r"(0x40490FDB));  // pi 的 float
    // 读 V0
    float32x4_t result;
    asm volatile("mov %0.16b, v0.16b" : "=w"(result));
    // result = {pi, 1, 1, 1} — 高位保留！

    float lane0 = vgetq_lane_f32(result, 0);
    float lane1 = vgetq_lane_f32(result, 1);
    printf("lane0=%.4f lane1=%.4f\n", lane0, lane1);
}

// 检查 FPCR
void test_fpcr() {
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    printf("FPCR = 0x%lx\n", fpcr);
    printf("  FZ (Flush-to-Zero) = %d\n", (fpcr >> 24) & 1);
    printf("  DN (Default-NaN)   = %d\n", (fpcr >> 12) & 1);
}
```

### 实验 22-2：RGB→BGR 三实现对比

三种实现对比：
1. **纯 C**：逐像素循环，基线
2. **NEON 汇编**：LD3 + ST3，手写汇编
3. **NEON intrinsics**：`vld3q_u8` + `vst3q_u8`，C 接口

```c
// 性能测量框架
#include <time.h>

#define BENCH(fn, data, n, iters) ({ \
    struct timespec _s, _e; \
    clock_gettime(CLOCK_MONOTONIC, &_s); \
    for (int _i = 0; _i < (iters); _i++) \
        fn(data, data, n); \
    clock_gettime(CLOCK_MONOTONIC, &_e); \
    double _ns = (_e.tv_sec - _s.tv_sec) * 1e9 \
               + (_e.tv_nsec - _s.tv_nsec); \
    printf("%-20s: %.1f ns/pixel (%d iters)\n", \
           #fn, _ns / (n * iters), iters); \
})

int main() {
    int n = 1920 * 1080 * 3;  // Full HD
    uint8_t *data = malloc(n);
    fill_random(data, n);

    BENCH(rgb_to_bgr_c, data, n, 100);
    BENCH(rgb_to_bgr_neon, data, n, 100);
    BENCH(rgb_to_bgr_intrinsics, data, n, 100);

    free(data);
    return 0;
}
```

| 实现 | 典型性能 | 代码复杂度 |
|------|---------|-----------|
| 纯 C | ~3-5 ns/pixel | 低 |
| NEON 汇编 | ~0.2-0.3 ns/pixel | 高 |
| NEON intrinsics | ~0.2-0.4 ns/pixel | 中 |

### 实验 22-3：8×8 矩阵乘法

8×8 矩阵乘法需要分块（blocking）策略：

```c
// 8×8 分块为 4 个 4×4
void matmul8x8_neon(float C[8][8], const float A[8][8],
                     const float B[8][8]) {
    // 清零 C
    memset(C, 0, sizeof(float) * 64);

    // 分块：4 个 4×4 子矩阵
    for (int bi = 0; bi < 8; bi += 4)
      for (int bj = 0; bj < 8; bj += 4)
        for (int bk = 0; bk < 8; bk += 4) {
          // 4×4 子矩阵乘加
          matmul4x4_neon(
            (float(*)[4])&C[bi][bj],
            (const float(*)[4])&A[bi][bk],
            (const float(*)[4])&B[bk][bj]);
        }
}
```

| 分块策略 | 寄存器使用 | cache 友好度 |
|----------|-----------|-------------|
| 4×4 直接 | 9/32 | 好 |
| 8×8 全展开 | 24/32 | 差（spill） |
| 4×4 + 预取 | 10/32 | 最好 |

## HFT 关联

实验 22-2 的三实现对比是 HFT 性能优化的典型方法：先写正确的 C 版本（基线），再写汇编/intrinsics 加速版本，最后用 benchmark 验证加速效果。

```c
// HFT NEON 加速验证流程
// 1. 写正确版本（纯 C）
// 2. 写加速版本（intrinsics）
// 3. 验证结果一致
// 4. benchmark 对比
// 5. objdump 检查生成代码

void hft_neon_validation() {
    float A[4][4] = {/* ... */};
    float B[4][4] = {/* ... */};
    float C_scalar[4][4], C_neon[4][4];

    // 1. 标量基线
    matmul4x4_scalar(C_scalar, A, B);

    // 2. NEON 版本
    matmul4x4_neon((float *)C_neon, (float *)A, (float *)B);

    // 3. 验证结果一致（允许浮点误差）
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++) {
        float diff = fabsf(C_scalar[i][j] - C_neon[i][j]);
        if (diff > 1e-5) {
            printf("MISMATCH at [%d][%d]: %f vs %f (diff=%e)\n",
                   i, j, C_scalar[i][j], C_neon[i][j], diff);
        }
      }

    // 4. Benchmark
    // ... clock_gettime 测量 ...
}
```

| HFT NEON 使用场景 | 加速比 | 注意事项 |
|------------------|--------|---------|
| 矩阵乘法 | 3-5× | 注意数据对齐 |
| 价格批量更新 | 3-4× | float32 足够 |
| Checksum 计算 | 10-15× | .16b 后缀 |
| 字符串搜索 | 8-12× | .16b 后缀 |
| Kalman 滤波 | 3-4× | 4×4 矩阵 |

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
