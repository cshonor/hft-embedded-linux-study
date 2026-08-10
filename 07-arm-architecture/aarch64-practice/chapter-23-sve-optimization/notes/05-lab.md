# §23.5 实验要点

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Ch23 的 SVE 实验在 QEMU+ARM64 Linux 上完成，包括 RGB 转换、矩阵乘法和字符串操作的 SVE 优化。Pi5 的 Cortex-A76 支持 SVE2。

## 核心要点

### 实验列表

| 实验 | 内容 | 平台 | 关键知识点 |
|------|------|------|-----------|
| 23-1 | RGB24→BGR32（SVE 优化） | SVE | gather/scatter、谓词 |
| 23-2 | 8×8 矩阵乘法运算 | SVE | FMLA 向量化、谓词尾部 |
| 23-3 | 用 SVE 优化 strcpy() | SVE | 谓词检测 '\0'、批量操作 |

### 实验环境

```bash
# 方法1: QEMU 模拟 SVE
qemu-system-aarch64 -M virt -cpu max -smp 2 -m 2G \
    -kernel Image -append "root=/dev/vda" \
    -drive file=rootfs.img,format=raw

# 方法2: QEMU 指定 SVE 向量长度
qemu-system-aarch64 -M virt -cpu max,sve256=on \
    -smp 2 -m 2G -kernel Image ...

# 方法3: Pi5 原生 SVE2（Cortex-A76）
# 直接在 Pi5 上编译运行

# 检查 SVE 支持
cat /proc/cpuinfo | grep Features | grep -o sve
cat /proc/cpuinfo | grep Features | grep -o sve2

# 检查 SVE 向量长度
# 用户态通过 prctl(PR_SVE_GET_VL)
python3 -c "import ctypes; ..."
# 或直接运行 SVE 代码打印 svcntb()
```

### 实验 23-1：RGB→BGR（SVE 版本）

对比 NEON LD3 和 SVE gather 的 RGB→BGR 转换：

```c
// SVE 版本 RGB→BGR
#include <arm_sve.h>
void rgb_to_bgr_sve(const uint8_t *src, uint8_t *dst, int64_t n) {
    int64_t i = 0;
    svbool_t pg = svwhilelt_b8_s64(i, n);

    while (svptest_first(svptrue_b8(), pg)) {
        // 加载 R 通道
        svuint8_t r = svld1_gather_offset_u8(pg, src + i, 0);
        // 加载 G 通道
        svuint8_t g = svld1_gather_offset_u8(pg, src + i, 1);
        // 加载 B 通道
        svuint8_t b = svld1_gather_offset_u8(pg, src + i, 2);

        // 交换 R 和 B，用 scatter 存回
        svst1_scatter_offset_u8(pg, dst + i, 0, b);
        svst1_scatter_offset_u8(pg, dst + i, 1, g);
        svst1_scatter_offset_u8(pg, dst + i, 2, r);

        i += svcntb() * 3;
        pg = svwhilelt_b8_s64(i, n);
    }
}

// NEON LD3 版本（对比）
// LD3 自动解交织 + ST3 自动交织
// 比 SVE gather/scatter 更高效（固定模式）
```

### 实验 23-2：8×8 矩阵乘法（SVE 版本）

```c
#include <arm_sve.h>
// SVE 矩阵乘法（自动适配向量长度）
void matmul_sve(const float *A, const float *B,
                float *C, int n) {
    for (int i = 0; i < n; i++) {
        int64_t j = 0;
        svbool_t pg = svwhilelt_b32_s64(j, n);

        while (svptest_first(svptrue_b32(), pg)) {
            // C[i][j:j+vl] = sum(A[i][k] * B[k][j:j+vl])
            svfloat32_t acc = svdup_f32(0.0f);

            for (int k = 0; k < n; k++) {
                // 广播 A[i][k]
                svfloat32_t a = svdup_f32(A[i * n + k]);
                // 加载 B[k][j:j+vl]
                svfloat32_t b = svld1_f32(pg, B + k * n + j);
                // 乘加
                acc = svmla_f32_x(pg, acc, a, b);
            }
            svst1_f32(pg, C + i * n + j, acc);

            j += svcntw();
            pg = svwhilelt_b32_s64(j, n);
        }
    }
}
```

### 实验 23-3：SVE strcpy

