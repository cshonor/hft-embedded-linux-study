## 3. 两大转发框架模型

> 思想源自专用 **网络处理器 (NP)**；DPDK 将其移植到 **通用 IA 多核**

---

### 一、Run to Completion (运行至终结)

| 特点 | 说明 |
|------|------|
| **一核一线** | 每个 lcore 负责报文 **完整生命周期**（RX → 处理 → TX） |
| **绑定** | EAL `-l` **绑核** |
| **优点** | 编程 **简单**、**横向扩展** 直观（加核加吞吐） |
| **缺点** | 单包逻辑 **耦合** 在一核；难对 **单一阶段** 做专用优化 |

**RTC 模式代码结构：**

```c
/* Run to Completion — 每个 lcore 独立完成全流程 */
static int
rx_worker_tx(void *arg)
{
    uint16_t port_id = *(uint16_t *)arg;
    struct rte_mbuf *bufs[BURST_SIZE];

    while (!force_quit) {
        /* 1. 收包 */
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0) continue;

        /* 2. 处理 — 在同一核上完成 */
        for (int i = 0; i < nb_rx; i++) {
            struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], void *);
            /* 解析 → 查表 → 修改 → 准备发送 */
            eth->dst_addr = eth->src_addr;  /* 简单回转示例 */
        }

        /* 3. 发送 — 同一核 */
        rte_eth_tx_burst(port_id, 0, bufs, nb_rx);
    }
    return 0;
}
```

**HFT 常见：** 行情 tick 路径 **单核 RTC** — 最小跨核、最小队列延迟。典型 tick-to-trade 路径：

```
NIC RX queue → PMD poll → parse L2/L3/L4 → decode market data
  → update orderbook → signal decision → send order → NIC TX
                    ↑ 全在一个 lcore 上 ↑
```

 [Ch8 Run-to-Completion 结合](../../chapter-08-flow-classification-multiqueue/notes/section-2-网卡多队列.md)

---

### 二、Pipeline (流水线 / Packet Framework)

借鉴 **工业流水线**：处理拆成多个 **逻辑功能单元**，分布在同核或不同核，**队列传递** mbuf。

**三大要素：**

| 要素 | 角色 | DPDK API |
|------|------|----------|
| **Port（逻辑端口）** | 报文进出 Pipeline 的抽象端点 | `rte_port_*` 系列 |
| **Table（查找表）** | 匹配 — Hash / LPM / ACL 等 | `rte_table_hash`, `rte_table_lpm` |
| **Action（处理逻辑）** | 命中后的修改、转发、丢弃 | `rte_pipeline_action` |

```
Stage 1 (lcore 0)     Stage 2 (lcore 1)     Stage 3 (lcore 2)
┌──────────┐          ┌──────────┐          ┌──────────┐
│  RX +    │  rte_ring │  Lookup  │  rte_ring │  Modify  │
│  Parse   │─────────→│  (Hash/  │─────────→│  + TX    │
│          │  SP/SC   │  LPM)    │  SP/SC   │          │
└──────────┘          └──────────┘          └──────────┘
```

- Stage 间通常经 **rte_ring** — [Ch4 无锁 ring](../../chapter-04-synchronization/notes/section-5-无锁机制.md)
- 每个 stage 可独立扩展（多个 lcore 跑同一 stage）

**HFT Pipeline 适用场景：** 多策略引擎 — 行情解析 stage → N 个策略 stage 并行 → 订单汇聚 stage。

---

### 三、选型对照

| | **Run to Completion** | **Pipeline** |
|---|----------------------|--------------|
| 延迟 | 通常 **更低**（无 stage 排队） | 多 stage 可能增加 **排队延迟** |
| 吞吐扩展 | 加 **完整 pipeline 副本**（多核各跑全流程） | 对 **热点 stage** 单独 **水平扩展** |
| 代码复杂度 | 低 | 高 — 需定义 Port/Table/Action |
| Cache 局部性 | 好 — 数据在一个核的 cache 中流转 | 差 — mbuf 在核间传递，每次跨核 cache miss |
| 典型产品 | 简单 L3 转发、低延迟网关 | 复杂分类、多表 lookup 产品 |

```
简单 + 低延迟     →  RTC + RSS（Ch8）
复杂 Match/Action →  Pipeline + rte_table_*
HFT tick 路径     →  RTC（单核全流程，零队列延迟）
HFT 多策略        →  Pipeline（解析 1 核 → 策略 N 标并行 → 汇聚 1 核）
```

---

← [2. 处理模块划分](./section-2-网络处理模块划分.md) · 下一节 [4. 转发算法](./section-4-核心转发算法.md)
