# Ch23 完整总结 · 可伸缩矢量计算与优化（SVE/SVE2）

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **跳过**（首遍可略）  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

SVE（Scalable Vector Extension）是 ARMv9 的可变长向量扩展。与 NEON 的固定 128 位不同，SVE 支持从 128 到 2048 位的可变长度。首遍可略，有 NEON 基础后再学。

---

## 23.1 SVE 核心特性

| 特性 | 说明 |
|------|------|
| **可变长向量** | 硬件决定向量长度（VQ），软件自适应 |
| **谓词寄存器** | P0-P15，16 个谓词寄存器，控制每条通道是否执行 |
| **聚集加载/分散存储** | 非连续内存一次性加载/存储 |
| **向量长度无关编程** | 同一代码在不同 VQ 上运行 |

### VQ（Vector Quotient）

```
VQ = 向量长度 / 128

VQ=1 → 128 bit (同 NEON)
VQ=2 → 256 bit
VQ=4 → 512 bit
...
VQ=16 → 2048 bit (最大)
```

```c
// 查询向量长度
#include <arm_sve.h>
uint64_t vl = svcntb();  // 向量中的字节数（128/8=16 ~ 2048/8=256）
```

> Pi5 Cortex-A76 支持 SVE2，VQ=1（128 位）。

---

## 23.2 谓词（Predicate）⭐

SVE 的核心创新——**每条通道可条件执行**：

```c
// SVE：用谓词控制哪些通道参与计算
svbool_t pg = svcmpgt_n_s32(svptrue_b32(), va, 0);  // pg = (va > 0)
svint32_t result = svadd_n_s32_x(pg, va, 1);        // 只对 pg=true 的通道 +1
```

```
谓词 P: | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0 |
向量 A: | a0| a1| a2| a3| a4| a5| a6| a7|
ADD  : |a0+1| |a2+1|a3+1| |a5+1| | |
结果:  | a0'|a1| a2'|a3'|a4| a5'|a6|a7|  ← 只有 pg=1 的通道被修改
```

> NEON 没有谓词，需要用 branch 或位掩码模拟，效率低。

---

## 23.3 SVE vs NEON

| 特性 | NEON | SVE |
|------|------|-----|
| 向量长度 | 固定 128 位 | 可变（128-2048） |
| 谓词 | 无 | P0-P15 |
| 聚集加载 | 无 | 有（gather/scatter） |
| 条件执行 | branch 或位运算 | 谓词 |
| 适用 | 图像/音频/矩阵 | HPC/科学计算/AI |
| 可用性 | ARMv7+ | ARMv8.2-A SVE / ARMv9 SVE2 |

---

## 23.4 实用示例：strcmp 优化

```c
// SVE strcmp（简化概念）
svbool_t pg = svptrue_b8();
while (1) {
    svuint8_t va = svld1_u8(pg, ptr_a);   // 加载 a
    svuint8_t vb = svld1_u8(pg, ptr_b);   // 加载 b
    svbool_t eq = svcmpeq_u8(pg, va, vb);  // 比较
    svbool_t za = svcmpeq_n_u8(pg, va, 0); // a 是否有 '\0'
    if (!svptest_first(pg, za)) {
        // 有 '\0' 且全部相等 → 返回 0
        return 0;
    }
    if (!svptest_first(za, eq)) {
        // 不等 → 返回差值
        // ...
    }
    ptr_a += svcntb();
    ptr_b += svcntb();
}
```

> SVE 一次比较整条向量，不用逐字节循环。

---

## 23.5 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 23-1 | RGB24→BGR32（SVE 优化） | SVE |
| 23-2 | 8×8 矩阵乘法运算 | SVE |
| 23-3 | 用 SVE 优化 strcpy() | SVE |

> SVE 实验可在 QEMU+ARM64 Linux 上完成。Pi5 的 Cortex-A76 支持 SVE2。

---

## 23.6 精简要点（跳过级别的最小知识）

1. **SVE = 可变长 + 谓词** → 同一代码适配不同向量长度
2. **谓词 P0-P15** → 每通道条件执行，无需 branch
3. **SVE2 = SVE + NEON 功能合并** → ARMv9 标准
4. **首遍跳过** → 有 NEON 基础后回来学

---

## 书中思考题（自测）

1. SVE 和 NEON 的最大区别是什么？
2. 谓词寄存器（P0-P15）的作用？
3. VQ 是什么？Pi5 的 VQ 是多少？
4. SVE 为什么适合 HPC/科学计算？
5. SVE2 相比 SVE 增加了什么？

**参考答案：**

1. SVE **向量长度可变**（128-2048），有**谓词**；NEON 固定 128 位，无谓词。  
2. 控制每条通道是否执行，实现**无 branch 条件计算**。  
3. VQ=向量长度/128。Pi5 Cortex-A76 的 VQ=**1**（128 位）。  
4. 可变长适应不同硬件、谓词减少分支、聚集加载适合非连续数据。  
5. SVE2 = SVE + **合并了 NEON 的功能**（更多数据处理指令），是 ARMv9 标准。

---

上一章 [Ch22 NEON](../../chapter-22-fp-neon/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
