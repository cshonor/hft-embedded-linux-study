# Ch 10 §1 页面替换策略 (Page Replacement Policy)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/mmzone.h` 的 `enum lru_list`、`mm/vmscan.c`）

---

## 本节讲什么

回收的第一步是「**选谁当 victim**」。本节回答：

1. Linux 为什么不用教科书上的纯 LRU？
2. 现代内核的 LRU 到底分几条链？——比原书「active/inactive 双链」多出什么？
3. 扫描时「扫匿名页还是文件页、扫多少」由谁决定？

原书以 2.4/2.6 的「`active_list` + `inactive_list`」双链叙述；v6.6 的 `enum lru_list`（`mmzone.h:263`）是**四象限 + unevictable 共 5 条**，且比例决策交给了基于 `anon_cost/file_cost` 的 `get_scan_count()`。

---

## 1. 为什么不用纯 LRU

| 纯 LRU 的问题 | Linux 的折中 |
|--------------|-------------|
| 每次访问都要把页移到链表头 → **热路径锁争用** | 用 `PG_referenced`/`PG_active` 标志做「**二次机会**」，不每次动链 |
| 全局一条链 → 多核大锁 | LRU 按 **lruvec（node × memcg）** 分片，锁粒度极小 |
| 不知道「匿名页 vs 文件页」谁更该踢 | 分 **anon / file 两条**，用成本模型决定扫描比例 |

所以 Linux 用的是「**带二次机会的近似 LRU**」——`struct page/folio` 上的 `PG_referenced` 位相当于 clock 算法的引用位，`PG_active` 位决定它在 active 还是 inactive 链上。

---

## 2. v6.6 的 LRU：四象限 + unevictable（`mmzone.h:263`）

```c
enum lru_list {
    LRU_INACTIVE_ANON = LRU_BASE,
    LRU_ACTIVE_ANON   = LRU_BASE + LRU_ACTIVE,
    LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
    LRU_ACTIVE_FILE   = LRU_BASE + LRU_FILE + LRU_ACTIVE,
    LRU_UNEVICTABLE,          /* ← mlock 的页住这里 */
    NR_LRU_LISTS
};
```

```
              anon（匿名页）          file（文件页）
   active    LRU_ACTIVE_ANON      LRU_ACTIVE_FILE     ← 工作集
   inactive  LRU_INACTIVE_ANON    LRU_INACTIVE_FILE   ← 回收候选
                        LRU_UNEVICTABLE               ← mlock / 不可回收
```

**为什么 anon 和 file 必须分开？** 回收成本完全不同：文件页脏了要 writeback（成本高但可再读回），匿名页要 swap out（成本最高，要写 swap 设备）。分开后内核能**按成本模型**决定先扫哪边，而不是把两类混在一起盲扫。

**`LRU_UNEVICTABLE` 是 HFT 的关键：** `mlock()` 钉住的页就挂这条链，**回收扫描根本不碰它**。这是 `mlock` 防换出的机制落点——不是「标记成不可回收」，而是「物理上不在可回收链表里」。

---

## 3. 扫描比例：`get_scan_count()` 与成本模型

`shrink_lruvec()`（`vmscan.c:6273`）里先调 `get_scan_count(lruvec, sc, nr)`，算出每条 LRU 该扫多少页。v6.6 的决策基于 `struct lruvec`（`mmzone.h:614`）里的两个字段：

```c
struct lruvec {
    struct list_head lists[NR_LRU_LISTS];  /* 5 条链表 */
    spinlock_t lru_lock;
    unsigned long anon_cost;   /* 回收一个匿名页的平均成本 */
    unsigned long file_cost;   /* 回收一个文件页的平均成本 */
    atomic_long_t nonresident_age;
    unsigned long refaults[ANON_AND_FILE];  /* 工作集 refault 检测 */
};
```

**直觉：** 哪边回收便宜（cost 低），就多扫哪边。`swappiness`（`/proc/sys/vm/swappiness`）就是在这里影响 anon/file 的倾向——`swappiness=0` 表示「尽量别 swap 匿名页，先回收文件页」。这比原书「active 恒占 2/3」的固定比例**动态得多**。

---

## 4. active ↔ inactive 迁移（二次机会）

| 事件 | 迁移 |
|------|------|
| 页被访问（PTE accessed 位置位） | inactive → active（被「救」回工作集） |
| active 尾部页长时间未访问 | active → inactive（`shrink_active_list`，`vmscan.c:2688`） |
| 页被 `mlock` | 任何链 → `LRU_UNEVICTABLE` |

**二次机会机制：** `PG_referenced` 是「最近被访问过」的信号。回收扫描时，若页有 `PG_referenced`，先清掉标志、把页**放回**（给它第二次机会），下次扫到还没被访问才真正回收。这正是 clock 算法的思想内核。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| `mlock` 防换出 | 页进 `LRU_UNEVICTABLE`，扫描根本不碰 |
| `vm.swappiness=0` | `get_scan_count()` 里 anon/file 比例极偏文件页 |
| 回收抖动 | 回收器误踢工作集 → refault → 再读回，二次抖动 |
| 工作集保护 | `refaults[]` 检测「刚回收又被 fault 回来」，动态调大 active |

---

## 6. 衔接

- 下节 [§2 页缓存](./section-2-页缓存.md)：file 链上挂的到底是什么
- [§3 LRU 链表管理](./section-3-LRU-链表管理.md)：隔离、回收、放回的三步实现
- [§6 kswapd](./section-6-页面换出守护进程.md)：谁在驱动这套扫描
- 前置：[Ch 2 §2 水位](../../chapter-02-describing-physical-memory/notes/section-2-内存区域.md)（扫描的触发条件）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 anon 和 file 页要分到不同的 LRU？**
A：回收成本不同。文件页脏了 writeback 到文件系统即可，干净的直接丢；匿名页没有后备存储，必须写 swap 设备（更慢、还要占 swap 空间）。分开后 `get_scan_count()` 能按 `anon_cost/file_cost` 决定先扫便宜的，避免「为了回收一个匿名页而放过十个可轻松回收的文件页」。

**Q2：`LRU_UNEVICTABLE` 和「标记页不可回收」是一回事吗？**
A：机制上更强。不是打标记，而是把页**从可回收链表物理移除**，挂到一条扫描路径根本不会遍历的链上。`mlock` 的页、以及某些驱动 pin 住的页都在这。所以回收器的 for 循环里压根见不到它们。

**Q3：`PG_referenced` 和 `PG_active` 两个标志什么区别？**
A：`PG_referenced` 是**瞬态信号**（「最近被访问过」，类似 clock 引用位，扫描时消费）；`PG_active` 是**状态归属**（「我住在 active 链还是 inactive 链」）。referenced 累积到一定程度才触发 inactive→active 迁移。

**Q4：`swappiness` 具体怎么影响扫描？**
A：它是 `get_scan_count()` 计算 anon/file 扫描比例的输入之一。`swappiness=0` 时（现代语义），内核几乎只扫 file 链、尽量不 swap 匿名页；`swappiness=100` 时 anon 和 file 同等对待。注意：它只是**倾向**，不是硬约束——内存极度紧张时仍可能 swap。

**Q5：为什么 LRU 要按 lruvec（node × memcg）分片，而不是全局一条？**
A：两个原因。① NUMA：每个 node 的回收应独立，避免跨 socket 锁竞争；② memcg：容器/进程组的**内存限额**要靠 per-memcg 的 LRU 来实现「谁超限回收谁」，全局一条链做不到。分片后每个 lruvec 一把小锁，热路径争用降到最低。

</details>

---
