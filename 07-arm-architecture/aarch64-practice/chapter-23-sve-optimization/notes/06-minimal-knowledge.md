# §23.6 精简要点（跳过级别的最小知识）

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

首遍跳过 SVE 时需要记住的最小知识集——4 条核心概念，确保有 NEON 基础后回来学时有锚点。

## 核心要点

### SVE 最小知识集

| # | 要点 | 一句话 | 详细参考 |
|---|------|--------|---------|
| 1 | SVE = 可变长 + 谓词 | 同一代码适配不同向量长度 | §23.1 |
| 2 | 谓词 P0-P15 | 每通道条件执行，无需 branch | §23.2 |
| 3 | SVE2 = SVE + NEON 合并 | ARMv9 标准 SIMD | §23.3 |
| 4 | 首遍跳过 | 有 NEON 基础后回来学 | — |

### SVE vs NEON 速查表

| 维度 | NEON | SVE |
|------|------|-----|
| 向量长度 | 固定 128 | 可变 128-2048 |
| 寄存器 | V0-V31 (128b) | Z0-Z31 (可变) |
| 谓词 | 无 | P0-P15 |
| Gather/Scatter | 无 | 有 |
| 条件执行 | branch/位运算 | 谓词 |
| 尾部处理 | 手动 | 自动（svwhilelt） |
| 架构要求 | ARMv7+ | ARMv8.2+ |
| 可用性 | 所有 ARM | 部分 ARM |
| 编程模型 | 硬编码通道数 | VQ 无关 |
| C 头文件 | `<arm_neon.h>` | `<arm_sve.h>` |

### SVE 核心 intrinsic 速记

```c
#include <arm_sve.h>

// 查询向量长度
uint64_t bytes = svcntb();   // 字节数
uint64_t words = svcntw();   // 32位数

// 谓词生成
svbool_t all = svptrue_b32();              // 全 true
svbool_t pg = svwhilelt_b32_s64(0, n);     // 前缀 true

// 加载/存储
svfloat32_t v = svld1_f32(pg, ptr);        // 谓词加载
svst1_f32(pg, ptr, v);                      // 谓词存储

// 运算（_x/_z/_m 后缀控制 inactive）
svfloat32_t r = svadd_f32_x(pg, a, b);     // 加法
svfloat32_t r = svmul_f32_x(pg, a, b);     // 乘法
svfloat32_t r = svmla_f32_x(pg, acc, a, b); // 乘加

// 比较生成谓词
svbool_t gt = svcmpgt_f32(pg, a, b);       // a > b
svbool_t eq = svcmpeq_f32(pg, a, b);       // a == b

// 谓词测试
bool any = svptest_first(svptrue_b32(), pg); // 有 true?
```

### 为什么首遍可以跳过

1. **NEON 足够入门**——128 位固定向量，概念简单，工具链成熟
2. **SVE 硬件不普及**——目前只有少数 ARMv8.2-A+ / ARMv9 芯片支持
3. **概念跨度大**——谓词、VQ 无关编程、gather/scatter 是新概念
4. **实际项目用 NEON**——当前 ARM 平台的 SIMD 代码以 NEON 为主

### 回来学 SVE 的前置条件

- 熟练掌握 NEON intrinsics 和汇编
- 理解矩阵乘法等典型 NEON 优化模式
- 有 SVE 硬件或 QEMU `-cpu max` 环境可用

### SVE 学习路径建议

```
第1步：掌握 NEON（Ch22）
  ├─ 浮点寄存器层级
  ├─ SIMD 通道拆分
  ├─ 常用指令（LD1/FMLA/TBL）
  ├─ intrinsics 编程
  └─ 矩阵乘法实战

第2步：理解 SVE 概念（本章，可首遍跳过）
  ├─ 可变长向量 + VQ
  ├─ 谓词驱动条件执行
  ├─ 向量长度无关编程
  └─ gather/scatter

第3步：SVE 实践（有硬件时）
  ├─ QEMU -cpu max 环境
  ├─ RGB→BGR SVE 版本
  ├─ strcmp/strcpy SVE 优化
  └─ 与 NEON benchmark 对比
```

### NEON → SVE 代码迁移对照

