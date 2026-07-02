# Ch 12 基准测试 · Benchmarking

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**基准测试「出人意料地棘手」** — 跑分高 ≠ 生产快。Gregg 不罗列工具了事，而是教 **如何设计实验、控制变量、结合观测、拷问报告**。Ch 8/9/10 各章的 fio/iperf 微基准，必须在本章方法论框架下解读。  
> **HFT：** 微观基准（fio、iperf）只做 **capacity baseline**；策略与端到端延迟靠 **生产级 replay + 应用 span**（→ [12-HFT ch10](../../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/)）。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 12.1 基准测试的背景与挑战 | [notes/section-12.1-基准测试的背景与挑战.md](./notes/section-12.1-基准测试的背景与挑战.md) |
| 12.2 基准测试的类型 | [notes/section-12.2-基准测试的类型.md](./notes/section-12.2-基准测试的类型.md) |
| 12.3 基准测试方法论 | [notes/section-12.3-基准测试方法论.md](./notes/section-12.3-基准测试方法论.md) |
| 12.4 基准测试拷问（Benchmark Questions） | [notes/section-12.4-基准测试拷问Benchmark-Questions.md](./notes/section-12.4-基准测试拷问Benchmark-Questions.md) |

---

## 大白话 · 本章就四件事

> **基准测试是实验，不是电竞跑分。**

**① 为什么要测 + 为什么常失败 — 先懂局限再动手。**

- 有效基准 = 可重复、可解释、**与生产 workload 相关**。
- 失败模式：测错层、测到 cache、样本太少、环境不一致、结论过度推广。

**② 四种类型：Micro / Simulation / Replay / Industry Standard。**

- **Micro** — 单组件（CPU/盘/网）；**Replay** — 录生产再播（架构变了可能误导）。
- **Macro 行业标准** — SPEC 等；HFT 更常 **自定义 + 真实报文 replay**。

**③ 方法论：主动/被动、USE+Profile、阶梯负载、Sanity Check、统计、Checklist。**

- 压测时 **同时** CPU 火焰图 + USE — 确认瓶颈在声称的那一层。
- **Ramping load** 找拐点；**Sanity Check** 交叉验证工具是否在测你以为的东西。

**④ 拷问报告（Benchmark Questions）— 看 vendor 亮分保持批判。**

- 工作负载是什么？预热了吗？P99 呢？与你们生产像吗？

下面按原书 12.1–12.4 展开。

---

## HFT 工具与场景对照

| 场景 | 推荐类型 | 工具/方式 |
|------|----------|-----------|
| 日志盘验收 | Micro | fio `direct=1`（Ch 8/9） |
| 网络带宽 baseline | Micro | iperf3（Ch 10） |
| 网卡 PPS 上限 | Micro | testpmd / pktgen |
| 策略回归 | Simulation / Replay | 历史 tick replay + P99 |
| 端到端 SLA | Custom + Passive | span timestamp（ch10） |
| 整机采购 | Macro + Micro | 行业标准 + 自有 replay |

---

## 本章 Checklist

- [ ] 能说清 **四种基准类型** 及各自陷阱
- [ ] 主动压测时 **同时 USE + profile**
- [ ] 磁盘/FS 测试考虑 **WSS 与 drop_caches**
- [ ] 报告 **P99** 而非仅 mean
- [ ] 会用 **12.4 拷问清单** 读 vendor 报告
- [ ] HFT 区分 **micro baseline** vs **生产 replay**

---

## HFT 精读捷径（Ch 12 在路线中的位置）

```
Ch 2  统计、延迟分解
Ch 8–10  fio / iperf 工具（各章微基准）
Ch 12  基准方法论（本章：如何测、如何信）
  → Ch 13 perf（压测时 profile）
  → 12-HFT ch10（量化端到端压测落地）
```

**本章最小行动集：**

1. 为 **日志 NVMe** 写一份 **fio job 文件** + 环境说明 — 存档可复现。
2. 对策略 replay 报 **P50/P99/P999** — 不只 average tick time。
3. 读任意一份 NIC 厂商 benchmark — 用 **12.4 问题** 标红 3 个缺失项。

**Gregg 本章金句（HFT 版）：**

> 基准测试 **出人意料地棘手** — 「跑分很高、生产很差」是常态，不是意外。  
> **Micro 测容量，Replay 测行为，Profile 证明瓶颈** — 三者缺一不可。

---

## 相关章节

- 上一章：[../chapter-11-cloud-computing/](../chapter-11-cloud-computing/)
- 下一章：[../chapter-13-perf/](../chapter-13-perf/)
- 方法论：[../chapter-02-methodologies/](../chapter-02-methodologies/)
- fio / FS：[../chapter-08-file-systems/](../chapter-08-file-systems/)
- fio / 磁盘：[../chapter-09-disks/](../chapter-09-disks/)
- iperf / 网络：[../chapter-10-network/](../chapter-10-network/)
- HFT 压测：[12-HFT ch10](../../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
