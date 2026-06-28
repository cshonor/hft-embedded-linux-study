# 第6章 动态网络

> **原书第 5 章 · Networking in Motion**（本仓库 **Ch6**；原书 Ch4 OS → 本仓库 **Ch4–5**）  
> **NIC · 交换机 · TCP/UDP · 二进制协议 · 包生命周期 · PTP**

← [chapter-04 硬件/OS 原理](./chapter-04-硬件选型与服务器配置.md) · Bypass 实操：[chapter-05](./chapter-05-操作系统内核极致调优.md)

---

## 本章定位

网络是 HFT 的 **生死赛道** — 决定 **ms vs μs** 竞争胜负。

本章追踪数据包：**交易所 → 交换机 → NIC → OS/用户态 → Strategy**，并说明为何必须 **直通交换机、二进制协议、UDP 组播、Kernel Bypass、PTP**。

→ 深化：[14-DPDK](../14-DPDK-Low-Latency-Network/) · [10-PNP](../10-Practical-Network-Programming/)

---

## 1. 网络基础与硬件

HFT 主要关注 **OSI 底层 + 传输层**：物理 · 数据链路 · 网络(IPv4) · 传输(TCP/UDP)。

| 设备 | 作用 | HFT 关注点 |
|------|------|-----------|
| **NIC** | 主机↔外界 | **PCIe 代际** · **10/25/100G** · **光纤口** · **硬件时间戳** |
| **Switch** | LAN 转发 | **Cut-through** · **HOL 阻塞** · 端口延迟 |

**NUMA：** NIC 与 **收包线程同 node** → [chapter-04 §1](./chapter-04-硬件选型与服务器配置.md#1-硬件商用服务器榨干物理极限)

---

## 2. 交换机转发模式（延迟关键）

出口拥堵 → **排队延迟** · **丢包** · **HOL（Head-of-Line）阻塞**。

| 模式 | 行为 | HFT |
|------|------|-----|
| **Store-and-forward** | 收 **整帧** + **CRC** 后转发 | **慢** — 不适合 |
| **Cut-through** | 读 **前若干字节**（如 **目标 MAC 6B** 或 **64B fragment-free**）即转发 | **首选** |
| **Layer-1 Switch** | 物理层 **跳线**（Arista/Cisco 等）— **不解析修改头部** | **极低延迟** |

**共址机柜内** 交换机选型与 **NIC 延迟** 同为 **T2T 预算** 一部分。

---

## 3. TCP vs UDP

| | **TCP** | **UDP** |
|---|---------|---------|
| **语义** | 可靠 · 有序 | 无连接 · **不保证送达** |
| **开销** | 握手 · 重传 | **低** |
| **HFT 分工** | **订单 (Orders)** — 关键会话 | **行情 (Market Data)** — 常 **Multicast** |
| **趋势** | 传统 OUCH 多 TCP | **UFO** 等 — **UDP 发单** 追极致速度 |

```
Market Data:  Exchange ──UDP multicast──► Gateway IN
Orders:       Gateway OUT ──TCP or UDP──► Exchange
```

→ [chapter-02 Gateway](./chapter-02-交易所架构与撮合原理.md#2-网关-gateways)

---

## 4. 金融应用层协议演进

| 协议 | 特点 | HFT |
|------|------|-----|
| **FIX** | **字符串** · 人类可读 · 解析慢 | 控制面/legacy |
| **FAST** | UDP + **压缩/增量** · 解压耗 CPU | 过渡 |
| **二进制（首选）** | **ITCH**（行情）· **OUCH**（订单）· **CME SBE** | **固定布局 · μs 解析** |

**Gateway IN：** wire format → **cast/模板解码** — 避免字符串扫描。

→ [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)

---

## 5. 数据包生命周期（Kernel 路径）

交易所数据到达 **NIC** 后的 **传统 Linux 路径**：

```
1. NIC DMA → RX ring buffer（内存）
2. 硬件 IRQ → CPU 打断用户态
3. 用户态 → 内核态（中断处理）
4. 分配 sk_buff · 剥 L2/L3/L4 头
5. 放入 socket 接收队列
6. 应用 recv() 再拷贝到用户缓冲
```

| 问题 | 每一步 **μs 级** 累积 + **缓存污染** |
|------|--------------------------------------|

**这就是为什么需要 Kernel Bypass：**

- **OpenOnload / DPDK** — 用户态 **直接 poll RX ring** · **零拷贝**
- 详见 [chapter-05 §3](./chapter-05-操作系统内核极致调优.md#3-kernel-bypass)

→ [14-DPDK Intro](../14-DPDK-Low-Latency-Network/01-Intro-Book/)

---

## 6. 监控与时间同步

### 抓包：TAP

| 设备 | 说明 |
|------|------|
| **Passive TAP** | **分光** 复制流量 · **不需电源** · **不增加原链路延迟** |
| 用途 | 离线分析 · 对照 **硬件时间戳** |

**原则：** 监控 **不能** 影响生产路径 — TAP **带外**。

### PTP（精确时间协议）

| | **NTP** | **PTP (IEEE 1588)** |
|---|---------|---------------------|
| 精度 | ms 级 | **ns~ps 级**（硬件打戳） |
| HFT | 不够 | **T2T / 共址测量基石** |

- 源：**GPS / 原子钟**  
- **NIC PHC** 打戳优于纯软件 `clock_gettime`

→ [chapter-10 T2T 测量](./chapter-10-延迟测量与基准压测.md)

---

## 7. 广域链路

跨数据中心 **Latency Arbitrage**（如 **CME ↔ NYSE**）：

| 介质 | 特点 |
|------|------|
| **传统光纤** | 光在玻璃 **~2/3 c** · 带宽高 · 稳定 |
| **Microwave 微波** | 空气 **~c** · **直线** · **最快** · **带宽极低** · **雨/云干扰** |
| **Hollow fiber 空芯光纤** | 芯内 **空气通道** · 快于实心 · 常 **DC ↔ 微波塔**（~数百码） |

**工程现实：** 容量、天气、许可、成本 — 与 **共址 + Cut-through** 同属基础设施栈。

→ 原书 Ch7 §3：[chapter-10 §3](./chapter-10-延迟测量与基准压测.md#3-跨数据中心微波与空芯光纤)

---

## 本章小结

| 要赢 μs 级 | 行动 |
|------------|------|
| **机房内** | Cut-through / L1 switch · 共址 |
| **协议** | 行情 **UDP multicast** · 订单 TCP/UDP · **ITCH/OUCH/SBE** |
| **主机** | Bypass **跳过 §5 内核路径** |
| **测量** | **Passive TAP + PTP** |

**下一步（原书 Ch6 ≈ 软件压榨）：** [chapter-07 无锁与内存](./chapter-07-无锁数据结构与内存布局.md) · [chapter-05 OS 绑核](./chapter-05-操作系统内核极致调优.md)

---

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch5 Networking in Motion | **本章 Ch6** |
| Ch6 HFT 优化（架构/OS） | **Ch5 + Ch7 + Ch8** |
