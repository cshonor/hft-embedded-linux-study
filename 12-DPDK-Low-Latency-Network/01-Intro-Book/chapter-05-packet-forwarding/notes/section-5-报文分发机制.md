## 5. 报文分发机制 (Packet Distributor)

---

### 一、角色

多核并发下，DPDK **Packet Distributor** 库负责 **收包侧负载均衡**：

```
        ┌─────────────┐
  RX ──→│ Distributor │──→ Worker 0
        │   (分发核)   │──→ Worker 1
        └─────────────┘──→ Worker N
```

- **Distributor 核**：从 RX 取 mbuf，**分给多个 Worker 核** 处理
- **Worker 核**：执行业务逻辑（查表、修改、转发等）

---

### 二、同流同 Worker（保序）

**关键：** 记录 mbuf 中的 **tag（哈希值）**，保证 **相同 stream 的报文总是落到同一 Worker**。

| 收益 | 说明 |
|------|------|
| **避免乱序** | 同一会话/五元组流在单 Worker 内 **顺序处理** |
| **有状态逻辑** | 会话表无需跨核同步 |
| **重排序** | 处理完后可配合 **排序库** 恢复全局顺序（若需要） |

与 **RSS 对称哈希**（[Ch8](../chapter-08-flow-classification-multiqueue/notes/section-3-硬件流分类.md)）对比：

| | **硬件 RSS** | **软件 Distributor** |
|---|-------------|------------------------|
| 分流点 | **网卡 / PMD 入队前** | **应用层** 分发核 |
| 成本 | 零 CPU 分流（理想） | 占用 **1 个 Distributor 核** |
| 灵活性 | 受 NIC 哈希字段限制 | tag 规则 **可编程** |

---

### 三、HFT 实践

- **低 PPS、强保序：** 单核 RTC 或 RSS **单队列** — 不用 Distributor  
- **高 PPS、多 Worker：** RSS 优先；需 **自定义流键** 时用 Distributor + tag  
- **tail latency：** Distributor 成为 **单点** — 需 **批量分发**、Worker 数与核数匹配 [Ch3 Gustafson](../chapter-03-parallel-computing/notes/section-2-多核性能与可扩展性.md)

---

← [4. 转发算法](./section-4-核心转发算法.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
