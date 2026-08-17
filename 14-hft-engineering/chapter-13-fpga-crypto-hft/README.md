# 第13章 高频 FPGA 与加密货币（索引）

> **原书第 11 章 · High-Frequency FPGA and Crypto**
> **纳秒级硬件加速 · 加密市场微观结构 · 云端共址**

← [chapter-12 策略](../chapter-12-market-making-arbitrage/README.md) · [chapter-04 硬件基础](../chapter-04-hardware-selection-server-config/README.md) · [chapter-03 共址](../chapter-03-orderbook-depth-market-data/3.4-共址与市场数据.md)

---

## 本章定位

软件优化（OS Bypass · C++ 关键路径）可将 T2T 压至 **几 μs**。当 **μs 仍不够** 时，原书 **Ch11** 给出两条延伸：

1. **FPGA** — T2T **<500 ns** 的硬件军备
2. **Crypto** — 传统 HFT 技术进入 **7×24 数字资产** 战场

| 主题 | 本章 | 交叉 |
|------|------|------|
| FPGA 原理速览 | **13.1** | [Ch4 §4](../chapter-04-hardware-selection-server-config/4.4-硬件选型速查.md) · [Ch1 §7](../chapter-01-hft-fundamentals-ecosystem/1.7-FPGA纳秒级.md) |
| 传统共址 | — | [Ch3 §4](../chapter-03-orderbook-depth-market-data/3.4-共址与市场数据.md) |
| 云端共址 | **13.3** | [Ch6 网络](../chapter-06-low-latency-network-protocol/README.md) |

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [13.1](./13.1-FPGA硬件加速.md) | FPGA 硬件加速 🔴 | 并行 + 确定性 + 无 OS，T2T < 500ns |
| [13.2](./13.2-加密货币高频.md) | 加密货币高频 | 7×24 · 永续资金费率 · CEX 主战场 |
| [13.3](./13.3-云端共址.md) | 云端共址 | 同 Region/AZ — 空间换时间 |
| [13.4](./13.4-技术栈总览.md) | 原书技术栈总览 | Ch1–11 完整链路：从零到 ns |

## 本章小结

| 原书 Ch11 主题 | 要点 |
|----------------|------|
| **FPGA** | 并行 · 确定性 · 网卡内协议/解析/执行 · **<500 ns** · 贵/难/风控风险 |
| **Crypto** | 7×24 · 永续+资金费率 · **CEX 主战场** |
| **云端** | **同 Region/AZ** · IaaS 极速 vs 弹性 ML |

**原书正文收官** — 本仓库 **Ch10 风控 · Ch11 运维 · Ch12 策略** 为工程扩展，可继续深化。

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch11 §1 FPGA | **本章 13.1** · Ch4 · Ch1 §7 |
| Ch11 §2 Crypto | **本章 13.2** |
| Ch11 §3 云端 | **本章 13.3** |
| 风控合规（扩展） | **Ch10** |
| 实盘运维（扩展） | **Ch11** |
| 做市套利（扩展） | **Ch12** |

## FPGA / Crypto 速查

| Do | Don't |
|----|-------|
| FPGA 做 **窄而确定** 的路径 | 把复杂 ML 硬塞 FPGA |
| **形式化验证 / 仿真** 后再上实盘 | 忽视 FPGA Bug 的 **纳秒级放大** |
| Crypto HFT 选 **CEX + 同 AZ** | 在 DEX 上做 μs 策略 |
| 永续 **资金费率** 纳入套利模型 | 假设 crypto = 传统期货规则 |
