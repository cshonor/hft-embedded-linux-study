## 3. Cache 预取 (Prefetching)

> 处理一个报文 = **多次内存读** — 不命中 Cache 则 **数百 cycle** 空等

---

### 一、为何要预取

典型收包路径需读：

- **RX 描述符** — 8/16B，包含 DD 位和 mbuf 指针
- **mbuf 头部** — `struct rte_mbuf`，~128B，包含元数据和数据指针
- **报文头部** — L2/L3/L4 头，前 64B 最关键

任一 **cache miss** 都会拖慢 **整包延迟**。在 HFT 场景下，一次 L3 miss (~40 cycle) 就可能让 tick-to-trade 从 500ns 变成 600ns。

---

### 二、硬件预取

基于 **时间局部性 / 空间局部性** — CPU **自动** 预取相邻地址。

| 有效 | 无效 |
|------|------|
| **顺序** 扫描数组、描述符环 | **跳跃式**、指针 chasing、哈希随机访存 |
| 线性访问 mbuf 数据 | 链表遍历、树查找 |

无效时预取 **浪费带宽**、**挤掉** 有用 cache line。可通过 `prefetchnta`（非临时对齐提示）减少污染。

---

### 三、软件预取（DPDK 常用）

开发者 **显式** 提前加载即将用到的数据：

| 手段 | 示例 | DPDK 封装 |
|------|------|-----------|
| 内联汇编 | `PREFETCH0 addr` | — |
| Intrinsics | `_mm_prefetch(addr, _MM_HINT_T0)` | `rte_prefetch0(addr)` |
| 非临时 | `_mm_prefetch(addr, _MM_HINT_NTA)` | `rte_prefetch_non_temporal(addr)` |

**用法：** 在处理 **当前包** 时，prefetch **下一包** 的描述符 / mbuf / 数据头 — **与计算重叠** 访存延迟。

**DPDK PMD 中的经典 prefetch 模式：**

```c
/* ixgbe/i40e PMD 收包循环中的 prefetch 策略 */
uint16_t
pmd_rx_burst(uint16_t port_id, uint16_t queue_id,
             struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
    volatile union ixgbe_adv_rx_desc *rx_ring = ...;
    uint16_t rx_id = rxq->rx_tail;

    for (int i = 0; i < nb_pkts; i++) {
        /* 预取后续描述符 — 提前 ~20 cycle 发起 L2 加载 */
        rte_prefetch0(&rx_ring[rx_id + 4]);

        /* 检查 DD 位 — 当前描述符已就绪？ */
        if (!(rx_ring[rx_id].wb.upper.status_error & IXGBE_RXDADV_STAT_DD))
            break;

        /* 预取 mbuf 结构体本身 */
        struct rte_mbuf *mbuf = rxq->sw_ring[rx_id];
        rte_prefetch0(mbuf);

        /* 预取 mbuf 数据区域（报文内容） */
        rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));

        /* 处理当前包（上一轮 prefetch 的数据此时应已到 L1） */
        rx_pkts[i] = mbuf;
        rx_id = (rx_id + 1) & (rxq->nb_rx_desc - 1);
    }

    return i;
}
```

**关键洞察：** prefetch 的本质是 **用计算时间隐藏访存延迟**。发出 prefetch 后，CPU 不等待，继续执行后续指令；当真正访问该地址时，数据已在 L1。预取距离通常 **3-4 个包** — 太近没效果，太远数据可能被驱逐。

> **深潜：** `rte_prefetch0()` 在 x86 上编译为 `PREFETCHT0` 指令 — 将数据加载到所有 cache 层级（L1/L2/L3）。`_MM_HINT_T1` 只到 L2，`_MM_HINT_T2` 只到 L3。NTA（Non-Temporal Access）提示数据不会被再次访问，避免污染 cache。

 PMD 热路径：[chapter-07 网卡性能优化](../../chapter-07-nic-performance-optimization/) · [CSAPP Ch6](../../../../02-computer-systems/chapter-06-memory-hierarchy/)

---

### 四、DPDK mempool 的内置 prefetch

`rte_mempool` 在 per-lcore cache 中维护一个 mbuf 栈，取出时自动 prefetch 下一个：

```c
/* rte_mempool_get() 内部简化逻辑 */
void *rte_mempool_get(struct rte_mempool *mp)
{
    struct rte_mempool_cache *cache = rte_mempool_default_cache(mp, rte_lcore_id());

    /* 从 per-lcore cache 取 — 无锁 */
    void *obj = cache->objects[--cache->len];

    /* 预取下一个对象 — 为下次 get 铺路 */
    if (cache->len > 0)
        rte_prefetch0(cache->objects[cache->len - 1]);

    return obj;
}
```

这就是 mempool cache 的双重价值：**减少锁竞争** + **隐藏访存延迟**。

---

← [2. Cache 层次](./section-2-阶梯式Cache系统.md) · 下一节 [4. 一致性](./section-4-Cache一致性与无锁设计.md)
