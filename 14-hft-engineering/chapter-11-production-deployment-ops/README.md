# 第11章 实盘上线与运维进阶（索引）

> **上线清单 · 监控 · 故障与降级**

← [chapter-01 实战启动](../chapter-01-hft-fundamentals-ecosystem/1.8-实战启动建议.md) · [chapter-09 测量](../chapter-09-latency-measurement-benchmarking/README.md)

---

## 本章定位

回测赚钱 ≠ 实盘赚钱——中间隔着**运维**。HFT 系统的运维本质：**在故障发生前看到它，在事故扩大前杀掉它**。三大支柱：上线门禁（不达标不上）、监控（看得见）、降级（坏得可控）。

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [11.1](./11.1-上线门禁清单.md) | 上线门禁清单 🔴 | 任何一项不过不上线 |
| [11.2](./11.2-监控与报警.md) | 监控与报警 | 分层采集 + 报症状不报原因 + 静默失败检测 |
| [11.3](./11.3-Kill-Switch分级.md) | Kill Switch 分级 🔴 | L1 停新单 → L4 断链；watchdog 独立于策略进程 |
| [11.4](./11.4-Capture与Replay.md) | Capture / Replay | 时间戳驱动确定性 — 复盘的唯一手段 |
| [11.5](./11.5-故障降级矩阵.md) | 故障降级矩阵 | 每类故障的自动/人工动作 + 双机热备对账 |
| [11.6](./11.6-运维日常.md) | 运维日常 | 变更纪律 · 灰度 · 季度演练 · 复盘文化 |

## 本章小结

| 目标 | 手段 |
|------|------|
| **不上有病的系统** | 上线门禁清单，一项不过不上 |
| **故障可见** | 分层监控 + 静默失败检测 + 独立报警通道 |
| **坏得可控** | Kill Switch 四级 + watchdog 独立于策略进程 |
| **出事可查** | Capture/Replay + 确定性时间戳驱动 |
| **死了有备** | 双机热备 + 升主前强制对账 |

**下一章：** [chapter-12 做市与套利](../chapter-12-market-making-arbitrage/README.md) —— 前面所有工程能力的**盈利出口**。

→ [06.6-Systems-Performance](../../06.6-systems-performance/) · [chapter-10 风控](../chapter-10-risk-compliance-slippage/README.md)
