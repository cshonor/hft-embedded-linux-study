## 9.9.14 分离空闲链表

> **Ch9 §9.9.14** · [章导读](../README.md) · 上节 [§9.9.13 ←](./section-9.9.13-显式空闲链表.md) · 下节 [§9.10 →](./section-9.10-垃圾收集.md)

---

← [本章导读](../README.md)

---

### 分离空闲链表（Segregated Free Lists）

- **核心：** 按块大小分类，每类一个空闲链表
- **查找：** 根据请求大小定位对应大小类的链表 → 只在该链表内搜索

**大小类划分示例：**
| 链表 | 大小范围 |
|------|----------|
| 0 | 1–16B |
| 1 | 17–32B |
| 2 | 33–64B |
| ... | 2 的幂 |
| k | > 2^(k+3) |

- **glibc ptmalloc：** 用分离适配（segregated fit），每个大小类一条链表
- **tcmalloc/jemalloc：** 更细粒度 + 线程局部缓存（thread-local cache）

**优势：** find_fit ≈ O(1)（直接定位大小类 + 链表短）

### 常见陷阱
1. **分离链表 = 多个链表按大小分类，不是一条链** — 每个大小类一个独立链表
2. **桶数选择是 tradeoff** — 多桶查找快但管理开销大（每个桶维护 prev/next）；少桶省空间但桶内链表长
3. **glibc 的 ptmalloc 用分离适配** — 但不是最优；tcmalloc/jemalloc 用线程局部缓存进一步减少锁竞争

### 自测题

<details>
<summary>Q1: 分离空闲链表如何提升查找效率？</summary>

按块大小分桶，每个大小类一条链表。malloc 时根据请求大小直接定位对应桶，只在该桶内搜索。桶内链表短，查找接近 O(1)。

</details>

<details>
<summary>Q2: 大小类（size class）如何划分？</summary>

通常按 2 的幂分组：1-16、17-32、33-64、65-128... 每个组对应一条链表。大请求搜更大的桶，小请求搜更小的桶。

</details>

<details>
<summary>Q3: glibc malloc 用什么策略？和 tcmalloc/jemalloc 有何区别？</summary>

glibc ptmalloc 用分离适配（多大小类链表）。tcmalloc/jemalloc 在此基础上加线程局部缓存（thread-local cache），减少锁竞争，多线程性能更好。

</details>

<details>
<summary>Q4: HFT 应该用哪个分配器？为什么？</summary>

最佳方案：自己实现固定大小对象池（无锁、无碎片、O(1)）。如果用通用分配器：jemalloc（线程局部缓存减少锁竞争，低尾延迟）。绝不在线程间共享一个 malloc arena。

</details>

---

← [§9.9.13 ←](./section-9.9.13-显式空闲链表.md) · [本章导读](../README.md) · [§9.10 →](./section-9.10-垃圾收集.md)
