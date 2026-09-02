# Ch 10 §7 2.6 内核的新变化 (What's New in 2.6)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/vmscan.c` 的 `lru_gen_shrink_lruvec`、`mm/swap.c` 的 `lru_add_drain`）

---

## 本节讲什么

原书总结 2.6 相对 2.4 的三点变化，本节：

1. 讲清这三点各自解决什么
2. 用 v6.6 源码把每点做实
3. 补一条「从 2.6 到 v6.6 回收器又进化了什么」

---

## 1. 原书三变化（2.4 → 2.6）

| 改进 | 解决什么 |
|------|----------|
| **LRU 按 Zone 维护** | 2.4 全局一条 active/inactive，回收顺序全局竞争 → 2.6 每 `struct zone` 一套，按 zone 局部回收 |
| **Pageout pressure 衰减平均** | 不用简单 priority 跳变 → **decaying average** 控制扫描强度，回收更平滑、少突发 |
| **`pagevec` 批量 LRU** | 2.4 每次改 LRU 抢全局锁 → 2.6 局部向量攒批，一次性链入/链出，降锁争用 |

---

## 2. v6.6 里这三点的现状

| 2.6 的做法 | v6.6 的演进 |
|-----------|-------------|
| LRU 按 zone 维护 | 进一步下沉到 **lruvec（node × memcg）**，粒度更细、锁更小 |
| decaying average 控制扫描 | 升级为 `anon_cost/file_cost` **成本模型**（§1），比例动态 |
| `pagevec` 批量 LRU | 演化为 `lru_add_drain()`（`mm/swap.c:646`）的 **per-CPU 攒批 + 批量排干**（§3） |

---

## 3. 从 2.6 到 v6.6：回收器的新武器

| 新特性 | 作用 | 源码落点 |
|--------|------|----------|
| **folio 化** | 多页 folio 替代单页，减少元数据开销 | `shrink_folio_list`（`vmscan.c`） |
| **lru_gen（多代 LRU）** | 把页按「世代」分层，近似最优 LRU，取代二次机会近似 | `lru_gen_shrink_lruvec`（`vmscan.c:5497`） |
| **workingset refault** | 统计「回收后又被 fault 回来」的频率，动态纠错 | `lruvec->refaults[]`（§5） |
| **dirty/writeback tag** | xarray tag 快速定位脏页，不必线性扫描 | `PAGECACHE_TAG_DIRTY`（§2） |

**lru_gen 值得单说一句：** 传统的 active/inactive + 二次机会是「近似 LRU」，有精度损失。`CONFIG_LRU_GEN`（v6.1 起默认开启）把页按**世代（generation）**组织，老化（aging）时推进世代、回收时踢最老世代——**更接近真正的 LRU**，代价是实现复杂度高。

---

## 4. 回收决策简图（v6.6 版）

```
__alloc_pages 需要 free 页
        │
        ├─ 水位 > LOW？ → 直接分，无回收
        │
        ├─ 水位 < LOW → wakeup_kswapd()（后台）
        │        kswapd → balance_pgdat → shrink_node
        │                    ├─ shrink_lruvec → get_scan_count 定比例
        │                    │     ├─ shrink_inactive_list（isolate→shrink→放回）
        │                    │     │     └─ shrink_folio_list：丢/写回/swap
        │                    │     └─ shrink_active_list（降级）
        │                    └─ shrink_slab → slab/dcache/icache shrinker
        │
        └─ 水位 < MIN → __alloc_pages 同步 direct reclaim（调用方阻塞）
                          try_to_free_pages → do_try_to_free_pages
```

---

## 5. HFT 精读 checklist

| 手段 | 目的 |
|------|------|
| `mlock` / `mlockall` | 进程页进 unevictable，不被 swap、不被回收 |
| 足够物理 RAM | 避免 kswapd / direct reclaim 常转 |
| `vm.swappiness=0` | 回收时倾向文件页、少 swap 匿名页（不替代 mlock） |
| 监控 vmstat | `allocstall`、`pgscan_direct`、`pgmajfault`、`workingset_refault` |
| 避免热路径大分配 | 减少分配触发的同步 direct reclaim |
| 理解 page cache | mmap 只读行情文件可被回收；私有 dirty 要 writeback |

**Gorman HFT 捷径终点：** Ch 2 → 3 → 8 → 4 → **Ch 10**——「内存为什么会抖」的内核侧答案，在本章与 Ch 2 水位、Ch 4 fault、Ch 6 分配**闭合**。

---

## 6. 衔接

- 上节 [§6 kswapd](./section-6-页面换出守护进程.md)
- 下章：[Ch 11 交换管理](../../chapter-11-swap-management/)（swap 槽位、swap cache 细节）
- 前置：[Ch 2 §5 2.6 新变化](../../chapter-02-describing-physical-memory/notes/section-5-2.6-内核的新变化.md)（同源的多核优化脉络）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：LRU 从「全局」到「zone」再到「lruvec」，每次细化的动机是什么？**
A：都是**缩小锁的半径**。全局一条链 = 一把大锁 + 跨 NUMA 竞争；per-zone = 每 zone 独立回收；lruvec（node × memcg）= 锁细到「某节点某 cgroup」，热路径争用降到最低，同时支持 memcg 限额回收。

**Q2：lru_gen（多代 LRU）和传统 active/inactive 二次机会差在哪？**
A：传统是「两桶 + 引用位」的近似 LRU，精度有限（页可能长期卡在中间态）。lru_gen 按**世代**组织：aging 把页推到新世代，回收踢最老世代，**更接近真正 LRU 的「越久没访问越该被踢」**。代价是实现复杂、内存开销略高。

**Q3：`lru_add_drain` 和 2.6 的 `pagevec` 是什么关系？**
A：一脉相承。`pagevec` 是 2.6 引入的「批量操作 LRU 的向量」；v6.6 的 per-CPU lru 缓存 + `lru_add_drain()` 是它的现代形态——页先攒在 per-CPU，攒够批量入链，扫描前排干。思想完全一致：**批处理换锁**。

**Q4：workingset refault 检测解决什么问题？**
A：解决「回收器不知道自己是收对了还是收错了」。传统回收器踢完就完事，误踢工作集会导致「回收→读回」抖动。refault 统计「回收后多久又被 fault 回来」，频繁 refault 就调保守，形成**自我纠错闭环**。

**Q5：dirty tag 比传统「线性扫 LRU 找脏页」快在哪？**
A：传统做法要遍历整条 LRU 逐页看 `PG_dirty` 位。xarray tag（`PAGECACHE_TAG_DIRTY`）在页**变脏的那一刻**就打了标记，回收器 `xa_marked()` 一问就知道「有没有脏页」，再按 tag 遍历只碰脏页——**从 O(全链) 降到 O(脏页数)**。

</details>

---
