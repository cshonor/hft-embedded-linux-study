## ⑤ 请求队列 · Request Queues

| 结构 | 角色 |
|------|------|
| **`request_queue`** | 每块设备一条（书中 **单队列** 模型）— 挂起上层来的 I/O |
| **`struct request`** | 队列中的 **一个 I/O 请求** — 可含 **一个或多个 bio** |

```
文件系统 / 页回写
    ▼
构造 bio ──► 并入 request ──► request_queue
    ▼
I/O 调度器 ──► 驱动 ──► 磁盘
```

> **现代演进：** **blk-mq（多队列）** 替代单队列 + 每 CPU 队列 — 读书抓 **「bio → 队列 → 调度 → 驱动」** 即可。

### 版本断崖：单队列模型已经不存在了

LKD 讲的是这个模型：

```
request_queue ──► request 链表 ──► elevator（电梯）排序 ──► 驱动
                  一把全局锁 q->queue_lock 保护一切
```

v3.13 引入 **blk-mq**、**v5.0 彻底删除旧单队列代码路径**。上面这个模型在现代内核里已经不存在了 —— 如果照着它去读 v6.6 源码，会一个字段都对不上。

### v6.6 现实：两级队列

```
      每个 CPU 一个软件队列 ctx            每个硬件队列一个 hctx
 ┌──────────────────────────────┐    ┌───────────────────────────┐
CPU0 ─► ctx[0]（本CPU独占,无锁）──┐   ┌► hctx[0] ─► HW queue 0
CPU1 ─► ctx[1]                 ─┼──►│    lock / dispatch / state
CPU2 ─► ctx[2]                 ─┤   └► hctx[1] ─► HW queue 1
CPU3 ─► ctx[3]                 ─┘
 └──────────────────────────────┘    └───────────────────────────┘
      tag 用 sbitmap 分配                  每 hctx 一把锁
                                     （锁+dispatch 独占缓存行）
```

第一级 **software queue（ctx，per-CPU）**：本 CPU 独占，**无锁**。第二级 **hardware dispatch queue（hctx）**：每个硬件队列一个，带锁。多个 ctx 可以映到同一个 hctx（CPU 多于硬件队列时）。

### 为什么必须改：三条旧假设全部失效

| 旧假设 | 现实 |
|---|---|
| 设备只有 1 个队列，1 把锁够用 | **NVMe 原生支持多达 65535 个 I/O 队列**，单队列软件层把硬件能力白白浪费 |
| 全局锁竞争可接受 | 几十万 IOPS 下 `queue_lock` 成为瓶颈，8 核以上扩展性崩塌 |
| 请求必须排序（电梯） | SSD 无寻道时间，排序不但无收益还**增加延迟** |

### 源码证据：`request_queue` 里的一等公民字段

| 字段（blkdev.h） | 作用 |
|---|---|
| `const struct blk_mq_ops *mq_ops` | **非 NULL 即 blk-mq 队列**。旧路径已删，现代驱动必须填它 |
| `struct blk_mq_ctx __percpu *queue_ctx` | per-CPU 软件队列，本 CPU 独占，**无锁** |
| `struct xarray hctx_table` | 硬件队列索引表。v6.6 用 **xarray**（呼应 13.4 讲的 radix tree → xarray 迁移） |
| `unsigned int nr_hw_queues` | 硬件队列数量 |
| `struct elevator_queue *elevator` | **可以为 NULL** —— 调度器是**可选**的 |
| `struct blk_mq_tag_set *tag_set` | **预分配** request 的池子 |
| `unsigned long nr_requests` | 队列深度上限 |

### request 是预分配的，不是提交时临时 kmalloc

`blk_mq_tag_set` 在**驱动初始化时**就按队列深度把 `struct request` 全部分配好。提交时用 **sbitmap**（scalable bitmap）分配 tag —— per-CPU hint 的无锁快速路径。

**tag 耗尽会怎样？** 两条路：

| 情况 | 行为 |
|---|---|
| 普通 IO | 请求挂在 hctx 的 `dispatch` 链表上等待，调度器下次 run 时优先派发 |
| 带 `REQ_NOWAIT` | 直接返回 `-EAGAIN`，绝不睡眠（见 14.4 Q3） |

