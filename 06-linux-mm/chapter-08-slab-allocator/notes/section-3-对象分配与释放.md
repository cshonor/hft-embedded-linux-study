# Ch 8 §3 对象分配与释放（SLUB 快慢路径）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/slub.c` :3095 `___slab_alloc`、:3500 `kmem_cache_alloc`、:3825 `kmem_cache_free`）

---

## 本节讲什么

原书的 `kmem_cache_alloc/free` 在 SLUB 里是 **一条无锁快路径 + 三层慢路径** 的瀑布。这条瀑布是内核里被走过最多次的代码路径之一（每个 skb、每个 dentry 都过这里）——它的形状就是"高性能分配器"的参考答案。

---

## 1. 分配瀑布（v6.6）

```
kmem_cache_alloc(s, gfp)                    slub.c:3500
  └─ slab_alloc_node()                      快路径
       │  object = c->freelist;             ← 读本 CPU 空闲链头
       │  this_cpu_cmpxchg_double(c->freelist, c->tid,
       │          object->next, next_tid)   ← 双字段原子交换（:421 注释）
       │  命中 → return object              【零锁、单条原子指令】
       ▼ 未命中（freelist 空 / 被抢）
  ___slab_alloc(s)                          慢路径 :3095
       ├─ ① c->partial 有备胎？→ 换 active slab（仍本 CPU，无全局锁）
       ├─ ② 本 CPU partial 也空 → 从 node->partial 搬一个（拿 list_lock）
       │      搬空的 node partial → 调用 slab 的 deactivate
       └─ ③ 全空 → new_slab() → alloc_pages()（buddy，可能 reclaim！）
                → 划对象 → freelist randomize → freeze
```

| 层级 | 拿什么锁 | 频率（稳态） |
|------|----------|--------------|
| 快路径 cmpxchg | 无 | 99%+ |
| 本 CPU partial 换 slab | local lock（关抢占） | slab 用尽时 |
| node partial 搬运 | `node->list_lock`（spinlock） | 罕见 |
| new_slab → buddy | zone lock + 可能 direct reclaim | 池冷启动/扩张时 |

## 2. 快路径细节：`tid` 事务号

```c
/* 概念还原（slub.c:2757 附近 next_tid 逻辑） */
c->tid = next_tid(c->tid);   /* 每次慢路径操作递增 */
```

`cmpxchg_double` 同时校验 `(freelist, tid)` 两个字段：中途被中断/抢占/迁移到别的 CPU 改了任一字段，CAS 失败重走——**这是 ABA 防护**，等价于 lock-free 编程里的 tagged pointer。写用户态 lock-free 池直接抄这个模式。

## 3. 释放路径

```
kmem_cache_free(s, x)                       slub.c:3825
  └─ do_slab_free(s, slab, head, tail, cnt)
       ├─ slab == c->slab（本 CPU active）？
       │     → set_freepointer + cmpxchg 挂回 c->freelist   【快：单 CAS】
       ├─ slab frozen（别的 CPU 的 active/partial）？
       │     → CAS 挂回 slab->freelist（frozen 语义：只放不取）
       └─ 都不是（冷 slab，还有其他对象在用）
             → __slab_free：拿 slab 锁判断
                  全空？→ 归还 buddy（或挂 per-cpu partial 备胎）
                  半空？→ 挂 node->partial（list_lock）
```

**free 快路径同样是单 CAS。** 分配/释放双快路径无锁，是 SLUB 相对 SLAB 的核心性能差来源。

## 4. 原书对照

| 原书（SLAB） | SLUB |
|---------------|------|
| `kmem_cache_grow()` 建新 slab | `new_slab()`（同构：buddy 取页 → 划对象） |
| bufctl 索引栈 O(1) 取还 | 嵌入式 freelist O(1)（更省） |
| 三链表找 partial | per-CPU partial → node partial 两级 |
| ctor 初始化新对象 | 无 ctor；`slab_post_alloc_hook` 只做 kasan/memcg 记账 |
| per-CPU array_cache batch refill | active slab 整块冻结（更粗粒度=更少慢路径次数） |

## 5. 慢路径的代价账（HFT 视角）

