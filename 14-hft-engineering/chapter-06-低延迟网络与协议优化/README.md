# 第6章 低延迟网络与协议优化（索引）

> **原书第 5 章 · Networking in Motion**
> **NIC · 交换机 · TCP/UDP · 二进制协议 · 包生命周期 · PTP**

← 硬件/OS 原理：[chapter-04](../chapter-04-硬件选型与服务器配置/README.md) · Bypass 实操：[chapter-05](../chapter-05-操作系统内核极致调优/README.md)

---

## 本章定位

网络是 HFT 的 **生死赛道** — 决定 **ms vs μs** 竞争胜负。

本章追踪数据包：**交易所 → 交换机 → NIC → OS/用户态 → Strategy**，并说明为何必须 **直通交换机、二进制协议、UDP 组播、Kernel Bypass、PTP**。

→ 深化：[13-DPDK](../../../13-dpdk/) · [12-PNP](../../../04-cpp/M5-cpp-network-programming/)

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [6.1](./6.1-网络硬件与交换机.md) | 网络硬件与交换机 🔴 | Cut-through / L1 Switch — 机柜内 μs 从选型开始 |
| [6.2](./6.2-TCP与UDP.md) | TCP 与 UDP | 订单走 TCP 会话，行情走 UDP 组播 |
| [6.3](./6.3-金融应用层协议.md) | 金融应用层协议 | ITCH/OUCH/SBE 固定布局 μs 解析，FIX 只配控制面 |
| [6.4](./6.4-数据包生命周期.md) | 数据包生命周期 🔴 | 内核路径六步 μs 级累积 — Bypass 的动机 |
| [6.5](./6.5-监控与时间同步.md) | 监控与时间同步 | Passive TAP 带外抓包 + PTP ns 级打戳 |
| [6.6](./6.6-广域链路.md) | 广域链路 | 微波/空芯光纤 — 跨所延迟套利的基础设施 |

## 本章小结

| 要赢 μs 级 | 行动 |
|------------|------|
| **机房内** | Cut-through / L1 switch · 共址 |
| **协议** | 行情 **UDP multicast** · 订单 TCP/UDP · **ITCH/OUCH/SBE** |
| **主机** | Bypass **跳过 6.4 内核路径** |
| **测量** | **Passive TAP + PTP** |

**下一步（原书 Ch6 ≈ 软件压榨）：** [chapter-07 无锁与内存](../chapter-07-无锁数据结构与内存布局/README.md) · [chapter-05 OS 绑核](../chapter-05-操作系统内核极致调优/README.md)

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch5 Networking in Motion | **本章 Ch6** |
| Ch6 HFT 优化（架构/OS） | **Ch5 + Ch7 + Ch8** |