### hctx 的缓存行隔离（blk-mq.h:288）

```c
struct blk_mq_hw_ctx {
	struct {
		spinlock_t	 lock;		/* 保护下面的 dispatch list */
		struct list_head dispatch;	/* 因资源不足未能下发的 request */
		unsigned long	 state;		/* BLK_MQ_S_*：active/scheduled/stopped */
	} ____cacheline_aligned_in_smp;
	...
};
```

`lock` / `dispatch` / `state` 被 `____cacheline_aligned_in_smp` 单独占住缓存行 —— 不同 CPU 操作不同 hctx 时**不会互相弹缓存行**。这是 blk-mq 能随核数线性扩展的关键细节，也是「把热字段隔离到自己的缓存行」这一内核惯用手法（参见 13.6 fdtable 读/写字段分离）的又一次现身。

### plug：提交侧的攒批

`current->plug` 指向 `struct blk_plug`，把短时间内的多个 bio 攒起来，凑够一批再一次性 flush（`blk_mq_flush_plug_list()`）。

| 面 | 说明 |
|---|---|
| 好处 | 一次拿锁派发多个 request，摊薄 hctx 锁开销 |
| **副作用** | **攒批本身引入延迟** —— 请求要先在 plug 里等着 |
| 何时派发 | 攒够（`BLK_MAX_REQUEST_COUNT` 相关阈值）、或显式 `blk_finish_plug()`、或调度出去时 |

**HFT 含义**：单次小量写入的低延迟场景，plug 的攒批窗口是延迟来源之一。`io_uring` 的 `SQPOLL` 内核线程提交路径、以及显式 `blk_finish_plug()` 的时机，都会影响落盘尾延迟。

### 提交路径全景（v6.6，`blk_mq_submit_bio` @ blk-mq.c:2963）

```
blk_mq_submit_bio(bio)
  │
  ├─ blk_mq_plug(bio)                       取当前任务的 plug（攒批容器）
  ├─ __bio_split_to_limits(bio, &q->limits, &nr_segs)
  │                                          ← 按 queue_limits 切分（见 14.2）
  ├─ 选 ctx（本 CPU）+ 选 hctx
  ├─ blk_mq_get_request()                   从 tag_set 用 sbitmap 取 tag + request
  │      └─ 取不到 & REQ_NOWAIT → -EAGAIN
  ├─ 有调度器？→ blk_mq_sched_insert_request()   进 elevator 队列等待排序
  │  无调度器？→ blk_mq_try_issue_directly()    尝试直接塞给驱动
  │                └─ 驱动忙 → 挂 hctx->dispatch
  └─ blk_mq_run_hw_queue()                  调驱动 q->mq_ops->queue_rq()
```

### 调度器是可选的 —— 而且多队列设备默认就是 none

这是本章对 HFT 最实操的一条。源码 `elevator_get_default()`（elevator.c:568）注释原文：

```c
/*
 * For single queue devices, default to using mq-deadline. If we have multiple
 * queues or mq-deadline is not available, default to "none".
 */
static struct elevator_type *elevator_get_default(struct request_queue *q)
{
	if (q->tag_set && q->tag_set->flags & BLK_MQ_F_NO_SCHED_BY_DEFAULT)
		return NULL;

	if (q->nr_hw_queues != 1 && !blk_mq_is_shared_tags(q->tag_set->flags))
		return NULL;

	return elevator_find_get(q, "mq-deadline");
}
```

**NVMe 是多队列设备，所以它默认根本不挂 I/O 调度器。** LKD 花整章讲的「电梯调度是块层核心」，在你手上的 NVMe 盘上默认被整个跳过了。

```bash
cat /sys/block/nvme0n1/queue/scheduler   # [none] mq-deadline kyber bfq
cat /sys/block/nvme0n1/queue/nr_requests # 队列深度
echo none > /sys/block/nvme0n1/queue/scheduler
```

### HFT 调优清单

