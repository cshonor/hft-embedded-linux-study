# Ch 10 §6 页面换出守护进程 (`kswapd`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/vmscan.c` 的 `kswapd`、`balance_pgdat`、`try_to_free_pages`）

---

## 本节讲什么

回收有两条路：**后台的 kswapd** 和 **前台的 direct reclaim**。本节回答：

1. kswapd 是一个进程还是每节点一个？它怎么睡、怎么醒？
2. direct reclaim 是什么？为什么它是 HFT 延迟尖刺的元凶？
3. 两者怎么配合，避免「要么不回收、要么卡死」？

---

## 1. kswapd：per-node 后台回收线程（v6.6 `vmscan.c:7713`）

```c
static int kswapd(void *p)
{
    pg_data_t *pgdat = (pg_data_t *)p;
    const struct cpumask *cpumask = cpumask_of_node(pgdat->node_id);

    if (!cpumask_empty(cpumask))
        set_cpus_allowed_ptr(tsk, cpumask);   /* 绑定到本 node 的 CPU */

    tsk->flags |= PF_MEMALLOC | PF_KSWAPD;    /* 防止递归回收 */
    set_freezable();

    for ( ; ; ) {
kswapd_try_sleep:
        kswapd_try_to_sleep(pgdat, alloc_order, reclaim_order, highest_zoneidx);
        /* ... 读新的 order / zoneidx ... */
        reclaim_order = balance_pgdat(pgdat, alloc_order, highest_zoneidx);
        if (reclaim_order < alloc_order)
            goto kswapd_try_sleep;
    }
}
```

| 属性 | 说明 |
|------|------|
| 数量 | **每个 NUMA 节点一个**（`pgdat->kswapd`，Ch 2 §1） |
| 亲和性 | 绑在本 node 的 CPU 上（`set_cpus_allowed_ptr`） |
| 平时 | 睡在 `kswapd_wait` 等待队列（`kswapd_try_to_sleep`，`vmscan.c:7616`） |
| 唤醒 | 某 zone 空闲跌破 `WMARK_LOW`（`wakeup_kswapd()`） |
| 工作 | `balance_pgdat()`（`vmscan.c:7391`）回收直到回 `WMARK_HIGH` |

**`PF_MEMALLOC` 标志：** kswapd 自己分配内存时（比如要写回页需要 buffer），**绕过回收逻辑**、优先拿到内存——否则「回收内存的过程中又需要内存」会死锁或递归回收。

---

## 2. direct reclaim：分配路径上的同步回收

当空闲页跌破 `WMARK_MIN`，kswapd 已经**来不及**了——`__alloc_pages()` 会在**调用方的上下文中同步回收**：

```c
/* vmscan.c:7041 —— direct reclaim 入口 */
unsigned long try_to_free_pages(struct zonelist *zonelist, int order,
                                gfp_t gfp_mask, nodemask_t *nodemask)
{
    struct scan_control sc = {
        .nr_to_reclaim = SWAP_CLUSTER_MAX,
        .priority = DEF_PRIORITY,
        .may_writepage = !laptop_mode,
        .may_unmap = 1,
        .may_swap = 1,
    };
    /* ... */
    nr_reclaimed = do_try_to_free_pages(zonelist, &sc);
    return nr_reclaimed;
}
```

**这就是延迟尖刺的来源：** 一个普通的 `malloc`（`GFP_KERNEL`），如果赶上内存紧张，会在**调用线程里**同步走完「扫描 LRU → 写回脏页 → 可能 swap」的整条回收链。原本微秒级的分配，变成**毫秒到秒级**的停顿——而且调用方毫无感知，`perf` 里看到的就是某个 `__alloc_pages` 突然拖了个长尾巴。

---

## 3. 两条路的配合

```
free pages
  HIGH ──────────────  正常
   LOW ──────────────  wakeup_kswapd() 唤醒后台回收（异步，不阻塞调用方）
   MIN ──────────────  __alloc_pages 同步 direct reclaim（调用方阻塞）
```

- **kswapd 是「预防」**：水位刚跌破 LOW 就提前后台回收，尽量别让水位掉到 MIN。
- **direct reclaim 是「兜底」**：kswapd 没跟上（突发分配、回收太慢），调用方自己动手。

**HFT 的关键判断：** 只要看到 `/proc/vmstat` 里 `allocstall`（direct reclaim 次数）增长，就说明系统已经进入「分配路径同步回收」的危险区——延迟尖刺必然随之而来。

---

## 4. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 延迟尖刺 | `allocstall` 增长 = direct reclaim 发生，`malloc` 里藏了回收 |
| 监控指标 | `/proc/vmstat` 的 `allocstall`、`pgscan_direct`、`pgsteal_direct`、`kswapd_*` |
| 预防手段 | 足够 RAM + 预分配 + `mlock` + 控制内存水位（`vm.min_free_kbytes`） |
| 内核线程 CPU 占用 | `kswapd` 高频运转 = 持续内存压力，看 `top` 里的 kswapd |

---

## 5. 衔接

- 上节 [§5 换出进程页面](./section-5-换出进程页面.md)：kswapd/direct reclaim 驱动的 swap out
- [§7 2.6 内核的新变化](./section-7-2.6-内核的新变化.md)：本章收尾
- 前置：[Ch 2 §2 水位](../../chapter-02-describing-physical-memory/notes/section-2-内存区域.md)（触发条件）
- 下游：[Ch 13 OOM](../../chapter-13-out-of-memory-management/)（回收都救不回来时的最后手段）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：kswapd 是全局一个还是每节点一个？为什么？**
A：**每 NUMA 节点一个**（`pgdat->kswapd`），且绑在本 node 的 CPU 上。原因：回收要按 node 局部做（回收本 node 的页、满足本 node 的分配），全局一个线程会跨 socket 抢锁、也无法针对性平衡各 node 水位。

**Q2：`PF_MEMALLOC` 为什么对 kswapd 至关重要？**
A：kswapd 回收时要写回脏页、可能要分配 buffer 等内存。如果它自己分配内存时又触发回收，就会「回收内存的过程中又需要回收内存」——递归乃至死锁。`PF_MEMALLOC` 让 kswapd 的分配绕过正常回收路径，优先拿内存，打破这个循环。

**Q3：direct reclaim 和 kswapd 都在收，怎么不打架？**
A：它们共享同一套回收代码（`shrink_node`/`shrink_lruvec`），靠 `struct scan_control` 里的 `current_is_kswapd()` 区分身份，并用 `too_many_isolated()` + `reclaim_throttle()` 做**并发节流**——同一时刻被隔离（isolate）的页过多时，后来的回收者会等一等，避免多路回收互相踩踏。

**Q4：怎么判断一次延迟尖刺是 direct reclaim 引起的？**
A：三查：① `/proc/vmstat` 的 `allocstall` / `pgscan_direct` 有没有跳增；② `perf` 火焰图里调用栈有没有 `__alloc_pages → try_to_free_pages → shrink_node`；③ `top` 里 kswapd CPU 是否飙升。三者齐了基本可断定是回收导致的抖动。

**Q5：`vm.min_free_kbytes` 调大能避免 direct reclaim 吗？**
A：能**推迟**，不能根治。它抬高 `WMARK_MIN`，让内核更早、更激进地后台回收，给 direct reclaim 留缓冲。但若应用内存需求持续超过物理内存，水位再高也扛不住，最终还是会进 direct reclaim 甚至 OOM。根治靠**容量规划**（预留足够 RAM）+ 预分配 + mlock。

</details>

---
