## 6. 编程实例入门

> 三个 **由小到大** 的经典示例 — 建立 DPDK 程序直觉

---

### 一、HelloWorld — 最小骨架

| 要点 | 说明 |
|------|------|
| **最简入门** | 理解 DPDK 程序 **骨架** |
| **`rte_eal_init()`** | 初始化 **EAL**（Environment Abstraction Layer）— 大页、lcore、PCI 等 |
| **多核启动** | 各 lcore 执行 `rte_eal_remote_launch` 或等价 worker 入口 |

**HelloWorld 代码骨架：**

```c
#include <rte_eal.h>
#include <rte_lcore.h>

static int
lcore_hello(__rte_unused void *arg)
{
    unsigned lcore_id = rte_lcore_id();
    printf("hello from lcore %u\n", lcore_id);
    return 0;
}

int main(int argc, char **argv)
{
    /* EAL 初始化 — 解析 DPDK 参数、映射大页、扫描 PCI 设备 */
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_panic("EAL init failed\n");

    /* 在每个可用 lcore 上启动 worker */
    rte_eal_mp_remote_launch(lcore_hello, NULL, CALL_MAIN);

    /* 等待所有 lcore 完成 */
    rte_eal_mp_wait_lcore();

    rte_eal_cleanup();
    return 0;
}
```

**编译运行：**

```bash
# 编译（使用 DPDK pkg-config）
gcc -o helloworld helloworld.c $(pkg-config --cflags --libs libdpdk)

# 运行 — EAL 参数
./helloworld -l 0-3 -n 4
# 输出：
# hello from lcore 0
# hello from lcore 1
# hello from lcore 2
# hello from lcore 3
```

 EAL 深潜：官方 [Programmer's Guide · EAL](https://doc.dpdk.org/guides/prog_guide/env_abstraction_layer.html) · 后续 [chapter-02 mbuf](../../chapter-02-cache-and-memory)

---

### 二、Skeleton — 最小收发包

| 要点 | 说明 |
|------|------|
| **单核最小骨架** | 收包 → **不处理** → 转发 |
| **核心 API** | `rte_eth_rx_burst()` / `rte_eth_tx_burst()` |
| **目的** | 看清 **PMD 收发环** 最小闭环 |

**Skeleton 核心流程：**

```c
/* 1. 初始化 EAL */
rte_eal_init(argc, argv);

/* 2. 配置以太网设备 */
struct rte_eth_conf port_conf = {
    .rxmode = {
        .mtu = RTE_ETHER_MTU,
        .offloads = DEV_RX_OFFLOAD_IPV4_CKSUM,
    },
};

/* 3. 分配 mempool — 收包缓冲区来源 */
struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
    "MBUF_POOL",
    NUM_MBUFS * nb_ports,   /* 总 mbuf 数 */
    MBUF_CACHE_SIZE,        /* per-lcore cache — 减少锁竞争 */
    0,                      /* 私有数据大小 */
    RTE_MBUF_DEFAULT_BUF_SIZE,  /* 2048 + headroom */
    rte_socket_id()         /* NUMA 感知分配 */
);

/* 4. 配置 RX/TX 队列 */
rte_eth_dev_configure(port_id, 1, 1, &port_conf);
rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE,
                       rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE,
                       rte_eth_dev_socket_id(port_id), NULL);

/* 5. 启动设备 */
rte_eth_dev_start(port_id);
rte_eth_promiscuous_enable(port_id);

/* 6. PMD 主循环 */
while (!force_quit) {
    nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
    if (nb_rx > 0)
        rte_eth_tx_burst(port_id, 0, bufs, nb_rx);  /* 直接回转 */
}
```

 [chapter-07 PMD 轮询](../../chapter-07-nic-performance-optimization/)

---

### 三、L3fwd — 三层转发

DPDK **最流行** 示例之一 — **三层转发**：

| 能力 | 说明 |
|------|------|
| 结合 HelloWorld + Skeleton | 多核 + 真实转发逻辑 |
| **Exact Match (Hash)** | 精确匹配转发 — `rte_hash` |
| **LPM** | 最长前缀匹配 — `rte_lpm`，tbl24/tbl8 两级查表 |

**L3fwd 数据路径：**

```
收包 → 解析以太网头 → 判断 IPv4/IPv6
  → 查路由表 (LPM 或 Hash)
  → 修改 MAC/IP/TTL
  → 重新计算 checksum (硬件 offload)
  → 从目标端口发送
```

**HFT 类比：** 行情 **UDP 五元组过滤**、简单 **ACL 转发** 与 L3fwd 结构类似（查表 → 选端口/out queue）。实际 HFT 行情网关更简单 — 通常只做 **组播订阅 + 五元组过滤 + payload 解析**，不需要 LPM 路由查找。

```c
/* HFT 行情网关简化模型 — 基于 L3fwd 框架 */
while (!force_quit) {
    nb_rx = rte_eth_rx_burst(port_id, queue_id, bufs, BURST_SIZE);

    for (int i = 0; i < nb_rx; i++) {
        struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
        struct rte_ipv4_hdr  *ip  = (struct rte_ipv4_hdr *)(eth + 1);
        struct rte_udp_hdr   *udp = (struct rte_udp_hdr *)(ip + 1);

        /* 五元组过滤 — 订阅特定组播组 */
        if (ip->dst_addr == mcast_group && udp->dst_port == feed_port) {
            /* 解析行情 payload → 送策略引擎 */
            feed_parser(rte_pktmbuf_mtod_offset(bufs[i], void *,
                        sizeof(*eth) + sizeof(*ip) + sizeof(*udp)));
        }
        rte_pktmbuf_free(bufs[i]);  /* 释放 mbuf 回 mempool */
    }
}
```

 官方：[L3 Forwarding Sample](https://doc.dpdk.org/guides/sample_app_ug/l3_forward.html)

---

### 四、后续章节索引

| Ch1 主题 | 继续读 |
|----------|--------|
| Cache / 大页 / NUMA | [chapter-02-Cache与内存](../../chapter-02-cache-and-memory/) 🔴 |
| mbuf / mempool | [chapter-06 §6 mbuf与Mempool](../../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) 🔴 |
| PMD / 轮询 | [chapter-07 网卡性能优化](../../chapter-07-nic-performance-optimization/) 🔴 |
| 零拷贝旁路 | [chapter-04 同步互斥](../../chapter-04-synchronization/) 🔴 |
| 组播行情 | [chapter-05 报文转发](../../chapter-05-packet-forwarding/) 🔴 |
| 内核栈对照 | [12-kernel-networking](../../../../12-kernel-networking/) |
| XDP / RDMA 选型 | [02-Advanced-Book](../../../02-Advanced-Book/) |
| 工程落地 | [14 HFT ch06](../../../../14-hft-engineering/chapter-06-low-latency-network-protocol/) |

---

← [5. 应用潜力](./section-5-应用潜力.md) · 下一章 [Ch2 Cache与内存](../../chapter-02-cache-and-memory/)
