## 6. Mbuf 与 Mempool

> DPDK 为配合底层 I/O 设计的 **包缓冲** 与 **对象池** — 实体书 mbuf 精讲；repo 实验 stub → [chapter-02-mbuf](../chapter-02-mbuf与内存池.md)

---

### 一、Mbuf（网络帧缓存）

**元数据 (Metadata) + 帧数据** 统一组织：

| 设计点 | 说明 |
|--------|------|
| **固定 Cache Line 头部** | 通常 **2 条 Cache Line** — 热字段放 **第一 Line**，减 miss |
| **head room** | 头部与数据间 **预留空间** — 封装/VLAN/GRE 头可 **向前生长** 而不 realloc |
| **buf_addr / data_off / pkt_len** | 与 PMD **零拷贝** 衔接 — DMA 直接写入 pool 对象 |

对照内核 **sk_buff** → [13-LKN](../../../13-Linux-Kernel-Networking/) · [Ch1 mbuf 提及](../chapter-01-认识DPDK/notes/section-2-硬件平台与DPDK定位.md)

---

### 二、Mempool（内存池）

基于 **双环形缓冲区** 的 **无锁** 结构（与 [Ch4 rte_ring](../chapter-04-同步互斥机制/notes/section-5-无锁机制.md) 同族思想）：

- **初始化时** 预分配全部对象 — 运行时 **无 malloc/free**  
- **get / put** 从池中取还 mbuf  

---

### 三、Mempool 深度优化

**1. 内存通道 / Rank 对齐**

- 对象间 **Padding**，使相邻对象落到 **不同通道、Rank**  
- 提高 **并发 DRAM 访问** 带宽 — 与 [Ch2 NUMA](../chapter-02-Cache与内存/notes/section-6-DDIO与NUMA.md) 叠加  

**2. 单核本地缓存 (Core Cache / Per-lcore Cache)**

| 问题 | 对策 |
|------|------|
| 多核同时 CAS 争用 **全局 ring** | 每 lcore **私有小块缓存** |
| 频繁跨核同步 | **批量** 从全局池 **填充/刷回** 本地 cache |

→ 热路径 **优先本地 get/put**，极大降低 [Ch4 CAS](../chapter-04-同步互斥机制/notes/section-2-原子操作.md) 争用。

---

### 四、与 I/O 链路的衔接

```
mempool_create (大页/NUMA) → rte_pktmbuf_pool_create
    → PMD RX: 描述符指向 mbuf 数据区
    → 应用处理 → TX 或 free 回 pool
```

**HFT 配置：** `socket_id` 与网卡 **同 NUMA**；pool 大小 ≥ 在途描述符 + 应用持有量 + burst 深度。

---

← [5. 净荷带宽计算](./section-5-PCIe净荷带宽计算.md) · 下一节 [7. 小结与索引](./section-7-小结与索引.md)