| 项 | 建议 | 理由 |
|---|---|---|
| `scheduler` | NVMe 上 **none** | 无旋转介质，排序纯开销。需要读优先保尾延迟时再考虑 `mq-deadline` |
| `nr_requests` | 匹配 NVMe SQ 深度（常见 1024） | 太小 → tag 耗尽排队；太大 → 队列深、尾延迟高 |
| `rq_affinity` | 2（完成中断回到提交 CPU） | 减少跨核缓存弹跳 |
| `nomerges` | 按负载试 | 合并省 IOPS 但吃 CPU、加延迟 |
| `nr_hw_queues` | 与在线 CPU 数匹配 | 充分利用多队列硬件 |

→ **14.6** 调度器详解 · **Ch 16** 页缓存如何构造 bio



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 块 IO 请求队列如何合并和排序？对 HFT 有什么影响？

<details><summary>答案</summary>

请求队列用电梯算法（elevator）合并相邻扇区的请求（merge）+ 排序（sort），减少磁盘寻道。SSD 不需要排序（无寻道），用 none/mq-deadline 调度器。HFT 交易日志写 NVMe：如果 IO 延迟不确定（被 merge/sort 延迟），可以用 O_DIRECT + io_uring 绕过 page cache 和 IO 调度。

**补充**：在 v6.6 上，"绕过 IO 调度" 对多队列设备（NVMe）其实是默认状态——`elevator_get_default()`（elevator.c:568）在 `nr_hw_queues != 1` 时直接返回 NULL。真正要绕过的是 page cache（O_DIRECT）和系统调用提交开销（io_uring）。

</details>

**Q2.** blk-mq 的两级队列（ctx / hctx）各自解决什么问题？为什么第一级是无锁的？

<details><summary>答案</summary>

**第一级 software queue（ctx，per-CPU）**解决的是**全局锁竞争**。旧单队列模型用一把 `q->queue_lock` 保护整个请求队列，几十万 IOPS 下 8 核以上扩展性直接崩塌。blk-mq 把队列按 CPU 拆分：每个 CPU 有自己的 `struct blk_mq_ctx`（`request_queue.queue_ctx`，`__percpu`），本 CPU 独占，**没有任何锁**——提交路径上完全无竞争。

**第二级 hardware dispatch queue（hctx）**解决的是**与硬件队列对齐**。NVMe 原生支持多达 65535 个 I/O 队列，旧的单队列软件层把硬件能力白白浪费。每个 hctx 对应一个硬件队列，`struct blk_mq_hw_ctx` 的 `lock`/`dispatch`/`state` 用 `____cacheline_aligned_in_smp` 独占缓存行，不同 CPU 操作不同 hctx 时不会互相弹缓存行。

v6.6 里 hctx 通过 `request_queue.hctx_table`（**xarray**）索引。多 CPU 对少硬件队列时是 N:1 映射，此时才会在 hctx 锁上竞争。

</details>

**Q3.** `struct request` 是提交 IO 时动态分配的吗？tag 耗尽会发生什么？

<details><summary>答案</summary>

**不是，是预分配的。** `struct blk_mq_tag_set` 在驱动初始化阶段就按队列深度把全部 `struct request` 分配好了。提交时只是用 **sbitmap**（scalable bitmap，per-CPU hint 的无锁快速路径）从池子里取一个 tag，把 request 和 tag 绑定——热路径上没有内存分配。

tag 耗尽有两条路：

| 情况 | 行为 |
|---|---|
| 普通 IO | request 挂到 hctx 的 `dispatch` 链表等待，硬件队列下次 run 时**优先派发**（为了公平） |
| 带 `REQ_NOWAIT` | 直接返回 `-EAGAIN`，绝不睡眠 |

所以 `nr_requests`（队列深度）是个权衡：太小 → tag 频繁耗尽、请求排队；太大 → 队列深、尾延迟高。HFT 场景通常把它调到与 NVMe SQ 深度匹配（常见 1024），并配合 `REQ_NOWAIT` + io_uring 让"拿不到资源"变成显式可处理的状态而不是隐式睡眠。

</details>

</details>
---
