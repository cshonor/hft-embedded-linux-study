## 7. 小结与后续索引

---

### 一、本章总结

**I/O 层 + 缓冲层：**

| 层次 | 要点 |
|------|------|
| **PCIe TLP** | MRd/MWr；协议头吃掉有效带宽 |
| **描述符环** | Head/Tail、DD；驱动本质是 **环管理** |
| **MMIO 优化** | 批量 refill/完成确认；减 Tail 写频率 |
| **对齐** | Cache Line — 避免 RMW 部分写 |
| **带宽量化** | 64B 小包 PCIe 总开销 ≫ 64B |
| **mbuf/mempool** | 2×CL 头、head room、双 ring 池、**Core Cache** |

```
Ch2 Cache/大页/NUMA
    ↓
Ch5 报文转发 — 软件框架
    ↓
Ch6 PCIe 与 I/O（本章）— DMA + mbuf/mempool
    ↓
PMD 轮询 — burst 收发包
    ↓
Ch8 多队列 — 分流减单链 PCIe 压力
```

---

### 二、后续章节索引

| Ch6 主题 | 继续读 |
|----------|--------|
| PMD / burst | [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md) 🔴 |
| mbuf 实验 stub | [chapter-02-mbuf与内存池.md](../chapter-02-mbuf与内存池.md) 🔴 |
| Cache / 大页 / DDIO | [chapter-02-Cache与内存](../chapter-02-Cache与内存/) 🔴 |
| 无锁 ring / CAS | [chapter-04-同步互斥机制](../chapter-04-同步互斥机制/) 🔴 |
| 多队列 / RSS | [chapter-08-流分类与多队列](../chapter-08-流分类与多队列/) 🔴 |
| 零拷贝旁路 | [chapter-04-零拷贝与用户态旁路.md](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| 内核 sk_buff | [13-LKN](../../../13-Linux-Kernel-Networking/) |
| HFT 网络 | [15 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [6. Mbuf 与 Mempool](./section-6-Mbuf与Mempool.md) · 下一章 [chapter-03-PMD](../chapter-03-PMD与轮询模式.md) · [Ch5 转发](../chapter-05-报文转发/)
