# 第3章 交易所动态与订单簿（索引）

> **原书第 3 章 · Understanding the Trading Exchange Dynamics**
> **撮合引擎 · FIFO/Pro-rata · 共址 · 本地 Book 启示**

← [chapter-02 关键组件](../chapter-02-exchange-architecture-matching/README.md) · 总览：[chapter-01](../chapter-01-hft-fundamentals-ecosystem/README.md)

---

## 本章定位

第二章构建 **我方交易系统**；第三章转向 **市场另一端——交易所**。

在 μs/ns 竞争中，**微观结构（Microstructure）** 与 **撮合规则** 是策略能否盈利的 **绝对前提** — 决定你是 **抢队列** 还是 **堆量 Pro-rata**。

→ 理论深化：[19-markets-microstructure](../../19-markets-microstructure/)

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [3.1](./3.1-交易所核心功能与订单路径.md) | 交易所核心功能与订单路径 | Router → FIFO 队列 → 撮合引擎 |
| [3.2](./3.2-撮合引擎三种场景.md) | 撮合引擎三种场景 | Best Price / Partial Fill / No Match |
| [3.3](./3.3-同价匹配算法.md) | 同价匹配算法 🔴 | FIFO vs Pro-rata 决定策略形态 |
| [3.4](./3.4-共址与市场数据.md) | 共址与市场数据 | 1 ns 也是执行优势 |
| [3.5](./3.5-本地BookBuilder与行情解析.md) | 本地 Book Builder 与行情解析 | O(1) + gap 恢复 + 归一化 |

## 本章小结

| 交易所侧 | 对你系统的含义 |
|----------|----------------|
| **Matching Engine** | 理解 **Fill / Rest** 三种场景 |
| **FIFO vs Pro-rata** | 决定 **延迟 vs 尺寸** 策略 |
| **Amend 丢优先** | 少改单 · 优化 **Gateway OUT** |
| **Co-location** | **Gateway IN/OUT** 物理布局 |

**下一步：** [chapter-04 硬件到 OS](../chapter-04-hardware-selection-server-config/README.md) · [chapter-05 OS 调优](../chapter-05-os-kernel-tuning/README.md)
