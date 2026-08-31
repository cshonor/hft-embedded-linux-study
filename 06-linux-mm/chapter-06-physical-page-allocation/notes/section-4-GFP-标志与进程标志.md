# Ch 6 §4 GFP 标志与进程标志（v6.6 语义全表）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/gfp_types.h`、`mm/page_alloc.c` :2835/:2920 水位）

---

## 本节讲什么

`gfp_mask` 是分配请求的 **合同**：声明"我什么上下文、要多干净的页、允许付出什么代价"。读错合同是内核态代码的经典事故源。本节给 v6.6 的完整语义表 + 水位模型 + 两个进程标志。

---

## 1. GFP 三段式结构（v6.6 `gfp_types.h`）

一个 gfp_mask 由三组位域组成：

```
[ zone 修饰 ]  [ 水位/移动性 ]  [ 行为标志 ]
   __GFP_DMA       __GFP_HIGH      __GFP_DIRECT_RECLAIM
   __GFP_DMA32     __GFP_MEMALLOC  __GFP_KSWAPD_RECLAIM
   __GFP_MOVABLE   __GFP_NOMEMALLOC __GFP_IO / __GFP_FS
                   __GFP_RECLAIMABLE __GFP_NORETRY / __GFP_RETRY_MAYFAIL
                                     __GFP_NOWARN / __GFP_NOFAIL ...
```

## 2. 常用组合速查（背下这张就够日常读码）

| 组合 | 展开 | 允许什么 | 禁止什么 | 典型用户 |
|------|------|----------|----------|----------|
| `GFP_KERNEL` | 内核普通 | 睡眠、reclaim、IO | — | 进程上下文分配 |
| `GFP_ATOMIC` | `(__GFP_HIGH)` | 不睡眠 | reclaim/IO/compaction 全禁 | 中断/持锁 |
| `GFP_NOWAIT` | — | 不睡眠 | 比 ATOMIC 更狠（不碰 pcp 之外的储备） | io_uring 快路径 |
| `GFP_NOIO` | | 回收但不发 IO | 块栈重入防护 | 存储/FS 内部 |
| `GFP_NOFS` | | 回收不走 FS 操作 | FS 重入防护 | 文件系统内部 |
| `GFP_USER` | | 用户可映射、可回收 | — | 用户页缺页 |
| `GFP_HIGHUSER_MOVABLE` | | + 可迁移 | — | 匿名页/THP 的正主 |
| `GFP_DMA` | | 低 16MiB | — | 老式 ISA 设备 |
| `GFP_KERNEL|__GFP_NORETRY` | | 一次回收尝试即认输 | 无限重试 | 高阶"尽力而为" |
| `GFP_KERNEL|__GFP_RETRY_MAYFAIL` | | 多轮努力但**不触发 OOM** | OOM kill | 宁可失败不要杀进程 |
| `GFP_KERNEL|__GFP_NOFAIL` | | **永不返回 NULL** | 失败 | 死等（慎用！） |

**三个 retry 系标志的微差是 v6.6 读码高频考点：**

| 标志 | 失败可能 | OOM 可能 | 用途 |
|------|----------|----------|------|
| 默认（GFP_KERNEL） | 低 | 有 | 常规 |
| `__GFP_NORETRY` | 高 | 无 | 降级友好（THP/kvmalloc 上游） |
| `__GFP_RETRY_MAYFAIL` | 中 | **无** | "尽力但别杀人" |
| `__GFP_NOFAIL` | 零 | — | 调用方无法处理失败（历史遗留雷区） |

## 3. 水位模型（每 zone 三条线）

```
      ───── high ─────  kswapd 在此线以下被唤醒（异步回收开始）
         free pages
      ───── low  ─────  kswapd 加速 + 直接回收阈值逼近
      ───── min  ─────  以下只许 PF_MEMALLOC 通行；普通分配进慢路径/OOM
```

| 事实 | 说明 |
|------|------|
| 计算 | `min` 由 `min_free_kbytes`（可调）按 zone 比例分摊；low/high 按 min 的 1.25×/1.5× 推 |
| 判定入口 | `zone_watermark_fast()`（page_alloc.c:2920）——order 越高判定越苛刻（为高阶留 fragmentation reserve） |
| HFT 调优 | `min_free_kbytes` 调大 = 给突发/高阶留缓冲垫，减少 direct reclaim 概率；代价是"可用"内存变少 |