```c
// 常见 NEON → SVE 迁移模式

// 1. 加载
// NEON:  float32x4_t a = vld1q_f32(ptr);
// SVE:   svfloat32_t a = svld1_f32(svptrue_b32(), ptr);

// 2. 加法
// NEON:  float32x4_t c = vaddq_f32(a, b);
// SVE:   svfloat32_t c = svadd_f32_x(svptrue_b32(), a, b);

// 3. 乘加
// NEON:  float32x4_t r = vfmaq_f32(acc, a, b);
// SVE:   svfloat32_t r = svmla_f32_x(svptrue_b32(), acc, a, b);

// 4. 条件选择
// NEON:  result = vbslq_f32(mask, a, b);
// SVE:   result = svsel_f32(mask, a, b);  // 更直观

// 5. 水平加
// NEON:  float sum = vaddvq_f32(v);
// SVE:   float sum = svaddv_f32(svptrue_b32(), v);

// 6. 循环（最大区别）
// NEON:  固定步长 4 + 手动尾部处理
// SVE:   步长 svcntw() + svwhilelt 自动尾部
```

## HFT 关联

HFT 开发者的 SVE 知识需求取决于部署平台：如果部署在 Cortex-A72/A76（只有 NEON），SVE 可跳过；如果未来迁移到 AWS Graviton（支持 SVE）或 ARMv9 服务器，需要回来学。

```c
// HFT SIMD 能力检测与选择
void hft_simd_init() {
    int sve = hft_check_sve();

    if (sve >= 2) {
        printf("Platform: ARMv9 SVE2, VL=%lu bytes\n", svcntb());
        // 使用 SVE2 代码路径
    } else if (sve >= 1) {
        printf("Platform: SVE, VL=%lu bytes\n", svcntb());
        // 使用 SVE 代码路径
    } else {
        printf("Platform: NEON (128-bit fixed)\n");
        // 使用 NEON 代码路径
    }
}

// HFT SVE 迁移决策
// 1. 当前部署 Cortex-A76 → 用 NEON（SVE 可跳过）
// 2. 评估 Graviton4 → SVE VQ=2，理论 2× 加速
// 3. 算法不变，只需重写 intrinsics（arm_neon.h → arm_sve.h）
// 4. 关键收益：谓词过滤（订单簿批量条件检查）
// 5. 关键风险：gather/scatter 延迟高（需 benchmark）
```

## 自测题

1. **SVE 和 NEON 的最大区别用一个词概括是什么？**

<details>
<summary>答案</summary>

**可变长**（Vector-Length Agnostic）。NEON 固定 128 位，SVE 支持 128-2048 位可变长。软件用 `svcntb()` 查询向量长度，代码自动适配不同硬件——这是 SVE 与 NEON 最本质的区别。其他区别（谓词、gather/scatter）都是建立在这个基础上的功能扩展。
</details>

2. **SVE2 相比 SVE 增加了什么？为什么 ARMv9 选 SVE2 作为标准？**

<details>
<summary>答案</summary>

SVE2 = SVE + **NEON 的数据处理功能**（TBL 查表、复数运算、AES/SHA 密码学等）。ARMv9 选 SVE2 作为标准因为：(1) SVE2 包含 NEON 的全部功能——向后兼容现有 NEON 代码；(2) SVE2 继承 SVE 的可变长和谓词——面向未来更宽的向量；(3) 统一 SIMD 编程模型——ARMv9 开发者只需学 SVE2，不需要同时维护 NEON 和 SVE 两套代码。ARMv9 芯片支持 SVE2（含 NEON 兼容模式）。
</details>

3. **首遍跳过 SVE 后，回来学的前置条件是什么？**

<details>
<summary>答案</summary>

三个前置条件：(1) **熟练掌握 NEON**——intrinsics 和汇编，能写矩阵乘法等典型优化代码；(2) **理解 NEON 优化的模式**——分块、向量化、寄存器分配、尾部处理；(3) **有 SVE 环境**——SVE 硬件（如 Pi5 Cortex-A76）或 QEMU `-cpu max` 模拟。有了 NEON 基础，SVE 的学习曲线主要在新概念：谓词驱动的条件执行、VQ 无关编程模型、gather/scatter 访存模式。这些概念不难，但需要实践才能掌握。
</details>

## 参考与延伸

- [§23.1 SVE 核心特性](01-sve-features.md) — SVE 完整特性
- [§23.3 SVE vs NEON](03-sve-vs-neon.md) — 何时用 SVE 何时用 NEON
- [Ch22 NEON 指令](../../chapter-22-fp-neon/notes/section-0-本章完整概述.md) — SVE 的前置基础
