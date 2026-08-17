## 6. Mbuf 与 Mempool

> DPDK 为配合底层 I/O 设计的 **包缓冲** 与 **对象池** — 实体书 mbuf 精讲

---

### 一、rte_mbuf 结构

**元数据 (Metadata) + 帧数据** 统一组织：

```c
struct rte_mbuf {
    /* ---- 第 1 条 Cache Line（热路径字段）---- */
    void *buf_addr;                 /* 数据缓冲区起始地址 */
    uint16_t data_off;              /* 数据起始偏移（headroom 后） */
    uint16_t refcnt;                /* 引用计数 */
    uint16_t nb_segs;               /* 段数（jumbo 帧） */
    uint16_t port;                  /* 来源端口 */
    uint64_t ol_flags;              /* offload 标志（checksum、VLAN 等） */
    uint32_t pkt_len;               /* 整包长度（含所有段） */
    uint16_t data_len;              /* 当前段数据长度 */
    /* ... VLAN、RSS hash、timestamp 等 ... */

    /* ---- 第 2 条 Cache Line（冷路径字段）---- */
    struct rte_mempool *pool;       /* 所属 mempool */
    struct rte_mbuf *next;          /* 下一段（chained mbuf） */
    /* ... 用户自定义数据区 ... */
} __rte_cache_aligned;

/* 内存布局 */
/* ┌──────────┬──────────┬──────────┬──────────────┐ */
/* │ rte_mbuf │ headroom │  data    │ tailroom     │ */
/* │ (128B)   │ (128B)   │ (pktsz)  │ (剩余空间)    │ */
/* └──────────┴──────────┴──────────┴──────────────┘ */
/*                       ↑                         */
/*                    buf_addr + data_off          */
```

| 设计点 | 说明 |
|--------|------|
| **固定 Cache Line 头部** | 2 条 Cache Line — 热字段放 **第一 Line**，减 miss |
| **head room** | 头部与数据间 **预留 128B** — 封装/VLAN/GRE 头可 **向前生长** 而不 realloc |
| **buf_addr / data_off / pkt_len** | 与 PMD **零拷贝** 衔接 — DMA 直接写入 pool 对象 |
| **refcnt** | 引用计数 — 多消费者共享同一包时 `rte_mbuf_refcnt_update(mbuf, 1)` |

**与内核 sk_buff 对照：**

| | `rte_mbuf` | `sk_buff` |
|---|-----------|-----------|
| 分配 | mempool 预分配，无 malloc | `alloc_skb()` 动态分配（slab） |
| 元数据大小 | ~128B（2 cache line） | ~232B |
| headroom | 128B | 64B（可配） |
| 数据访问 | `rte_pktmbuf_mtod()` 宏 | `skb->data` 指针 |
| 释放 | `rte_pktmbuf_free()` 回池 | `kfree_skb()` 回 slab |

对照内核 **sk_buff** → [12-kernel-networking](../../../../12-kernel-networking/) · [Ch1 mbuf 提及](../../chapter-01-dpdk-intro/notes/section-2-硬件平台与DPDK定位.md)

---

### 二、rte_mempool 结构

基于 **双环形缓冲区** 的 **无锁** 结构（与 [Ch4 rte_ring](../../chapter-04-synchronization/notes/section-5-无锁机制.md) 同族思想）：

```c
/* 创建 mbuf pool */
struct rte_mempool *pool = rte_pktmbuf_pool_create(
    "MBUF_POOL",
    32768,              /* 总 mbuf 数 — 必须够覆盖在途+持有+burst */
    256,                /* per-lcore cache size */
    0,                  /* mbuf 私有数据大小 */
    RTE_MBUF_DEFAULT_BUF_SIZE,  /* 2176 = 2048 data + 128 headroom */
    rte_socket_id()     /* NUMA 节点 */
);

/* 取 mbuf（热路径） */
struct rte_mbuf *m = rte_pktmbuf_alloc(pool);
/* → 先从 per-lcore cache 取 — 无锁、无 CAS */
/* → cache 空时批量从全局 ring 补充 */

/* 还 mbuf */
rte_pktmbuf_free(m);
/* → 放回 per-lcore cache — 无锁 */
/* → cache 满时批量刷回全局 ring */
```

