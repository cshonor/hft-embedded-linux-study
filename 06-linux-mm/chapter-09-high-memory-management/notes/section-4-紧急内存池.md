# Ch 9 §4 紧急内存池 (Emergency Pools)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（bounce 专用紧急池已随 `mm/bounce.c` 删除；`mempool` 泛化存活）

---

## 本节讲什么

本节回答一个问题：**「内存紧张 → 需要内存才能推进 → 但又拿不到内存」这个死锁，内核怎么破？**

答案是**预留 (reserve)**——关键路径提前圈一块内存，再紧张也能完成少量关键 I/O。原书的「bounce 紧急池」已随 bounce.c 删除，但「**关键路径预 reserve**」这个思想被 `mempool` 泛化并存活至今。

---

## 1. 死锁场景：为什么需要紧急池

```
HIGHMEM I/O 需要 LOWMEM 做 bounce
        │
        ▼
LOWMEM 耗尽（普通分配拿不到）
        │
        ▼
I/O 挂起（bounce 分配失败，DMA 无法完成）
        │
        ▼
进程阻塞在 I/O，无法释放内存
        │
        ▼
内存更紧张 → 彻底僵局（deadlock）
```

**核心矛盾**：完成 I/O 需要内存（bounce），但**只有完成 I/O 才能释放内存**。这是一个「先有鸡还是先有蛋」的循环。破解方法只有一个：**提前预留**。

---

## 2. 原书方案：bounce 紧急池（已删除）

原书为 bounce 保留两个紧急池：

| 池 | 保留对象 |
|----|----------|
| **页面池** | 至少若干**可立即用于 bounce 的页**（`init_emergency_pool`） |
| **`buffer_head` 池** | 块 I/O 路径关键结构 |

**保证**：内存再紧也能完成少量关键 HIGHMEM I/O，避免全局僵局。

**v6.6 真相**：这两个池**都随 `mm/bounce.c` 一起删除了**。因为块层 bounce 消失了（§3），「bounce 专用紧急池」这个具体机制也一并退役。

---

## 3. 思想泛化：`mempool`（存活至今）

原书 §4 的「紧急池」思想在 2.6 被泛化为 **`mempool_t`**（Ch8），并**存活到 v6.6**——这是本节真正值得带走的机制：

```c
/* 任何子系统都能预 reserve 关键对象 */
mempool_create_kmalloc_pool(min_nr, size);   /* 预分配 min_nr 个对象 */
mempool_alloc(pool, GFP_KERNEL);             /* 先走普通分配，失败则吃 reserve */
mempool_free(obj, pool);                     /* 归还（优先回填 reserve） */
```

| 概念 | 说明 |
|------|------|
| `min_nr` | 池里**至少保留**的对象数——平时不动，只在普通分配失败时吐出 |
| 双级分配 | 先走常规 allocator；失败才动 reserve |
| 回填 | free 时先补满 reserve，多余才还给常规 allocator |

**典型用户**：块层 `bio`、`bio_vec`、SCSI 命令、网络 skb 关键路径——**凡「失败会导致无法推进」的路径**都该配 mempool。

---

## 4. v6.6 的 bounce 保底：swiotlb 的预分配

bounce 的「紧急池」虽没了，但 swiotlb 用**另一种方式**实现了同样的保底：

| 原书紧急池 | swiotlb 的对应 |
|-----------|---------------|
| 启动时预分配 bounce 页 | `io_tlb_default_mem` 在**启动时预分配**（默认 64MB） |
| 内存紧张时保证 bounce 可用 | 池是**专用的、不参与普通分配**，天然隔离 |
| 池满 → 全局僵局 | 池满 → `swiotlb buffer is full` 告警（而非静默死锁） |

**关键差异**：swiotlb 的池是**「专用 + 启动预分配」**，而不是「运行时从普通内存池里 reserve」。这比 mempool 更彻底——**连「从普通分配器借」都不需要**，直接划一块谁都碰不到的低地址内存。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 关键路径（订单发送）不能因内存失败 | 用 mempool 预 reserve 关键对象，保证「最坏情况下也能推进」 |
| 排查 swiotlb 溢出 | `dmesg` 看 `swiotlb buffer is full`，对应「bounce 保底耗尽」 |
| 「预留 vs 灵活」的权衡 | mempool 的 `min_nr` 是「牺牲一部分平时可用内存，换最坏情况的确定性」 |

**核心思想**（HFT 尤其受用）：**延迟敏感系统的关键路径要「预留」，而非「临时借」**——因为「临时借」在内存紧张时会失败，失败就会让关键路径卡死。mempool 和 swiotlb 都是这一思想的内核级实现。

---

## 6. 衔接

- 上一节：[§3 回弹缓冲区](./section-3-回弹缓冲区.md)（swiotlb 是 bounce 的现代保底）
- mempool 详解：[Ch8 Slab 分配器 §6 mempool](../../chapter-08-slab-allocator/notes/section-6-2.6-内核的新变化.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么「紧急池」能破死锁，而「再多分配器」不能？**
A：死锁的根子是「**需要内存才能完成 I/O，但完成 I/O 才能释放内存**」。再多分配器，内存紧张时照样拿不到。紧急池的破法**不靠多分，靠预留**——提前圈一块「谁都抢不走」的内存，关键时刻拿出来完成 I/O，让系统**重新转动起来**。

**Q2：原书的 bounce 紧急池为什么在 v6.6 删除了？**
A：因为它服务的对象——**块层 bounce 缓冲**——本身被删了（§3）。`mm/bounce.c` 连同 `init_emergency_pool`、`buffer_head` 紧急池一起退役。这不是「不需要保底」了，而是「保底的对象换了」：swiotlb 用启动预分配实现同样的保底。

**Q3：`mempool` 和 swiotlb 的「预留」方式有什么本质区别？**
A：mempool 是**运行时 reserve**——从普通内存分配器里预留 `min_nr` 个对象，平时放着，紧张时吐出；swiotlb 是**启动时专用预分配**——划一块**独立、不参与普通分配**的低地址物理内存。后者隔离得更彻底，但**牺牲了灵活性**（64MB 只能给 swiotlb 用）。

**Q4：`mempool_alloc` 的「双级分配」具体怎么走？**
A：先走**普通分配**（如 `kmalloc`），成功就直接返回，**不动 reserve**；失败时再从池的 reserve 里拿一个。这样 reserve 在**平时完全不消耗**（不会被常态流量吃掉），只在**紧急时刻**才被动用——「平时零成本，急时保底」。

**Q5：HFT 里怎么用「预留」思想保护关键路径？**
A：订单发送、行情处理这类**绝不能因内存失败而卡住**的路径，应仿照 mempool——预分配好**固定数量的关键对象**（如订单结构、缓冲），最坏情况下也能推进。这比「临时 malloc 再判 NULL」可靠得多，因为**判 NULL 意味着你已经失败了**，而预留意味着「最坏情况也能走完」。

</details>

---
