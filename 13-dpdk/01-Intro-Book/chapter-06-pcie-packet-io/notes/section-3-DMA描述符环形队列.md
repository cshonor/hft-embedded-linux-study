## 3. DMA 描述符环形队列

---

### 一、队列结构

DMA 控制器通过 **环形队列** 与 CPU 协作：

| 组件 | 作用 | 大小 |
|------|------|------|
| **描述符数组** | 物理 **连续** 内存，每项描述一块缓冲区 | 16-32B/描述符 |
| **控制寄存器** | **Base**（基址）、**Size**（深度）、**Head**、**Tail** | MMIO 寄存器 |

```
        Base ──→ [ desc0 | desc1 | ... | descN-1 ]
                    ↑ Head          ↑ Tail
                   (硬件维护)       (软件写)
```

- **硬件** 通常维护 Head（已处理位置）
- **软件** 通过移动 **Tail** 通知硬件「有新描述符可用」

**描述符格式（以 ixgbe Advanced RX Descriptor 为例）：**

```c
/* 16 字节 — 恰好 1/4 cache line */
union ixgbe_adv_rx_desc {
    struct {
        uint64_t pkt_addr;      /* mbuf 数据区物理地址 (DMA 写入目标) */
        uint64_t hdr_addr;      /* 头部缓冲区地址 (可选) */
    } read;                      /* 软件填写 — 告诉硬件写哪 */
    struct {
        uint32_t status_err;    /* DD 位、错误标志等 */
        uint16_t length;         /* 收到的包长度 */
        uint16_t vlan;           /* VLAN tag */
        /* ... 更多字段 ... */
    } wb;                        /* 硬件回写 — 收包后填入 */
};
```

---

### 二、接收 (RX)

```
1. 软件：将空闲 mbuf 的物理地址填入描述符 read.pkt_addr
2. 软件：写 Tail 寄存器 → 通知硬件有新描述符可用
3. 硬件：包到达 → DMA 写数据到 pkt_addr 指向的内存
4. 硬件：写回描述符 wb.status_err，置 DD 位 (Descriptor Done)
5. 软件：轮询 DD 位 → 取走 mbuf → 用新 mbuf 重填描述符
```

```c
/* DPDK PMD RX 轮询简化逻辑 */
uint16_t
ixgbe_rx_burst(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
    volatile union ixgbe_adv_rx_desc *rx_desc;
    struct rte_mbuf *mbuf;
    uint16_t nb_rx = 0;

    while (nb_rx < nb_pkts) {
        rx_desc = &rxq->rx_ring[rxq->rx_tail];

        /* 检查 DD 位 — 硬件是否已完成 DMA 写入 */
        if (!(rx_desc->wb.upper.status_error & IXGBE_RXDADV_STAT_DD))
            break;  /* 没有更多已完成的包 */

        /* 取出 mbuf — 数据已在其中（DMA 已写入） */
        mbuf = rxq->sw_ring[rxq->rx_tail];
        mbuf->pkt_len = rx_desc->wb.upper.length;
        mbuf->data_len = mbuf->pkt_len;
        mbuf->port = rxq->port_id;

        rx_pkts[nb_rx++] = mbuf;

        /* 重填描述符 — 用新 mbuf 替换 */
        struct rte_mbuf *new_mbuf = rte_pktmbuf_alloc(rxq->mb_pool);
        rxq->sw_ring[rxq->rx_tail] = new_mbuf;
        rx_desc->read.pkt_addr = rte_mbuf_data_iova(new_mbuf);
        /* 清 DD 位 — 准备下次收包 */
        rx_desc->wb.upper.status_error = 0;

        rxq->rx_tail = (rxq->rx_tail + 1) & (rxq->nb_rx_desc - 1);
    }

    /* 更新 Tail 寄存器 — 告诉硬件已消费到哪 */
    IXGBE_PCI_REG_WRITE(rxq->rdt_reg_addr, rxq->rx_tail);
    return nb_rx;
}
```

**关键细节：**
- `rte_mbuf_data_iova()` 返回 mbuf 数据区的 **IOVA（物理地址）** — 硬件 DMA 需要物理地址，不是虚拟地址
- Tail 寄存器写入是一次 **MMIO**（~50-100ns）— 所以 **批量更新** 而非每包更新
- DD 位轮询是对 **描述符内存** 的读 — 如果在 L1/L2 cache 中则很快

---

### 三、发送 (TX)

```
1. 软件：将待发送数据的物理地址/长度填入描述符
2. 软件：更新 Tail 寄存器 → 触发硬件发送
3. 硬件：DMA 读数据 → 通过网线发送
4. 硬件：写回 DD 位
5. 软件：检查 DD → 回收描述符 / 释放 mbuf
```

**TX 与 RX 的区别：** TX 是 DMA **读** 操作（从内存读到网卡），RX 是 DMA **写** 操作（从网卡写到内存）。

---

### 四、描述符深度与性能

| 参数 | 典型值 | 影响 |
|------|--------|------|
| RX desc count | 512-1024 | 太小 → 繁忙时丢包；太大 → cache 覆盖不住 |
| TX desc count | 512 | 太小 → 发送反压；太大 → 延迟增加 |
| BURST_SIZE | 32 | 每次轮询取多少包 — 太小开销大，太大延迟大 |

**HFT 调优：** 减小 RX desc count（如 128-256）以降低 **描述符环遍历延迟**；BURST_SIZE 用 32（延迟与效率的平衡点）。

---

### 五、与 DPDK PMD 的关系

- PMD **poll mode** 批量检查 DD、批量 refill — 减少 per-packet 寄存器访问（→ §4）
- 描述符环深度、Prefetch 与 [Ch2 Cache 预取](../../chapter-02-cache-and-memory/notes/section-3-Cache预取.md) 影响 miss 率
- **DD 位轮询** 是 PMD 的热路径核心 — 每次循环至少读一个 cache line 的描述符

 内核 NAPI 环对照 [12-kernel-networking](../../../../12-kernel-networking/) · PMD 深潜 [chapter-07 网卡性能优化](../../chapter-07-nic-performance-optimization/)

---

← [2. PCIe 事务与带宽](./section-2-PCIe事务与带宽.md) · 下一节 [4. CPU 与 I/O 优化](./section-4-CPU与IO协奏优化.md)
