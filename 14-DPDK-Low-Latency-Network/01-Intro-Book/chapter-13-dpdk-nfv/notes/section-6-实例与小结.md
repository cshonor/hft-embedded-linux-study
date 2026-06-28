## 6. 实例解析与小结

---

### 一、Intel vBRAS 原型

**场景：** 宽带远程接入 — **QinQ、GRE** 隧道封装/解封装复杂。

**架构：** [Ch5 Pipeline](../chapter-05-packet-forwarding/notes/section-3-转发框架模型.md) 模型

| Stage | 线程 | 职责 |
|-------|------|------|
| **LB** | 负载均衡 | 报文分发到 worker |
| **WT** | 工作线程 | 隧道处理 |
| **RT** | 路由查找 | LPM 等 |

**传递：** **DPDK ring** — [Ch4 无锁](../chapter-04-synchronization/notes/section-5-无锁机制.md)

---

### 二、Brocade vRouter 5600

**商用虚拟路由器：**

| 要点 | 说明 |
|------|------|
| **控制/转发分离** | 控制面与 **DPDK 转发面** 独立 |
| **per-core 资源** | 各 X86 核 **避免争用** |
| **性能** | 接近 **专有硬件** 路由方案 |

→ 与 [Ch1 控制/数据分离](../chapter-01-dpdk-intro/notes/section-4-底层方法论.md) · [Ch7 isolcpus](../chapter-07-nic-performance-optimization/notes/section-4-平台优化与配置调优.md) 一致

---

### 三、本章总结

**DPDK × NFV = 通用服务器跑电信级网元的破局路径：**

```
NFVI（DPDK：I/O + 多核 + CryptoDev…）
    ↓
VNF（Pipeline / 接口选型 / QoS）
    ↓
MANO + 商业交付（vRouter、vBRAS…）
```

| 全书部分 | 本章收束 |
|----------|----------|
| Ch1–9 裸金属 | → VNF **数据面** 技术栈 |
| Ch10–12 虚拟化 | → **NFVI 接口** 选型 |
| **Ch13 应用篇** | → **运营商 / 云 NFV** 方法论 + 案例 |

**结语：** SDN/NFV 变现依赖 **VNF 性能** — DPDK 是 **关键使能** 之一（非唯一 — 见 [02-Advanced RDMA/XDP](../../02-Advanced-Book/)）。

---

### 四、后续索引

| Ch13 主题 | 继续读 |
|----------|--------|
| 虚拟化 NFVI | [chapter-10–12](../chapter-12-vhost-optimization/) 🟡 |
| Pipeline / ring | [chapter-05-packet-forwarding](../chapter-05-packet-forwarding/) 🔴 |
| 组播 / HFT 落地 | [chapter-05-组播行情接入.md](../chapter-05-组播行情接入.md) 🔴 |
| 进阶网络 | [02-Advanced-Book](../../02-Advanced-Book/) 🟡 |
| 性能方法论 | [03 SysPerf](../../../03-Systems-Performance-2nd/) |
| HFT | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. VNF 优化](./section-5-VNF深度优化设计.md) · [Ch12 vhost](../chapter-12-vhost-optimization/) · [01-Intro README](../README.md)