**mempool 两级结构：**

```
┌─────────────────────────────────────────┐
│  Global Ring (MP/MC rte_ring)           │
│  [mbuf] [mbuf] [mbuf] ... [mbuf]        │  ← 所有 mbuf 初始在此
└──────┬──────────────────┬───────────────┘
       │ bulk get (256)   │ bulk get (256)
       ↓                  ↓
┌──────────────┐   ┌──────────────┐
│ lcore 0 cache│   │ lcore 1 cache│       ← per-lcore 私有
│ [mbuf]×256   │   │ [mbuf]×256   │       ← 无锁 get/put
└──────────────┘   └──────────────┘
```

---

### 三、Mempool 深度优化

**1. 内存通道 / Rank 对齐**

- 对象间 **Padding**，使相邻对象落到 **不同通道、Rank**
- 提高 **并发 DRAM 访问** 带宽 — 与 [Ch2 NUMA](../../chapter-02-cache-and-memory/notes/section-6-DDIO与NUMA.md) 叠加

**2. Per-lcore Cache（核心优化）**

| 问题 | 对策 |
|------|------|
| 多核同时 CAS 争用 **全局 ring** | 每 lcore **私有小块缓存**（默认 512 个） |
| 频繁跨核同步 | **批量** 从全局池 **填充/刷回** 本地 cache |

```c
/* mempool get 热路径 — per-lcore cache 命中时零锁 */
static inline int
mempool_get(struct rte_mempool *mp, void **obj_table, unsigned int n)
{
    struct rte_mempool_cache *cache;
    cache = rte_mempool_default_cache(mp, rte_lcore_id());

    /* 1. 先从 per-lcore cache 取 — 无锁 */
    if (cache->len >= n) {
        /* 快路径 — 全部从 cache 取 */
        for (int i = 0; i < n; i++)
            obj_table[i] = cache->objects[--cache->len];
        return 0;
    }

    /* 2. 慢路径 — cache 不够，批量从全局 ring 补充 */
    rte_ring_mc_dequeue_bulk(mp->pool, temp, cache->flushthresh, NULL);
    /* 补充后再次从 cache 取 */
    ...
}
```

 热路径 **优先本地 get/put**，极大降低 [Ch4 CAS](../../chapter-04-synchronization/notes/section-2-原子操作.md) 争用。

---

### 四、与 I/O 链路的衔接

```
rte_pktmbuf_pool_create (大页/NUMA)
    ↓
PMD RX setup: rte_eth_rx_queue_setup(port, queue, desc, socket, NULL, pool)
    ↓ 描述符的 buf_addr 指向 mbuf 数据区
    ↓
NIC DMA → 写入 mbuf data 区域 → 置 DD 位
    ↓
PMD poll: rte_eth_rx_burst() → 检查 DD → 取出 mbuf → 重填描述符
    ↓
应用处理 → TX 或 rte_pktmbuf_free() 回 pool
```

**HFT 配置：**

```c
/* pool 大小计算 */
uint16_t nb_ports = rte_eth_dev_count_avail();
uint32_t nb_mbufs = RTE_MAX(
    nb_ports * RX_DESC_DEFAULT +    /* 在途 RX 描述符 */
    nb_ports * TX_DESC_DEFAULT +    /* 在途 TX 描述符 */
    nb_ports * BURST_SIZE * MAX_LCORES +  /* 各核 burst 持有量 */
    8192,                            /* 安全余量 */
    8192);

/* NUMA 感知 — 每个节点一个 pool */
struct rte_mempool *pools[RTE_MAX_NUMA_NODES];
for (int i = 0; i < nb_nodes; i++) {
    pools[i] = rte_pktmbuf_pool_create(
        "MBUF_POOL", nb_mbufs, 256, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, i);
}

/* 每个 RX 队列用同 NUMA 的 pool */
int socket_id = rte_eth_dev_socket_id(port_id);
rte_eth_rx_queue_setup(port_id, queue, RX_DESC, socket_id, NULL, pools[socket_id]);
```

---

← [5. 净荷带宽计算](./section-5-PCIe净荷带宽计算.md) · 下一节 [7. 小结与索引](./section-7-小结与索引.md)
