## ④ 内存区域的链表与树

单个 `mm_struct` 可能含 **成百上千 VMA** — 内核用 **链表 + 红黑树** 双重索引：**遍历** vs **按地址查找**。

#### 两种结构

| 结构 | 字段 | 复杂度 | 用途 |
|------|------|--------|------|
| **单向链表** | **`mmap`** | O(n) 遍历 | **`/proc/maps` 输出**、**全扫描** |
| **红黑树** | **`mm_rb`** | O(log n) 查找 | **`find_vma`**、**page fault**、**mmap 冲突检测** |

```
mm_struct
    │
    ├── mmap ──► VMA_a ──► VMA_b ──► VMA_c ──► ...   （链表，按 vm_start 排序）
    │
    └── mm_rb （红黑树，按 vm_start 键）
              │
              └── 快速：给定 addr，找覆盖它的 VMA
```

#### 为何需要两种

| 操作 | 频率 | 结构 |
|------|------|------|
| **缺页 / `find_vma(addr)`** | **极高** | **红黑树** |
| **munmap 合并相邻 VMA** | 中 | 链表 + 树 **同步更新** |
| **调试 dump maps** | 低 | **链表 walk** |

#### 与 Ch 6 / Ch 4 同构

| 子系统 | 数据结构 |
|--------|----------|
| **VMA 管理** | **`mm_rb` 红黑树** |
| **CFS 调度** | **`vruntime` 红黑树** |
| **epoll / interval tree** | 其他 **树形索引** |

#### `mmap_cache` 优化（Ch 15.5）

| 事实 | 说明 |
|------|------|
| **局部性** | 连续 **fault** 常落在 **同一 VMA** |
| **`mmap_cache`** | 缓存 **上次 find_vma 结果** — **顺序访问** 快路径 |

#### 插入 / 删除不变量

| 不变量 | 原因 |
|--------|------|
| **VMA 不重叠** | 同一 addr **至多一个** VMA |
| **按 `vm_start` 排序** | **`find_vma` 语义** |
| **链表与树一致** | 增删 **同时维护** 两结构 |

#### 维护双结构的代价（LKD 时代已嫌重）

双结构意味着 `mmap`/`munmap`/`mprotect` 路径每次都要做 **两份簿记**：

```
vma_link(vma):
    ① __vma_link_list()   ← 链表插入 O(1)（已知前驱）
    ② __vma_link_rbtree() ← 红黑树插入 O(log n) + rebalance
    ③ 两个结构任何一处漏改 = 内核内存管理静默损坏
```

这正是后来被 **单一结构**（maple tree，见下节）取代的动机之一：一份簿记、缓存更友好、还顺带解决了锁竞争。

---

### 现代内核演化：maple tree 取代双结构（v6.1+）

> **⚠️ 版本断崖（核对 v6.6 源码 `include/linux/mm_types.h` / `mm/mmap.c`）：**
> 本节上半部分（`mmap` 链表 + `mm_rb` 红黑树 + `mmap_cache`）是 **LKD3rd（2.6 时代）** 的实现。
> **v6.1 起三者全部移除**，`mm_struct` 里只剩 `struct maple_tree mm_mt`（mm_types.h:690）。
> 读内核源码时按版本对号入座，别在新内核里找 `mm_rb`。

#### 演化时间线

| 阶段 | 结构 | 缺点 |
|------|------|------|
| 2.6（LKD3rd） | `mmap` 链表 + `mm_rb` 红黑树 + 单条 `mmap_cache` | 双份簿记；缓存一条不够用 |
| ~3.16 | **vmacache**（每 task 4 条 VMA 缓存）取代单条 `mmap_cache` | 缓存仍只是"贴膏药"，树本身还是慢 |
| **v6.1** | **maple tree `mm_mt`** 取代链表 + 红黑树（Liam Howlett） | —— |
| v6.4 | **per-VMA lock**（`CONFIG_PER_VMA_LOCK`）进一步解决缺页锁竞争 | —— |

#### v6.6 源码证据

```c
// include/linux/mm_types.h —— mm_struct 里 VMA 的唯一索引：
struct maple_tree mm_mt;              // ← 没有 mm_rb，没有 mmap 链表头

// mm/mmap.c —— find_vma 就是 mt_find：
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
        unsigned long index = addr;
        mmap_assert_locked(mm);
        return mt_find(&mm->mm_mt, &index, ULONG_MAX);
}

// include/linux/mm_types.h —— vm_area_struct 开头注释直说了设计意图：
// "The first cache line has the info for VMA tree walking."
union {
        struct {
                unsigned long vm_start;
                unsigned long vm_end;
        };
        struct rcu_head vm_rcu;       // CONFIG_PER_VMA_LOCK 下延迟释放
};
```

