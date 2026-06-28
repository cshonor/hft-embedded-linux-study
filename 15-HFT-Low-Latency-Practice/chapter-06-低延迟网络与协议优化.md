# 第6章 低延迟网络与协议优化

> **二进制协议 · 零拷贝 · 微波链路**

← 总览：[chapter-01 §5](./chapter-01-高频交易基础与生态.md#5-网络协议与物理传输)

---

## 1. 协议：FIX vs 二进制

| | **FIX（文本）** | **FAST / ITCH / OUCH / SBE** |
|---|----------------|------------------------------|
| 解析 | 分隔符扫描 · 慢 | 固定偏移 / **SBE 模板** · **μs 级** |
| 带宽 | 大 | 紧凑 |
| HFT | 内外网 **控制面** 可能仍用 | **行情+订单热点** 必二进制 |

**Gateway IN：** 按交易所规范 **增量解析** — 字段 **直接 cast** 到 struct（注意 endian / align）。

→ [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/) · [10-PNP](../10-Practical-Network-Programming/)

---

## 2. 内核旁路与零拷贝

见 [chapter-05](./chapter-05-操作系统内核极致调优.md#3-kernel-bypass) — **OpenOnload / DPDK**。

**补充：**

- **UDP 组播** 行情 — 单包 **fan-in** 多订阅
- **TCP 订单** — 会话保序；常 **专用 OUT 线程**

---

## 3. 物理层：微波与空芯光纤

**Latency Arbitrage** 场景（如 **CME ↔ NYSE**）：

| 介质 | 特点 |
|------|------|
| **传统光纤** | 光在玻璃中 **~2/3 c** |
| **微波 / 空芯** | 接近 **空气中 c** — 可快 **数 ms~μs 级**（距离依赖） |

**工程现实：** 容量、天气、许可、成本 — 顶级 HFT **基础设施竞赛**。

---

## 4. 时间同步

- **PTP（IEEE 1588）** — 跨机 **sub-μs** 时钟
- **硬件打戳** — NIC **PHC** 优于 `clock_gettime` 软件路径

→ [chapter-10 测量](./chapter-10-延迟测量与基准压测.md)