| 事件 | 量级 | 触发场景 |
|------|------|----------|
| 本 CPU partial 换 slab | ~100ns（本地） | 每 64~N 个对象一次 |
| node partial 搬运 | 数百 ns + 自旋锁 | 池抖动 |
| **new_slap → buddy 直排** | **µs~ms（reclaim/compaction 时）** | 内存压力下 kmalloc |
| memcg/kasan/kfence hook | 每次分配 +几十~几百 ns | 开销与 debug 选项成正比 |

**教训同构于用户态：** 池要 **预热**（启动时把 partial/node 层填满），运行期别让分配掉到"向 OS 要内存"那层——mempool/arena 满额、hugepage 预留，都是这个道理。

## 6. 观测与调优

```bash
# 每个 cache 的分配/换主统计
grep -w 'cmpxchg_double_fail\|cmpxchg_double_cpu_fail' /proc/vmstat   # CAS 竞争程度
# sysfs 调优旋钮（专用 cache 才值得动）
echo 64 > /sys/kernel/slab/<name>/cpu_partial   # 每 CPU 备胎对象数
cat /sys/kernel/slab/<name>/alloc_calls
```

## 7. HFT / 嵌入式关联

| 机制 | 用户态镜像 |
|------|-----------|
| cmpxchg_double 快路径 | 每核 freeptr + 版本号，单 CAS pop |
| 三层瀑布 | 每核热池 → 每 NUMA 冷池 → mmap 预留区；**每层容量递增、速度递减** |
| frozen 只放不取 | MPSC ring / 无锁栈的 lazy free 方向 |
| new_slab 直排惩罚 | 池水位线告警 + 预扩容（启动期填充所有层） |

## 8. 衔接

- [§2 核心数据结构](./section-2-核心数据结构：Cache-与-Slab.md)：瀑布跑在其上的结构
- [§5 每 CPU 缓存](./section-5-每-CPU-对象缓存.md)：① 层的完整语义
- [Ch 6 物理页分配](../../chapter-06-physical-page-allocation/)：③ 层的下游

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么快路径用 cmpxchg_double 而不是普通 cmpxchg？**
A：需要同时原子更新两个字段：`freelist`（指针）和 `tid`（事务号）。x86_64/ARM64 有 16 字节双字 CAS 指令（CMPXCHG16B / LDXP-STXP），正好放下 (ptr, counter)。只用单字段 CAS 会留 ABA 窗口：链头 A→B→free→A，CAS 以为没变。**用户态复刻时没有 cmpxchg_double 就用 128 位原子或版本号打包。**

**Q2：分配快路径里完全没锁，那禁不禁中断？**
A：不禁。cmpxchg 失败就重试（循环里再读 c->freelist）。能这样是因为 per-CPU 字段只被本 CPU 的任务/中断上下文触达，竞争窗口只有"被中断后中断处理里又分配同 cache"这种自嵌套——CAS 重试天然解决。比关中断便宜。

**Q3：`kmalloc(GFP_ATOMIC)` 和 `GFP_KERNEL` 在 SLUB 层有区别吗？**
A：SLUB 层几乎无区别（都走同一瀑布）；区别在慢路径 ③：GFP_ATOMIC 禁止睡眠/直排回收（alloc_pages 带 `__GFP_NORETRY` 语义倾向），拿不到就直接 NULL。中断上下文必须 GFP_ATOMIC 的原因全在 buddy 层。

**Q4：free 一个属于"已从 node partial 摘下的冷 slab"的对象，走哪条路？**
A：`__slab_free` 冷路径：拿 slab bit 锁，挂回对象后判断——若因此全空且 partial 备胎超限，整 slab 还 buddy；否则挂 node->partial。这条路径拿 `list_lock`，是多核 free-heavy 负载的锁热点（perf 可见）。

**Q5：用户态池怎么抄"预热"最省事？**
A：启动时循环 alloc N 个对象再全 free——free 会走快路径挂回本 CPU freelist，等效把 active slab + per-CPU partial 填满（注意：对象要真正落在本核 free 才进本 CPU 链，跨核 free 会进 frozen 慢路——所以预热线程要绑核各做一遍）。

</details>

---
