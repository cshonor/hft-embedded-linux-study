## 4. LRU 链表 (Active / Inactive Lists)

> 区分 **in-use** vs **unused** — 只从尾部收 **冷页**

---

### 一、双链表结构

用户进程页 + 页缓存页 组织进：

| 链表 | 含义 |
|------|------|
| **Active（活跃）** | 近期 **被访问** |
| **Inactive（非活跃）** | 较久 **未访问** — PFRA **优先收尾部** |

每个 **内存节点 (NUMA node)** 可有独立 LRU 列表。

---

### 二、页描述符标志

`struct page` 上相关位（ULK 2.6）：

| 标志 | 作用 |
|------|------|
| **`PG_lru`** | 页在 LRU 链上 |
| **`PG_active`** | 在 active 还是 inactive |
| **`PG_referenced`** | 近期被访问过（硬件/accessed 位等配合） |

---

### 三、链表间移动

| 函数（概念） | 行为 |
|--------------|------|
| **`mark_page_accessed()`** | 访问时标记 — 可能 **promote** 到 active |
| **`refill_inactive_zone()`** | active **过多** 时 **降级** 页到 inactive |

**效果：** 只有 **长时间未访问** 的非活跃链表 **尾部** 页成为回收候选。

HFT：热数据应 **常驻 active**；冷启动后大量 **inactive** 页可被收 — 理解 **latency spike** 来源。

---

← [3. 反向映射](./section-3-反向映射.md) · 下一节 [5. 触发与 OOM](./section-5-执行时机与OOM.md)