注意 **`vm_area_struct` 里已经没有 `vm_next`/`vm_prev`** —— 链表指针根本不存在了；第一缓存行专门留给树查找要用的字段。

#### maple tree 是什么

| 特性 | 说明 |
|------|------|
| 本质 | **B-tree 的特化变体**（R-tree 思路），节点放 **多个 VMA 的范围条目**（不像红黑树一节点一条） |
| 查找 | 每层下降比较**多个**键 → **树更矮**、**缓存行命中更高**——为 `find_vma` 这种"给地址找区间"的场景定制 |
| 范围操作 | 原生支持区间查询/区间替换——`munmap` 挖洞、`mprotect` 改一段，不再是"链表+树各改一遍" |
| RCU | 读侧可无锁遍历（配合 per-VMA lock） |
| prealloc | 写路径先 `vma_iter_prealloc()` 预分配节点，**避免在持锁时分配内存**（v6.6 mmap.c 里 `vma_link`/`vma_expand`/`vma_shrink` 全是这个模式） |

#### 新旧对照

| 操作 | LKD 双结构 | maple tree（v6.1+） |
|------|-----------|---------------------|
| `find_vma(addr)` | vmacache 命中 O(1) / 未命中红黑树 O(log n) | `mt_find` O(log n) 但树矮、缓存友好——**平均更快** |
| 全量遍历（`/proc/maps`） | 链表 O(n) 顺序走 | 树序遍历（`vma_iter`） |
| `mmap` 插入 | **两份**簿记（list + rbtree + rebalance） | **一份**（`vma_iter_store`） |
| `munmap` 挖洞 | 拆 VMA + 双结构同步 | 区间 clear/store，一步到位 |
| 读侧扩展性 | 全程 `mmap_lock` | RCU + **per-VMA lock**（缺页只锁那一个 VMA，v6.4+） |

**HFT：** 运行时 **VMA 数量应稳定** — **启动期 `mmap` 全部 ring**，盘中 **不再 munmap/mmap**（避免 **树 rebalance** 与 **锁**）。若用 **`MAP_FIXED`** 固定 VA，**重复映射** 仍触发 **`do_munmap` + 新建** — 应 **一次到位**。maple tree + per-VMA lock 把"多线程同时缺页"的锁竞争大幅降低，但**热路径零 fault**（`MAP_POPULATE` + `mlock`）仍是第一原则——数据结构再快也不如不去碰它。

→ [Ch 6 内核数据结构](../../chapter-06-kernel-data-structures/) · [Ch 4 CFS rbtree](../../chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md) · [15.5 find_vma](./section-15.5-操作内存区域.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 为什么 VMA 同时用链表和红黑树两种结构？

<details><summary>答案</summary>

链表：按地址顺序遍历所有 VMA（如 /proc/maps 输出、munmap 合并检查）。红黑树：按起始地址查找特定 VMA（O(log n)，如缺页处理时找 addr 属于哪个 VMA）。两种结构各有优势：遍历用链表 O(n)，查找用红黑树 O(log n)。mmap/munmap 频繁操作 VMA，需要高效数据结构。

</details>

**Q2.** LKD3rd 描述的双结构在现代内核（v6.1+）发生了什么变化？为什么？

<details><summary>答案</summary>

v6.1 起 `mmap` 链表 + `mm_rb` 红黑树 + vmacache 全部移除，`mm_struct` 只剩 **maple tree `mm_mt`**（v6.6 mm_types.h:690）。动机：① 双结构 = 每次增删**两份簿记**、易漏改；② maple tree 是 B-tree 特化变体，节点存**多条范围条目**，树更矮、缓存行命中更高，专为"给地址找区间"定制；③ 原生区间操作让 munmap 挖洞一步到位；④ 配合 v6.4 的 per-VMA lock（`CONFIG_PER_VMA_LOCK`），缺页路径只锁单个 VMA 而非整个 `mmap_lock`。`vm_area_struct` 中 `vm_next`/`vm_prev` 已不存在，第一缓存行专门留给树查找字段（源码注释原文："The first cache line has the info for VMA tree walking."）。

</details>

**Q3.** 写路径上 `vma_iter_prealloc()` 的设计意图是什么？

<details><summary>答案</summary>

在**持有写锁之前/期间不分配内存**：maple tree 写入可能需要新节点，若在持锁后才分配，分配本身可能睡眠或失败，导致**持锁路径回退复杂化**。所以 v6.6 的 `vma_link`/`vma_expand`/`vma_shrink` 全是"先 `vma_iter_prealloc()` 预分配节点，再 `vma_iter_store()` 提交"的模式——**把可能失败/睡眠的操作挪到锁外**，锁内只剩确定成功的提交。这个思想与用户态"临界区内不做 IO/分配"完全同构。

</details>

</details>
---
