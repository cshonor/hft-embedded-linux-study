# Ch 7 特定领域架构 · Domain-Specific Architectures

> **Computer Architecture 6th** · Hennessy & Patterson · **跳过 ⚪**（HFT 场景触发读）

> 本章定位：**摩尔/Dennard 放缓后** — 通用 CPU 单线程见顶，用 **DSA** 对某一领域（本章以 **DNN、图像** 为主）做极致面积/能效优化，其余交给通用核。HFT 关联：**FPGA/SmartNIC 行情解析、固定格式解码、Roofline 判瓶颈**，而非训练 TPU 本身。

**核心问题：** 何时值得 **砍掉通用性**？DSA 的五条原则如何映射到 **脉动阵列、scratchpad、窄类型、DSL**？

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 7.1–7.2 DSA 设计原则 | [notes/section-7.1-7.2-DSA设计原则.md](./notes/section-7.1-7.2-DSA设计原则.md) |
| 7.3 典型领域：DNN | [notes/section-7.3-深度神经网络领域.md](./notes/section-7.3-深度神经网络领域.md) |
| 7.4 Google TPU | [notes/section-7.4-Google-TPU.md](./notes/section-7.4-Google-TPU.md) |
| 7.5 Microsoft Catapult | [notes/section-7.5-Microsoft-Catapult.md](./notes/section-7.5-Microsoft-Catapult.md) |
| 7.6 Intel Crest | [notes/section-7.6-Intel-Crest.md](./notes/section-7.6-Intel-Crest.md) |
| 7.7 Pixel Visual Core | [notes/section-7.7-Pixel-Visual-Core.md](./notes/section-7.7-Pixel-Visual-Core.md) |
| 7.8–7.11 综合对比与陷阱 | [notes/section-7.8-7.11-综合对比与陷阱.md](./notes/section-7.8-7.11-综合对比与陷阱.md) |

---

## HFT 精读捷径（场景触发）

| 场景 | 读本节 |
|------|--------|
| 理解 **FPGA/ASIC 为何能赢 CPU** | 7.1–7.2 五条原则 |
| ML 信号 **推理** vs 训练分工 | 7.3、7.4 TPU、7.6 Crest |
| **网卡旁路/inline 加速** 架构直觉 | 7.5 Catapult bump-in-the-wire |
| **Roofline 判算力/带宽** | 7.8–7.11、Ch4 Roofline |

**默认：** 实盘 tick 路径仍以 **通用 CPU + DPDK** 为主；DSA 多见于 **行情预处理、风控 ML、研究**。

→ [Ch1 Dennard/DSA](../chapter-01-quantitative-design-fundamentals/notes/section-1.1-1.2-引言与计算机分类.md) · [Ch4 GPU/SIMD](../chapter-04-vector-simd-gpu/) · [14-DPDK](../../../14-DPDK-Low-Latency-Network/)

---

## 相关章节

- 上一章：[chapter-06-warehouse-scale-computers](../chapter-06-warehouse-scale-computers/)
- 附录：[appendix-A-指令集原理](../appendix-A-指令集原理.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
