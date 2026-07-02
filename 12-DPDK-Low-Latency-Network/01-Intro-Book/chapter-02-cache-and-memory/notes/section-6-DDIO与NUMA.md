## 6. DDIO 与 NUMA

---

### 一、DDIO（Data Direct I/O）

**传统路径：**

```
网卡 → PCIe → 主存(DRAM) → CPU 再载入 Cache
```

**Intel DDIO：**

- 外部设备（网卡）可与 **CPU L3 Cache (LLC)** **直接交换数据**  
- **绕过慢速 DRAM** 往返 — 降 **延迟** 与 **内存带宽** 压力  

**HFT 含义：** 共置 **Intel + 支持 DDIO 的网卡** 时，收包数据可能 **已在 LLC** — 与软件 prefetch 协同；无 DDIO 平台更依赖 **大页 + 预取 + 绑核**。

→ [Ch1 最佳实践](../chapter-01-dpdk-intro/notes/section-3-性能最佳实践.md)

---

### 二、NUMA 架构

核数增加后 **SMP 总线** 瓶颈 → **NUMA**：

| 概念 | 说明 |
|------|------|
| **Node** | 每颗 CPU（或片）**直连** 本地内存 + 本地 PCIe |
| **本地访问** | 低延迟 |
| **远程访问** | 跨 **QPI/UPI** — **高延迟、占带宽** |

→ [ULK Ch8 ZONE / 节点](../../../04-Understanding-Linux-Kernel/chapter-08-memory-management/notes/section-2-页框管理.md)

---

### 三、DPDK NUMA 感知

**「本地设备本地处理」：**

| 资源 | 原则 |
|------|------|
| **网卡** | 插在 **Node N** |
| **处理 lcore** | 绑定 **Node N** 的核 |
| **大页 / mempool** | 从 **Node N** **`socket_id`** 分配 |
| **RX queue** | 队列 **i** 由 **同 NUMA 核** 轮询 |

违反 → **远程内存 + 远程 PCIe** — tail latency **恶化**。

**工具：** `lstopo` · `dpdk-devbind` · EAL `--socket-mem` / `-l` 绑核。

---

### 四、本章小结

```
Cache 层次 + 软件预取 → 隐藏延迟
对齐 + per-core → 避免 MESI 风暴
大页 → TLB 命中
DDIO → 数据进 LLC
NUMA → 本地内存本地 NIC
    ↓
mbuf/mempool 在正确内存上预分配
```

---

### 五、后续章节索引

| Ch2 主题 | 继续读 |
|----------|--------|
| mbuf / mempool | [chapter-02-mbuf](../chapter-02-mbuf与内存池.md) 🔴 |
| 并行 / SIMD | [chapter-03-并行计算](../chapter-03-parallel-computing/) 🔴 |
| PMD 收发包 | [chapter-03 PMD](../chapter-03-PMD与轮询模式.md) 🔴 |
| 零拷贝旁路 | [chapter-04](../chapter-04-零拷贝与用户态旁路.md) |
| CSAPP / Hennessy | [01 Ch6](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) · [02 Ch2](../../../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/) |
| ULK 内存 | [06 Ch8/9/17](../../../04-Understanding-Linux-Kernel/chapter-08-memory-management/) |
| HFT 工程 | [15 ch04/ch07](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 大页](./section-5-大页Hugepages.md) · 下一章 [mbuf 与内存池](../chapter-02-mbuf与内存池.md)
