## 2. 网卡多队列 (Multi-queue)

---

### 一、技术由来

| 驱动因素 | 说明 |
|----------|------|
| **多核 CPU** | 并行算力可用 |
| **高速 NIC** | 10G+ 单核 **跟不上** 线速 PPS — 10G/64B 需 ~14.88 Mpps |

**多队列：** 网卡提供 **多个硬件 RX/TX 队列** — 不同队列可由 **不同 CPU 核** 独立轮询处理。

**单核瓶颈计算：**

```
10Gbps / (64B × 8 bit) = 14.88 Mpps
每包可用 cycle = 3GHz / 14.88M = ~201 cycle
→ 单核处理时间不够（解析+查表+转发需 200-500 cycle）
→ 必须多核并行
```

 内核侧对照：[12-kernel-networking RSS/NAPI](../../../../12-kernel-networking/)

---

### 二、RSS — 硬件流分发

**RSS（Receive Side Scaling）** — 网卡硬件根据包头的 hash 值自动将包分发到不同 RX 队列：

```c
/* RSS 配置 — DPDK */
struct rte_eth_conf port_conf = {
    .rxmode = { .mtu = RTE_ETHER_MTU },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_hf = ETH_RSS_IP | ETH_RSS_UDP | ETH_RSS_TCP,
            /* hash 五元组：src_ip + dst_ip + src_port + dst_port + proto */
        }
    },
};

/* 配置 4 个 RX 队列 + 4 个 TX 队列 */
rte_eth_dev_configure(port_id, 4, 4, &port_conf);

/* 每个队列 setup */
for (int q = 0; q < 4; q++) {
    rte_eth_rx_queue_setup(port_id, q, RX_DESC,
                           rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
    rte_eth_tx_queue_setup(port_id, q, TX_DESC,
                           rte_eth_dev_socket_id(port_id), NULL);
}
```

**RSS hash 计算（硬件）：**

```
输入：{src_ip, dst_ip, src_port, dst_port, protocol}
  ↓ Toeplitz hash（硬件固定算法）
输出：32-bit hash value
  ↓ RETA (Redirection Table) 查表
分发到：queue_id = hash & (reta_mask)
```

**RETA（RSS Redirection Table）：**

```c
/* 查看/配置 RSS 重定向表 */
struct rte_eth_rss_reta_entry64 reta_conf[RETA_SIZE / 64];
rte_eth_dev_rss_reta_query(port_id, reta_conf, RETA_SIZE);

/* RETA 是一个 128 项的表，每项 8 bit (queue_id) */
/* hash 的低 7 位索引 RETA → 决定包去哪个队列 */
/* 可以手动调整 RETA 来均衡负载 */
```

**HFT 含义：** 组播行情通常固定到特定五元组 → RSS 会把它全部 hash 到 **同一个队列**。需要确保该队列绑定的 lcore 有足够处理能力，或使用 Flow Director 精确控制。

---

### 三、DPDK 与多队列的天然契合

| 模型 | 做法 | 优势 |
|------|------|------|
| **Run to Completion** | 一 lcore ↔ **专属 RX + TX 队列** | 只处理该队列报文，**无锁** |
| **Pipeline** | 多 lcore 共享队列 + `rte_ring` | 灵活但需无锁队列 |

**一对一绑核模式：**

```c
/* 每个 lcore 绑定一个 RX 队列 */
static int
lcore_worker(void *arg)
{
    unsigned lcore_id = rte_lcore_id();
    unsigned queue_id = lcore_id - FIRST_WORKER_LCORE;

    /* 每个 lcore 只轮询自己的队列 — 无竞争 */
    while (!force_quit) {
        nb_rx = rte_eth_rx_burst(port_id, queue_id, bufs, BURST_SIZE);
        /* ... 处理 + 发送 ... */
    }
}

/* 启动 — EAL 自动在指定 lcore 上运行 */
rte_eal_remote_launch(lcore_worker, NULL, worker_lcore_0);
rte_eal_remote_launch(lcore_worker, NULL, worker_lcore_1);
rte_eal_remote_launch(lcore_worker, NULL, worker_lcore_2);
rte_eal_remote_launch(lcore_worker, NULL, worker_lcore_3);
```

 [Ch2 per-core · NUMA](../../chapter-02-cache-and-memory/notes/section-4-Cache一致性与无锁设计.md) · [Ch1 绑核](../../chapter-01-dpdk-intro/notes/section-3-性能最佳实践.md)

---

### 四、Flow Director — 精确流分类

比 RSS 更精细 — 可指定特定五元组 → 特定队列：

```c
/* 将组播行情流定向到专用队列 */
struct rte_eth_fdir_filter fdir_filter = {
    .action = {
        .behavior = RTE_ETH_FDIR_ACCEPT,
        .rx_queue = 3,  /* 定向到 queue 3 */
    },
    .input = {
        .flow_type = RTE_ETH_FLOW_UDP4,
        .flow.udp4_flow = {
            .dst_ip = htonl(0xE0010001),  /* 224.1.0.1 组播组 */
            .dst_port = htons(10001),     /* 行情端口 */
        },
    },
};

rte_eth_dev_filter_ctrl(port_id, RTE_ETH_FILTER_FDIR,
                        RTE_ETH_FILTER_ADD, &fdir_filter);
/* 组播 224.1.0.1:10001 的包全部去 queue 3 */
/* 由专用 lcore 轮询处理 */
```

**HFT 场景：** 行情 A → queue 0（lcore 2），行情 B → queue 1（lcore 4），管理面 → queue 2（lcore 6）— 隔离不同行情源。

---

### 五、配置要点

- **`rte_eth_dev_configure()`** — `nb_rx_queue` / `nb_tx_queue`
- **队列 i** 绑定 **lcore j** + **NUMA socket** 与大页一致
- **PMD** `rx_burst` / `tx_burst` **per-queue 调用**
- **DCB（Data Center Bridging）** — 可配合优先级队列实现不同流量类别

 [chapter-07 PMD](../../chapter-07-nic-performance-optimization/) · [Ch8 流分类](./section-3-硬件流分类.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 流分类](./section-3-硬件流分类.md)