```c
#include <arm_sve.h>
// SVE strcpy：一次处理一个向量的字节
char *strcpy_sve(char *dst, const char *src) {
    char *orig_dst = dst;
    svbool_t pg = svptrue_b8();

    while (1) {
        // 加载一个向量宽度的字节
        svuint8_t v = svld1_u8(pg, (const uint8_t *)src);

        // 检测 '\0' 位置
        svbool_t null_pg = svcmpeq_n_u8(pg, v, 0);

        if (svptest_first(svptrue_b8(), null_pg)) {
            // 有 '\0' → 只存储到 '\0' 为止
            // 用 BRK 指令生成前缀谓词
            svbool_t store_pg = svbrkb_z(pg, null_pg);
            // 存储 '\0' 本身
            svst1_u8(store_pg, (uint8_t *)dst, v);
            // 存 '\0'（null_pg 的第一个 true 通道）
            // ...
            break;
        }

        // 全部存储
        svst1_u8(pg, (uint8_t *)dst, v);
        src += svcntb();
        dst += svcntb();
    }
    return orig_dst;
}
```

### 实验结果预期

| 实验 | 标量 | NEON | SVE (VQ=1) | SVE (VQ=2) |
|------|------|------|-----------|-----------|
| 23-1 RGB | 1× | 10-15× | 5-10× | 8-15× |
| 23-2 矩阵 | 1× | 3-5× | 3-5× | 5-8× |
| 23-3 strcpy | 1× | 8-10× | 10-20× | 15-30× |

> 注意：SVE VQ=1 与 NEON 理论并行度相同，但谓词操作更高效。SVE gather/scatter 在固定模式（如 RGB 交错）可能比 NEON LD3 更慢。

## HFT 关联

SVE 实验的价值在于学习谓词驱动的向量化思维。HFT 中类似模式：(1) 批量检查多个订单是否满足条件（谓词过滤）；(2) 批量更新价格表中的多个条目（gather/scatter + 谓词）。

```c
// HFT SVE 实验评估清单
void hft_sve_evaluation() {
    // 1. 检查 SVE 可用性
    int sve_ver = hft_check_sve();  // 0=无, 1=SVE, 2=SVE2
    printf("SVE version: %d\n", sve_ver);

    if (sve_ver == 0) {
        printf("No SVE, using NEON\n");
        return;
    }

    // 2. 查询向量长度
    printf("Vector length: %lu bytes\n", svcntb());
    printf("  int32 lanes: %lu\n", svcntw());
    printf("  int64 lanes: %lu\n", svcntd());

    // 3. Benchmark 对比
    // NEON vs SVE 同一算法
    // 测量: clock_gettime + cntvct_el0

    // 4. 代码质量检查
    // gcc -O3 -S -march=armv8.2-a+sve code.c
    // objdump -d code.o | grep -c fmov
    // 检查多余的 register move
}
```

## 自测题

1. **如何在 QEMU 中模拟 SVE？Pi5 的 SVE 支持情况如何？**

<details>
<summary>答案</summary>

QEMU 用 `-cpu max` 模拟最大 CPU 特性（包括 SVE/SVE2）。也可以指定向量长度：`-cpu max,sve256=on` 启用 256 位 SVE。Pi5 的 Cortex-A76 **支持 SVE2**，VQ=1（128 位）。在 Pi5 上可以直接运行 SVE 代码，不需要 QEMU 模拟。检查支持：`cat /proc/cpuinfo | grep -o sve2` 或 `lscpu | grep SVE`。注意 QEMU `-cpu max` 可能模拟出真实硬件不具备的特性，生产代码应以目标硬件的实际能力为准。
</details>

2. **实验 23-1 中 SVE 的 gather/scatter 与 NEON 的 LD3/ST3 相比有什么不同？**

<details>
<summary>答案</summary>

NEON 的 LD3/ST3 是**固定模式的交错加载/存储**——硬件自动按 3 路交织模式分离/合并数据，效率高但只支持固定的交织模式（2/3/4 路）。SVE 的 gather/scatter 是**任意模式的非连续访问**——用索引向量指定每个通道的地址，灵活但延迟高（每通道独立访存）。对 RGB→BGR 来说：NEON LD3/ST3 更高效（固定 3 路交织正是 RGB 需要的），SVE gather/scatter 更灵活但性能可能更差。SVE 的优势在非规则访问模式，不是固定交错。
</details>

## 参考与延伸

- [§23.6 精简要点](06-minimal-knowledge.md) — SVE 最小知识集
- [§23.4 strcmp 优化](04-strcmp-optimization.md) — 实验 23-3 的核心算法
