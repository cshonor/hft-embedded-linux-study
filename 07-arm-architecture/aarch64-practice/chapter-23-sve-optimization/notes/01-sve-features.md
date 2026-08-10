# §23.1 SVE 核心特性

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

SVE（Scalable Vector Extension）是 ARMv8.2-A 引入的可变长向量扩展，支持 128-2048 位向量长度，与 NEON 的固定 128 位不同。软件通过 VQ 查询适配硬件向量长度。

## 核心要点

### SVE 核心特性

| 特性 | 说明 |
|------|------|
| **可变长向量** | 硬件决定向量长度（VQ），软件自适应 |
| **谓词寄存器** | P0-P15，16 个谓词寄存器，控制每条通道是否执行 |
| **聚集加载/分散存储** | 非连续内存一次性加载/存储（gather/scatter） |
| **向量长度无关编程** | 同一代码在不同 VQ 上运行 |

### VQ（Vector Quotient）

```
VQ = 向量长度 / 128

VQ=1  → 128 bit  (同 NEON)
VQ=2  → 256 bit
VQ=4  → 512 bit
...
VQ=16 → 2048 bit  (最大)
```

### 查询向量长度

```c
#include <arm_sve.h>
uint64_t vl = svcntb();  // 向量中的字节数（128/8=16 ~ 2048/8=256）
```

> Pi5 Cortex-A76 支持 SVE2，VQ=1（128 位）。服务器芯片（如 AWS Graviton4）可能支持 VQ=2（256 位）。

### 向量长度无关编程

```c
// SVE 代码不关心向量长度——while 循环自动适配
svbool_t pg = svwhilelt_b32_u64(0, n);  // 生成 [0,n) 范围的谓词
while (svptest_first(svptrue_b32(), pg)) {
    svint32_t va = svld1_s32(pg, ptr);   // 加载 pg 为 true 的通道
    svst1_s32(pg, ptr, svadd_n_s32_x(pg, va, 1));  // +1 并存储
    ptr += svcntw();
    pg = svwhilelt_b32_u64(ptr - base, n);
}
```

## HFT 关联

SVE 的可变长向量对 HFT 意味着同一份代码在不同 ARM 服务器上自动获得最优性能——在 128 位芯片上跑 4 路并行，在 256 位芯片上自动跑 8 路。Gather/Scatter 指令对稀疏数据访问（如按索引查询订单簿中的特定价位）有潜力，但 gather 操作通常延迟较高（每通道独立访存）。当前 HFT 主要用 NEON（所有 ARM 核心都支持），SVE 作为未来升级路径。

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
