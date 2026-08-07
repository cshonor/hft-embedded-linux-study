## 9.9.13 显式空闲链表

> **Ch9 §9.9.13** · [章导读](../README.md) · 上节 [§9.9.12 ←](./section-9.9.12-简单分配器综合.md) · 下节 [§9.9.14 →](./section-9.9.14-分离空闲链表.md)

---

- 空闲块内 **指针** 串成链表 — 只遍历空闲块，更快

---

### 显式空闲链表

- **核心思想：** 空闲块内存 prev/next 指针，串成双向链表
- **优势：** find_fit 只遍历空闲块，跳过已分配块 → 快很多
- **结构：**
  ```
  空闲块：[header][prev][next][...padding...][footer]
  已分配块：[header][payload][footer]  （无 prev/next，省空间）
  ```
- **插入策略：**
  - **LIFO** — 新释放块插表头，O(1) 插入，但碎片多
  - **FIFO** — 插表尾，碎片分布均匀

| 对比 | 隐式链表 | 显式链表 |
|------|----------|----------|
| find_fit | O(总块数) | O(空闲块数) |
| 空闲块开销 | header+footer | +prev+next（8-16B） |
| 已分配块开销 | header+footer | header+footer（不变） |

### 常见陷阱
1. **显式链表只在空闲块存指针，已分配块不占额外空间** — 指针存在空闲块的 payload 区域
2. **LIFO vs FIFO 插入策略影响碎片分布** — LIFO 快但碎片集中在表头；FIFO 均匀但插入慢
3. **指针占用空闲块的有效载荷空间** — 最小空闲块要容纳 header+prev+next+footer

### 自测题

<details>
<summary>Q1: 显式空闲链表和隐式链表的核心区别？</summary>

显式链表在空闲块的 payload 中存 prev/next 指针，将所有空闲块串成双向链表。find_fit 只遍历空闲块，跳过已分配块，速度从 O(总块数) 提升到 O(空闲块数)。

</details>

<details>
<summary>Q2: 已分配块需要存 prev/next 指针吗？为什么？</summary>

不需要。已分配块不会被 find_fit 遍历，不需要链表指针。空闲时才在 payload 区域写入 prev/next，分配时这些空间归还给用户。

</details>

<details>
<summary>Q3: LIFO 和 FIFO 插入策略各自的优缺点？</summary>

LIFO：释放时插表头，O(1)，但反复分配释放同大小块会在表头形成碎片聚集。FIFO：插表尾，碎片分布均匀，但需要遍历到表尾（或维护尾指针）。

</details>

<details>
<summary>Q4: 显式链表的最小空闲块大小由什么决定？</summary>

必须容纳 header + prev 指针 + next 指针 + footer。16B 对齐时，最小空闲块通常 16-24B，比隐式链表的最小块大。

</details>

---

← [§9.9.12 ←](./section-9.9.12-简单分配器综合.md) · [本章导读](../README.md) · [§9.9.14 →](./section-9.9.14-分离空闲链表.md)
