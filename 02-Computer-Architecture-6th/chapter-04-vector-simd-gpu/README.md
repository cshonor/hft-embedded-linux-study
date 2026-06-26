# Ch 4 向量、SIMD 与 GPU 架构中的数据级并行 · Data-Level Parallelism in Vector, SIMD, and GPU Architectures

> **Computer Architecture 6th** · Hennessy & Patterson · **跳过 ⚪**（HFT 热路径默认不押 DLP/GPU）

> 本章定位：**单指令多数据（SIMD）** 开发 **数据级并行（DLP）** — 向量架构、x86 AVX 类扩展、GPU/CUDA 三条路线。科学计算与 ML 核心；对 HFT 而言，价值主要在 **批量回放/风控/研究**，而非 tick-to-trade 主链。

**核心问题：** 同一操作能否施加于 **一整块数据**？瓶颈在 **算力屋顶** 还是 **内存带宽屋顶**（Roofline）？

```
DLP 三变体：向量架构 (RVV) → 多媒体 SIMD (SSE/AVX) → GPU (SIMT/CUDA)
编译器/手写向量化 ← 循环携带依赖、掩码、步幅/收集-散布
```

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 4.1–4.2 向量架构 | [notes/section-4.1-4.2-向量架构.md](./notes/section-4.1-4.2-向量架构.md) |
| 4.3 多媒体 SIMD 扩展 | [notes/section-4.3-多媒体SIMD扩展.md](./notes/section-4.3-多媒体SIMD扩展.md) |
| 4.4 GPU 架构 | [notes/section-4.4-GPU架构.md](./notes/section-4.4-GPU架构.md) |
| 4.5 循环级并行检测 | [notes/section-4.5-循环级并行检测.md](./notes/section-4.5-循环级并行检测.md) |
| 4.6–4.8 交叉问题、实例与陷阱 | [notes/section-4.6-4.8-交叉问题实例与陷阱.md](./notes/section-4.6-4.8-交叉问题实例与陷阱.md) |

---

## HFT 精读捷径（场景触发）

| 场景 | 读本节 |
|------|--------|
| 批量解码/fix checksum/固定宽度字段 | 4.2–4.3 **AVX**、条带挖掘、掩码 |
| 回测矩阵、大规模特征工程 | 4.3 **Roofline**、算术强度 |
| ML 信号训练/推理（非实盘热路径） | 4.4 **GPU**、合并访存 |
| 判断某循环能否向量化 | 4.5 **循环携带依赖**、GCD 测试 |

**默认跳过：** 实盘 tick-to-trade 以 **低延迟串行热路径 + Ch2/Ch3** 为主；GPU 极少上关键路径。

→ [Ch1 DLP 引言](../chapter-01-quantitative-design-fundamentals/notes/section-1.1-1.2-引言与计算机分类.md) · [Ch3 ILP](../chapter-03-instruction-level-parallelism/) · [Ch7 DSA](../chapter-07-特定领域架构.md)

---

## 相关章节

- 上一章：[chapter-03-instruction-level-parallelism](../chapter-03-instruction-level-parallelism/)
- 下一章：[chapter-05-线程级并行](../chapter-05-线程级并行.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
