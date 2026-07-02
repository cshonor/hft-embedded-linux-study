## 4. 核心转发算法

---

### 一、精确匹配 (Exact Match)

基于 **哈希** — 常用 **CRC32**、**J hash** 等。

| 要点 | 说明 |
|------|------|
| **冲突处理** | 融合 **分离链表** 与 **开放寻址** 优点 |
| **硬件加速** | IA **SSE4.2** `CRC32` 指令（如 `CRC32Q`）一次性算 key |
| **multi-buffer** | 批量算多个 key，**降低指令依赖**、提高 IPC → [Ch3 SIMD](../chapter-03-parallel-computing/notes/section-4-数据并行与SIMD.md) |

**场景：** 流表、MAC/VLAN 精确表、会话表。

---

### 二、最长前缀匹配 (LPM)

**IPv4 路由** 典型算法 — DPDK 策略：**空间换时间**。

```
tbl24：2^24 条目（第一级）
tbl8 ：多张 2^8 条目（第二级，按需挂接）
```

| 前缀长度 | 访存次数 |
|----------|----------|
| **≤ 24 位**（绝大多数路由） | **1 次** — 仅查 tbl24 |
| **> 24 位** | **2 次** — tbl24 → tbl8 |

→ 极大提升 **路由查找** 性能；表创建/查找受 [Ch4 rwlock](../chapter-04-synchronization/notes/section-3-读写锁.md) 保护（读多写少）。

---

### 三、ACL (访问控制列表)

**N 元组** 规则匹配 — 防火墙、分类器常用。

- 按 **Tier（层）** 建数据结构
- 匹配字段 **每个字节** 作为 Tier 中 **一层** 做路径匹配
- 与 Pipeline **Table + Action** 天然契合

---

### 四、算法选型速查

| 需求 | 算法 | DPDK 库 |
|------|------|---------|
| 精确五元组 / key-value | Hash + 冲突链 | `rte_hash` |
| IPv4 最长前缀路由 | tbl24/tbl8 LPM | `rte_lpm` |
| 多维规则、防火墙 | Tier ACL | `rte_acl` |

**HFT：** 行情网关若只做 **固定组播订阅**，可能 **无需 LPM**；跨 VLAN/多源过滤可用 **Hash 会话表** 或 **ACL**。

→ [02-Hennessy 存储器/并行](../../../02-Computer-Architecture-6th/) · [01-CSAPP 哈希/缓存](../../../01-CSAPP-3rd/)

---

← [3. 转发框架](./section-3-转发框架模型.md) · 下一节 [5. 报文分发](./section-5-报文分发机制.md)
