# Ch 1 量化设计与分析基础 · Fundamentals of Quantitative Design and Analysis

> **Computer Architecture 6th** · Hennessy & Patterson · **选读 🟡**

> 本章定位：**全书量化框架** — 在 Dennard 缩放终结、摩尔定律放缓的背景下，用可度量的方式谈性能、功耗、成本与可靠性，并给出 Amdahl、局部性等设计原则。HFT 不必背每页公式，但要能 **用这些尺子解释「为什么热路径要这样优化」**。

**核心问题：** 当「单靠提频 + ILP」走不通时，架构师和程序员该往哪找性能？如何用 **执行时间** 而非峰值 MIPS 说话？

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1.1–1.2 引言与计算机分类 | [notes/section-1.1-1.2-引言与计算机分类.md](./notes/section-1.1-1.2-引言与计算机分类.md) |
| 1.3 定义计算机体系结构 | [notes/section-1.3-定义计算机体系结构.md](./notes/section-1.3-定义计算机体系结构.md) |
| 1.4–1.6 技术、成本与功耗趋势 | [notes/section-1.4-1.6-技术成本与功耗趋势.md](./notes/section-1.4-1.6-技术成本与功耗趋势.md) |
| 1.7–1.8 可靠性与性能量化 | [notes/section-1.7-1.8-可靠性与性能量化.md](./notes/section-1.7-1.8-可靠性与性能量化.md) |
| 1.9 计算机设计的量化原则 | [notes/section-1.9-计算机设计的量化原则.md](./notes/section-1.9-计算机设计的量化原则.md) |
| 1.10–1.12 综合运用与谬误陷阱 | [notes/section-1.10-1.12-综合运用与谬误陷阱.md](./notes/section-1.10-1.12-综合运用与谬误陷阱.md) |

---

## HFT 精读捷径

| 本节 | 带走什么 |
|------|----------|
| 1.1–1.2 | ILP 红利见顶 → **TLP + 数据局部性 + DSA** 是 HFT 现实路径 |
| 1.4–1.6 | **带宽 ∝ 延迟²** — 网卡/PCIe/内存「吞吐够、尾延迟仍痛」 |
| 1.7–1.8 | 只信 **wall-clock 执行时间**；基准用几何平均，别被峰值忽悠 |
| 1.9 | **Amdahl + 局部性** — profile 找 p，再动 cache line / 绑核 / 旁路 |
| 1.10–1.12 | 性能/功耗/价格三角 — colo 机器选型与压测方法论 |

→ 与 [01-CSAPP Ch1 §1.9 Amdahl](../../01-CSAPP-3rd/chapter-01-tour-of-computer-systems/notes/section-1.9-重要主题-Amdahl与并发与抽象.md) 交叉 · 深入内存 → [Ch 2](../chapter-02-存储器层次结构设计.md)

---

## 相关章节

- 下一章：[chapter-02-存储器层次结构设计](../chapter-02-存储器层次结构设计.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
