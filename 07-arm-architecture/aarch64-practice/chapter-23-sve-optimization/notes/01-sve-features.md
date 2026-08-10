# §23.1 SVE 核心特性

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

SVE（Scalable Vector Extension）是 ARMv8.2-A 引入的可变长向量扩展，支持 128-2048 位向量长度，与 NEON 的固定 128 位不同。软件通过 VQ 查询适配硬件向量长度。

## 核心要点

### SVE 核心特性

| 特性 | 说明 | NEON 对应 |
|------|------|-----------|
| **可变长向量** | 硬件决定向量长度（VQ），软件自适应 | 固定 128 位 |
| **谓词寄存器** | P0-P15，每通道 1 bit 控制执行 | 无（用位掩码模拟） |
| **聚集加载/分散存储** | 非连续内存一次性加载/存储 | 无 |
| **向量长度无关编程** | 同一代码在不同 VQ 上运行 | 硬编码通道数 |
| **寄存器** | Z0-Z31（可变长 128-2048 bit） | V0-V31（固定 128 bit） |

### VQ（Vector Quotient）

```
VQ = 向量长度 / 128

VQ=1  → 128 bit  (同 NEON，Pi5 Cortex-A76)
VQ=2  → 256 bit  (AWS Graviton3+)
VQ=4  → 512 bit  (Fujitsu A64FX)
VQ=8  → 1024 bit (未来)
VQ=16 → 2048 bit (最大规格)

每个 VQ 级别对应的通道数：
VQ    .16b    .8h    .4s    .2d
1     16      8      4      2       (= NEON)
2     32      16     8      4
4     64      32     16     8
```

### 查询向量长度

```c
#include <arm_sve.h>

// 查询向量长度
uint64_t vl_bytes = svcntb();   // 字节数 (16~256)
uint64_t vl_words = svcntw();   // 32位数 (4~64)
uint64_t vl_dbls  = svcntd();   // 64位数 (2~32)

// 运行时适配
printf("SVE vector length: %lu bytes = %lu lanes of int32\n",
       vl_bytes, vl_words);
// Pi5 输出: SVE vector length: 16 bytes = 4 lanes of int32
```

### 向量长度无关编程

```c
// SVE 代码不关心向量长度——while 循环自动适配
void add_one_sve(int32_t *arr, int64_t n) {
    int32_t *ptr = arr;
    int64_t i = 0;

    // svwhilelt 生成 [0, n) 范围的谓词
    svbool_t pg = svwhilelt_b32_s64(i, n);

    while (svptest_first(svptrue_b32(), pg)) {
        // 加载 pg 为 true 的通道
        svint32_t va = svld1_s32(pg, ptr);
        // 只对 pg=true 的通道 +1
        va = svadd_n_s32_x(pg, va, 1);
        // 存储
        svst1_s32(pg, ptr, va);

        ptr += svcntw();  // 前进一个向量长度
        i += svcntw();
        pg = svwhilelt_b32_s64(i, n);  // 更新谓词
    }
}

// 同一份代码：
// - VQ=1 (128bit): 每次处理 4 个 int32
// - VQ=2 (256bit): 每次处理 8 个 int32
// - VQ=4 (512bit): 每次处理 16 个 int32
// 无需重新编译！
```

### SVE 寄存器架构

```
Z0-Z31: 32 个可变长向量寄存器（128-2048 bit）
  ↓ 低 128 位别名
  V0-V31 (NEON 兼容)

P0-P15: 16 个谓词寄存器（每通道 1 bit）
  ↓ 控制每条通道是否执行

FFR: First Fault Register (gather 加载错误标记)

Z 寄存器在 VQ=1 时退化为 NEON 的 V 寄存器
Z 寄存器在 VQ=2 时扩展为 256 位
```

### SVE vs NEON vs x86 AVX

| 特性 | NEON | SVE | x86 AVX2 | x86 AVX-512 |
|------|------|-----|---------|-------------|
| 向量长度 | 128 | 128-2048 | 256 | 512 |
| 谓词 | 无 | P0-P15 | 无 | k0-k7 |
| Gather/Scatter | 无 | 有 | 有 | 有 |
| 长度无关 | 否 | 是 | 否 | 否 |
| 可用性 | 所有 ARM | ARMv8.2+ | x86+ | Xeon Phi+ |

