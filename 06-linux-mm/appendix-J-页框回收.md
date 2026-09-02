# 附录 J 页框回收 · Page Frame Reclamation

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6（`mm/vmscan.c`，8148 行）

概念总览 → [./chapter-10-page-frame-reclamation/](./chapter-10-page-frame-reclamation/)（现代 **`mm/vmscan.c`**）

---

## 本节走读什么

原书附录 J 走读 **LRU 链表 + kswapd + balance_pgdat**。但 v6.6 发生两次换代：**① 回收单位从 `struct page` 全面迁移到 `struct folio`**；**② 新增 MGLRU（多代 LRU）路径，与经典 LRU 链表并存**（`lru_gen_enabled()` 时走新路径）。本附录走读 v6.6 的两条回收主线：**后台 kswapd** 与 **前台 direct reclaim**，并拆解经典 `shrink_lruvec` 与 MGLRU 的分工。

---

## 1. 回收的两条腿

```
内存不足
   ├─ 后台腿：kswapd 内核线程（每 NUMA 节点一个）
   │      kswapd()                          // vmscan.c:7713
   │        └─ balance_pgdat()              // :7391  水位检测 + 触发回收
   │             └─ kswapd_try_to_sleep()   // :7616  水位恢复后休眠
   │
   └─ 前台腿：direct reclaim（分配页失败时调用方自己回收）
          try_to_free_pages()               // :7041
            └─ do_try_to_free_pages()       // :6823
                 └─ shrink_node()           // :6519
                      └─ shrink_node_memcgs()  // :6461（遍历 memcg）
                           └─ shrink_lruvec()  // :6273  ← 核心
```

**走读要点**：kswapd 是**异步预防**（水位低于阈值时提前回收，不阻塞分配者），direct reclaim 是**同步兜底**（分配者自己下场回收，会阻塞并拉长分配延迟）。这正是 HFT 最忌讳的「分配路径上的隐式回收」。

---

## 2. 核心结构 `struct scan_control` 与 LRU 组织

**`struct scan_control`**（include/linux/swap.h）是回收任务的「控制块」：

| 字段 | 含义 |
|------|------|
| `nr_to_reclaim` | 本轮要回收多少页 |
| `priority` | 优先级（`DEF_PRIORITY`=12 起，逐轮下降；越低扫得越狠） |
| `reclaim_idx` | 从哪个 zone 开始回收 |
| `may_swap` / `may_writepage` | 是否允许换出 / 写回脏页 |
| `swappiness` | anon 与 file 的回收倾向（0~200，默认 60） |

**LRU 五链表**（`enum lru_list`）：`LRU_INACTIVE_ANON` / `LRU_ACTIVE_ANON` / `LRU_INACTIVE_FILE` / `LRU_ACTIVE_FILE` / `LRU_UNEVICTABLE`。anon（匿名页，换出到 swap）与 file（文件页，脏则写回、干净直接丢弃）**分开**管理，因为回收代价不同。

---

## 3. 经典路径：`shrink_lruvec`（:6273）

```
shrink_lruvec(lruvec, sc)                    // :6273
   ├─ lru_gen_enabled() 且非 root reclaim？
   │     └─ lru_gen_shrink_lruvec()          // :5497  ← 走 MGLRU，直接返回
   ├─ get_scan_count(lruvec, sc, nr)         // :3025 按 swappiness 算 anon/file 各扫多少
   ├─ while (还有待扫的链表)
   │     └─ shrink_list(lru, nr_to_scan, lruvec, sc)   // :2844
   │           ├─ 是 active  → shrink_active_list()    // :2688 降级到 inactive
   │           └─ 是 inactive → shrink_inactive_list()  // :2568 真正回收
   │                 └─ isolate_lru_folios()           // :2304 从 LRU 摘出 folio
   │                       └─ shrink_folio_list()      // :1705 逐个写回/换出/释放
   └─ 若 anon inactive 过低 → shrink_active_list() 再平衡
```

**`get_scan_count`（:3025）** 是回收的「调度器」：根据 `swappiness`、anon/file 比例、是否有 swap 空间，算出每个链表要扫多少页。**`shrink_folio_list`（:1705）** 是真正的终结者——对每个摘出的 folio 判断：干净 file → 直接释放；脏 file → 写回；anon → 换出到 swap；被锁/正在写回 → 放回 LRU（`activate`）。

