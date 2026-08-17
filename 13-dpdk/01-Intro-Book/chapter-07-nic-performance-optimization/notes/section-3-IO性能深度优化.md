## 3. 网卡 I/O 性能深度优化

> 收发包软件路径上的 **微架构级** 手段 — 与 [Ch3 ILP](../../chapter-03-parallel-computing/notes/section-3-指令级并发.md)、[Ch6 MMIO 批量](../../chapter-06-pcie-packet-io/notes/section-4-CPU与IO协奏优化.md) 一脉相承

---

### 一、Burst 收发包

| 要点 | 说明 |
|------|------|
| **API** | `rte_eth_rx_burst` / `rte_eth_tx_burst` — 一次处理 **8 / 16 / 32** 包 |
| **Cache** | 连续描述符/mbuf 访问 → **预取友好**，贴合 **64B Cache Line** |
| **MMIO** | 摊薄 **Tail 寄存器** 写频率（Ch6）— 1 次 MMIO 写覆盖 32 包 |

**burst size 对延迟和吞吐的影响：**

| BURST_SIZE | 延迟 | 吞吐效率 | 适用场景 |
|:---:|------|----------|----------|
| 1 | 最低（立即处理） | 极低（每包 1 次 MMIO） | 不推荐 |
| 8 | 低 | 中等 | 低延迟原型 |
| **32** | **平衡** | **高** | **HFT 推荐** |
| 64 | 略高 | 最高 | 吞吐优先（非 HFT） |

```c
/* burst 收发 — DPDK 标准模式 */
#define BURST_SIZE 32

while (!force_quit) {
    /* 一次最多取 32 个包 */
    nb_rx = rte_eth_rx_burst(port_id, queue_id, bufs, BURST_SIZE);

    if (nb_rx > 0) {
        /* 批量处理 — 利用 ILP 和 prefetch */
        for (int i = 0; i < nb_rx; i++) {
            /* 预取后续包数据 */
            if (i + 2 < nb_rx)
                rte_prefetch0(rte_pktmbuf_mtod(bufs[i+2], void *));
            process_packet(bufs[i]);
        }

        /* 批量发送 — 1 次 Tail 寄存器写 */
        rte_eth_tx_burst(port_id, queue_id, bufs, nb_rx);
    }
}
```

---

### 二、时延隐藏与批量处理

内存读、描述符 fetch **高延迟** — 利用 **超标量 + 乱序执行**：

- 将 **无数据依赖** 的多包处理 **铺开**（multi-packet pipeline）
- 当前包等内存时，CPU 执行 **下一包** 独立指令
- **prefetch 距离** 通常 2-4 包 — 覆盖 L3 miss 延迟 (~40 cycle)

```c
/* 多包流水线 — 处理包 N 时预取包 N+3 */
for (int i = 0; i < nb_rx; i++) {
    /* 预取 N+3 的 mbuf 结构和数据 */
    if (i + 3 < nb_rx) {
        rte_prefetch0(bufs[i + 3]);
        rte_prefetch0(rte_pktmbuf_mtod(bufs[i + 3], void *));
    }
    /* 处理当前包 — 此前 prefetch 的数据应已到 L1 */
    process_packet(bufs[i]);
}
```

 [Ch3 Gustafson / 并行](../../chapter-03-parallel-computing/) · [Ch2 Cache 预取](../../chapter-02-cache-and-memory/notes/section-3-Cache预取.md)

---

### 三、减少 Cache Line 冲突

**问题：** CPU **更新环尾 / 重填描述符** 与 NIC **DMA 写回** 争用 **同一 Cache Line**（读-写、写-写 bouncing）。

| 手段 | 效果 | 实现 |
|------|------|------|
| **批量分配** 新 mbuf / 描述符 | 少次 touch 控制行 | `rte_pktmbuf_alloc_bulk()` |
| **延迟更新 Tail** | 按 **Cache Line 整数倍** 移动尾指针 | 积累 8-16 包后一次写 |
| **对齐** | 环、控制块按 cache line 对齐 | `__rte_cache_aligned` |
| **DDIO** | DMA 写直达 LLC 减少 DRAM 往返 | BIOS 启用 |

---

### 四、SIMD 向量化描述符处理

- **SSE/AVX Shuffle** — **一次处理多个描述符** 字段转换
- 批量检查 DD 位 — 一次加载 4-8 个描述符，SIMV 比较所有 DD 位
- 与 [Ch3 SIMD / rte_memcpy](../../chapter-03-parallel-computing/notes/section-4-数据并行与SIMD.md) 同思路

```c
/* SIMD 批量检查 DD 位 — 一次检查 4 个描述符 */
__m128i dd_mask = _mm_set1_epi32(0x01);  /* DD 位 */
__m128i desc0_3 = _mm_load_si128((const __m128i *)rx_descs);
__m128i dd_check = _mm_and_si128(desc0_3, dd_mask);
/* 一次比较 4 个描述符的 DD 位 */
```

---

### 五、TX 路径优化

```c
/* TX cleanup — 批量回收已发送的 mbuf */
static inline void
tx_cleanup(struct tx_queue *txq)
{
    /* 检查硬件写回的 DD 位 */
    volatile struct tx_desc *tx_desc;
    uint16_t nb_clean = 0;

    while (txq->free_desc < txq->nb_tx_desc - TX_FREE_THRESHOLD) {
        tx_desc = &txq->tx_ring[txq->tx_tail];
        if (!(tx_desc->status & TX_DESC_DD))
            break;

        /* 批量释放 mbuf */
        rte_pktmbuf_free(txq->sw_ring[txq->tx_tail]);
        txq->sw_ring[txq->tx_tail] = NULL;
        txq->tx_tail = (txq->tx_tail + 1) & (txq->nb_tx_desc - 1);
        txq->free_desc++;
        nb_clean++;
    }
}
```

**TX rs_threshold：** 每发送 N 个包后，硬件回写一次 DD 位 — 减少 cache line bouncing。典型值 32。

---

← [2. 轮询与混合中断](./section-2-轮询与混合中断模式.md) · 下一节 [4. 平台优化](./section-4-平台优化与配置调优.md)