## HFT 关联

SVE 的可变长向量对 HFT 意味着同一份代码在不同 ARM 服务器上自动获得最优性能——在 128 位芯片上跑 4 路并行，在 256 位芯片上自动跑 8 路。

```c
// HFT SVE 未来路径：订单簿批量更新
#include <arm_sve.h>

// 在 VQ=1 上每次处理 4 个价格
// 在 VQ=2 上自动每次处理 8 个价格
void hft_update_prices_sve(float *prices, int64_t n, float delta) {
    int64_t i = 0;
    svbool_t pg = svwhilelt_b32_s64(i, n);
    svfloat32_t dv = svdup_f32(delta);  // delta 广播到所有 lane

    while (svptest_first(svptrue_b32(), pg)) {
        svfloat32_t p = svld1_f32(pg, prices + i);
        p = svadd_f32_x(pg, p, dv);   // 谓词控制 +1
        svst1_f32(pg, prices + i, p);
        i += svcntw();
        pg = svwhilelt_b32_s64(i, n);
    }
}

// 检查 SVE 可用性
int hft_check_sve() {
    // 用户态通过 HWCAP 检查
    FILE *f = fopen("/proc/cpuinfo", "r");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Features")) {
            if (strstr(line, "sve")) return 1;
            if (strstr(line, "sve2")) return 2;
        }
    }
    return 0;  // 无 SVE，用 NEON
}
```

| HFT SVE 评估维度 | 当前 | 未来 |
|-----------------|------|------|
| 部署芯片 | Cortex-A76 (NEON only) | Graviton4 (SVE 256bit) |
| 并行度 | 4×float32 (NEON) | 8×float32 (SVE) |
| Gather/Scatter | 不可用 | 可用（稀疏数据） |
| 工具链 | NEON 成熟 | SVE intrinsics 较新 |

## 自测题

1. **SVE 和 NEON 的根本区别是什么？VQ 是什么？**

<details>
<summary>答案</summary>

根本区别：NEON **固定 128 位**，SVE **可变长**（128-2048 位）。VQ（Vector Quotient）= 向量长度 / 128，表示向量是 128 位的几倍。VQ=1 → 128 位（同 NEON），VQ=2 → 256 位，VQ=4 → 512 位。SVE 的软件用 `svcntb()` 查询向量长度，代码自动适配不同 VQ——这是"向量长度无关编程"的核心。Pi5 Cortex-A76 的 VQ=1。
</details>

2. **"向量长度无关编程"是什么意思？为什么重要？**

<details>
<summary>答案</summary>

"向量长度无关编程"是指**同一份代码**在不同向量长度的硬件上都能正确运行并充分利用硬件宽度。SVE 通过谓词驱动的循环实现：每次迭代处理 `svcntw()` 个元素（当前硬件的向量宽度），谓词 `svwhilelt` 处理尾部不足一个向量的元素。重要性：NEON 代码硬编码 128 位，换到 256 位硬件需要重写。SVE 代码在 128 位芯片上编译一次，在 256 位芯片上自动获得 2 倍吞吐——无需重新编译。
</details>

3. **SVE 的 gather/scatter 指令有什么优势？有什么劣势？**

<details>
<summary>答案</summary>

**优势**：gather/scatter 可以一次性加载/存储**非连续内存**地址的数据（如 `svld1_gather_u64index_s32(pg, base, indices)`），适合稀疏数据访问、哈希表查找等。NEON 没有这个功能，需要逐元素标量加载。**劣势**：每条通道独立访存，硬件需要发起多个 cache line 访问——延迟高、带宽利用率低。如果数据在 cache 中且索引有规律，gather 可能比标量循环快 2-4 倍；如果数据分散在多个 cache line，gather 可能比标量还慢。
</details>

## 参考与延伸

- [§23.2 谓词](02-predicate.md) — SVE 谓词驱动的条件执行
- [§23.3 SVE vs NEON](03-sve-vs-neon.md) — 两者详细对比