---

## 4. MGLRU 路径：多代 LRU（`lru_gen_shrink_lruvec`）

v6.6 引入 **MGLRU（Multi-Gen LRU）**，用「代（generation）+ 层（tier）」替代「active/inactive 二分类 + 两次访问激活」：

```
lru_gen_shrink_lruvec()                      // :5497
   └─ evict_folios()                         // :5165
        └─ scan_folios()                     // :4999
             ├─ gen = min_seq[type]          // 从最老一代开始扫
             ├─ sort_folio()                 // :4896 按 tier 分类
             └─ isolate_folio()              // :4965 摘出待回收 folio
```

**核心数据 `struct lru_gen_folio`**（`lruvec->lrugen`）：每个 type（anon/file）× 每个 zone 的**多代环形链表** `folios[gen][type][zone]`，`min_seq`/`max_seq` 标记代际范围。`get_type_to_scan`（:5095）用 **refault 控制误差**（`positive_ctrl_err`）动态决定先扫 anon 还是 file、扫到哪一代——比经典路径的静态 swappiness 更自适应。

**走读要点**：MGLRU 的核心洞察是 **refault（缺页重新读回）是「回收错了」的信号**——刚被回收又被访问的页不该那么早回收。它用 PID 控制器式反馈（tier 划分）把「容易 refault 的页」保留得更久。

---

## 5. 两处关键细节

**① `shrink_slab`（:1033）**：回收除了换页，还压缩 slab 缓存（dcache/icache），通过附录 H 的 `shrinker` 回调 `count_objects`/`scan_objects`。

**② `reclaim_pages`（:2811）**：批量回收一个 folio 列表的公开 API，被 madvise(MADV_PAGEOUT) 等调用——用户态可主动触发回收。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| kswapd / balance_pgdat | Ch10 §2（后台回收） |
| try_to_free_pages / direct reclaim | Ch10 §3（直接回收） |
| shrink_lruvec / get_scan_count | Ch10 §4（扫描调度） |
| shrink_folio_list | Ch10 §5（页终结） |
| MGLRU | Ch10 §6（版本断崖） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| 避免 direct reclaim | 分配路径上的 `try_to_free_pages` 会阻塞并引入不可预期延迟——预热内存、`mlock` 关键页、预留大页 |
| kswapd 抖动 | 观察 `/proc/vmstat` 的 `pgscan_*`/`pgsteal_*`，kswapd 频繁唤醒说明水位设置或 workload 有问题 |
| MGLRU | 新内核（≥6.1）默认行为改变，升级内核后需重新验证「哪些页会被先回收」对延迟尾部的冲击 |
| `vm.swappiness` | 对 HFT（大量匿名内存）调低 swappiness 减少无谓换出 |

---

## 相关章节

- 上一章：[appendix-I-高端内存管理.md](./appendix-I-高端内存管理.md)
- 下一章：[appendix-K-交换管理.md](./appendix-K-交换管理.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：kswapd 和 direct reclaim 的分工？**

kswapd 是后台内核线程，水位低于阈值时**异步预防性回收**（balance_pgdat :7391），不阻塞分配者；direct reclaim 是分配失败时**调用者同步自己回收**（try_to_free_pages :7041），会阻塞分配路径。

**Q2：anon 和 file 页回收的区别？**

anon（匿名页）只能换出到 swap（有 I/O 成本）；file（文件页）脏则写回、干净直接丢弃。所以两者分链表管理，回收代价不同。

**Q3：`get_scan_count`（:3025）干什么？**

回收的「调度器」：根据 `swappiness`、anon/file 比例、是否有 swap 空间，算出每个 LRU 链表本轮要扫多少页。

**Q4：`shrink_folio_list`（:1705）如何处置摘出的 folio？**

干净 file → 直接释放；脏 file → 写回；anon → 换出到 swap；被锁或正在写回 → 放回 LRU（activate，暂不回收）。

**Q5：MGLRU 相比经典 active/inactive LRU 的核心改进？**

用「代 + tier + refault 反馈」替代「二分类 + 两次访问激活」。refault 被当作「回收错了」的信号，通过 PID 式控制动态决定先扫 anon/file 及扫到哪一代，比静态 swappiness 更自适应。

</details>
