## 6. 小结与后续索引

---

### 一、本章总结

**框架 + 算法 + 分发：**

| 层次 | 要点 |
|------|------|
| **模块划分** | 硬件 offload 优先；软件靠 **高效算法 + 并行** |
| **RTC** | 一核跑全程 — **简单、低延迟、易扩展副本** |
| **Pipeline** | Port / Table / Action — **复杂产品、stage 扩展** |
| **算法** | Hash（CRC SIMD）、LPM（tbl24/8）、ACL（Tier） |
| **Distributor** | **同流同 Worker** — 保序、有状态 |

```
Ch4 同步互斥 — ring / LPM·ACL 锁
    ↓
Ch5 报文转发（本章）— 框架 + 算法 + Distributor
    ↓
Ch6 PCIe 与 I/O — DMA、mbuf/mempool
    ↓
Ch8 流分类与多队列 — 硬件 RSS / FD 分核
    ↓
PMD — burst 收发包
```

---

### 二、后续章节索引

| Ch5 主题 | 继续读 |
|----------|--------|
| PCIe / DMA / mbuf | [chapter-06-PCIe与包处理IO](../chapter-06-pcie-packet-io/) 🔴 |
| 硬件分核 / RSS | [chapter-08-流分类与多队列](../chapter-08-flow-classification-multiqueue/) 🔴 |
| mbuf / ring | [chapter-02-mbuf与内存池.md](../chapter-02-mbuf与内存池.md) 🔴 |
| PMD 轮询 | [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md) 🔴 |
| LPM / ACL 锁 | [chapter-04-同步互斥机制](../chapter-04-synchronization/) 🔴 |
| SIMD / CRC | [chapter-03-并行计算](../chapter-03-parallel-computing/) 🔴 |
| 组播落地 | [chapter-05-组播行情接入.md](../chapter-05-组播行情接入.md) 🔴 |
| 内核转发对照 | [13-LKN](../../../11-Linux-Kernel-Networking/) |
| HFT 网络架构 | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 报文分发](./section-5-报文分发机制.md) · 下一章 [chapter-06-PCIe与I/O](../chapter-06-pcie-packet-io/) · [Ch4 同步](../chapter-04-synchronization/)