**`/proc/zoneinfo` 的 pages min/low/high 三行**就是这三条线的实时值。

## 4. 两个进程标志（原书重点，仍在）

| 标志 | 谁设 | 通行权 |
|------|------|--------|
| `PF_MEMALLOC` | kswapd、direct reclaim 路径、`memalloc_use_reserve()` 区段 | **无视 min 水位**——回收者必须能拿到页才能完成回收（否则自锁死） |
| `PF_MEMDIE`（v6.6 已并入 `PF_MEMALLOC` 语义下的 `TIF_MEMDIE`） | 被 OOM 选中的进程 | 临终赠礼：让它自己的退出路径能分配内存 |

**用户态对应物：** systemd 的 `MemoryMin`/`MemoryLow`（cgroup 水位）——"保护关键路径在压力下的分配权"。HFT 部署清单里引擎 cgroup 应设 memory.min，与内核这套思想同源。

## 5. `__GFP_ZERO` 与编译器不可见性

`alloc_pages(GFP_KERNEL | __GFP_ZERO, order)` = 页级 calloc。值得知道：kmalloc 场景 **优先 `kzalloc` 而非 kmalloc+memset**——`__GFP_ZERO` 可走 buddy 的零页优化路径（合并到分配循环里做，cache 行为更好）。

## 6. HFT / 嵌入式关联

| 场景 | 正确姿势 |
|------|----------|
| 内核态驱动热路径 | `GFP_ATOMIC` + 处理 NULL（或预分配池，见 Ch 8） |
| 大缓冲 | `GFP_KERNEL\|__GFP_NORETRY\|__GFP_NOWARN` + 失败降级 vmalloc（kvmalloc 已封装） |
| 避免 OOM 连坐 | `__GFP_RETRY_MAYFAIL`——引擎的内核组件被 OOM kill 是事故 |
| 调水位 | `min_free_kbytes` 适度上调 + `/proc/zoneinfo` 监控水位穿越频率 |
| 用户态边界 | mlock 之外，引擎还可 `madvise(MADV_RANDOM)` 减少 page cache 预读的 GFP 压力 |

## 7. 衔接

- [§2 页面分配](./section-2-页面分配.md)：这些标志在慢路径瀑布的兑现
- [Ch 13 OOM](../../chapter-13-out-of-memory-management/)：`TIF_MEMDIE` 的完整故事
- [12.5/ch12 io_uring](../../../12.5-modern-networking/chapter-12-io-uring-net/)：`GFP_NOWAIT` 的高频用户

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：中断上下文里能用 `GFP_KERNEL` 吗？就一小块。**
A：不能，多大都不行。GFP_KERNEL 允许睡眠——中断里睡眠 = 调度崩溃。持自旋锁时同理（睡眠会死锁别的核）。规则不按大小打折：**上下文决定 GFP，需求只决定 size**。

**Q2：`__GFP_NOFAIL` 为什么是雷区？**
A：它把"分配失败"这个错误路径整个删除——失败时内核无限重试/等待。调用方看似省事，实际在内存耗尽时造成活锁，且已被标记为待淘汰接口（新代码禁用，存量逐步清理）。教训：**不可失败的接口 = 不可测试的接口**。

**Q3：`GFP_NOIO` 和 `GFP_NOFS` 具体防什么死锁？举一例。**
A：NOFS 防文件系统重入：FS 写路径分配页 → 触发回收 → 回收为释放内存要写脏页 → 又进同一个 FS 的写路径 → 递归持锁死锁。NOIO 同理作用于块层。**回收是"会调用任何人的"公共路径，分配者必须声明自己怕谁。**

**Q4：水位三线里为什么需要 low？high 唤醒 kswapd 不够吗？**
A：kswapd 是异步的、可能跟不上消耗速度。low 线的意义是"分配者自己在 low 以下时也加入回收"（direct reclaim 的触发参考）——两级响应：异步（kswapd）先顶，顶不住同步（direct）跟上。类似熔断器的双阈值设计。

**Q5：`min_free_kbytes` 调大能消灭 direct reclaim 吗？**
A：只能降低频率，不能消灭——突发流量或大额泄漏照样穿底。它是"缓冲垫"不是"保证"。真正的消灭手段还是：预留（hugepage/mempool）+ 容量规划 + cgroup 限额把邻居的雷隔开。

</details>

---
