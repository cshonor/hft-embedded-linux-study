# 01 — XDP_REDIRECT 的四个目的地：批量语义、devmap 与 cpumap

> **对应 Rosen:** 无（XDP 4.8+、DEVMAP 4.14+、CPUMAP 4.15+，3.x 时代均不存在）
> **内核版本：** 以 **v6.6** 为准，行号取自：
> - `net/core/filter.c` — `xdp_do_redirect`、`__xdp_do_redirect_frame` 4274、`xdp_do_flush` 4202
> - `kernel/bpf/devmap.c` — `bq_enqueue` 441、`bq_xmit_all` 361、`dev_map_enqueue` 525
> - `kernel/bpf/cpumap.c` — `bq_enqueue` 697、`cpu_map_kthread_run` 262
> - `include/net/xdp.h` — `XDP_BULK_QUEUE_SIZE` 190、`struct xdp_cpumap_stats` 212
>
> ⚠️ **路径提示**：v6.6 起 devmap 已从 `net/core/devmap.c` 移到 **`kernel/bpf/devmap.c`**，
> cpumap 在 **`kernel/bpf/cpumap.c`**。网上大量资料仍写旧路径。

---

## 文档概述

[chapter-05](../../chapter-05-xdp-architecture/) 讲了 XDP 的五个动作和它的位置，
[chapter-06](../../chapter-06-af-xdp/) 讲了其中一个目的地（AF_XDP socket）。
本篇补齐剩下三个目的地，并回答一个几乎所有资料都含糊带过的问题：

> **`XDP_REDIRECT` 返回的那一刻，包去哪了？**

答案是：**哪儿都没去，它还在 per-CPU 的批量队列里躺着。**
真正把它送走的是 `xdp_do_flush()`，而那要等到 NAPI poll 结束。

| 笔记 | 侧重 |
|------|------|
| **01（本篇）** | redirect 的三步语义、四个目的地（devmap/cpumap/xskmap/ifindex）、批量队列、观测 |
| [02 XDP vs DPDK](02-xdp-vs-dpdk.md) | 旁路路线选型："谁拥有网卡"而不是"谁更快" |

---

## 一、`XDP_REDIRECT` 不是立即动作：三步语义

内核源码里有一段把这件事讲得最清楚的注释（`net/core/filter.c:4170`）：

```
 * 1. The bpf_redirect() and bpf_redirect_map() helpers will lookup the target
 *    of the redirect and store it (along with some other metadata) in a per-CPU
 *    struct bpf_redirect_info.
 *
 * 2. When the program returns the XDP_REDIRECT return code, the driver will
 *    call xdp_do_redirect() which will use the information in struct
 *    bpf_redirect_info to actually enqueue the frame into a map type-specific
 *    bulk queue structure.
 *
 * 3. Before exiting its NAPI poll loop, the driver will call
 *    xdp_do_flush(), which will flush all the different bulk queues,
 *    thus completing the redirect. Note that xdp_do_flush() must be
 *    called before napi_complete_done() in the driver, as the
 *    XDP_REDIRECT logic relies on being inside a single NAPI instance
 *    through to the xdp_do_flush() call for RCU protection of all
 *    in-kernel data structures.
```

```c
/* net/core/filter.c:4202 */
void xdp_do_flush(void)
{
	__dev_flush();
	__cpu_map_flush();
	__xsk_map_flush();
}
EXPORT_SYMBOL_GPL(xdp_do_flush);
```

```
包进来
  │
  ├─ ① bpf_redirect_map()  ──→ 查 map，结果存进 per-CPU bpf_redirect_info
  │                              （此时包还没动）
  ├─ ② 程序返回 XDP_REDIRECT
  │     └→ xdp_do_redirect() ──→ xdp_convert_buff_to_frame()
  │                              → 进 per-CPU 批量队列（devmap 16 / cpumap 8）
  │
  ├─ （继续处理 NAPI poll 里的其他包，每个都重复 ①②）
  │
  └─ ③ NAPI poll 即将结束 → xdp_do_flush()
        ├─ __dev_flush()      → devmap 批量队列 → ndo_xdp_xmit()
        ├─ __cpu_map_flush()  → cpumap 批量队列 → ptr_ring → 唤醒 kthread
        └─ __xsk_map_flush()  → xsk 批量队列   → 发布 RX ring producer
```

### 为什么要这么设计

| 原因 | 说明 |
|------|------|
| **摊销开销** | `ndo_xdp_xmit()` / doorbell MMIO 是 per-batch 的成本，一次发 16 个比发 16 次便宜得多 |
| **RCU 保护窗口** | map 里的目标指针（`bpf_dtab_netdev` 等）靠"整个 redirect 过程在同一个 NAPI poll 内"来保证不被并发释放，**不需要额外的 `rcu_read_lock()`** |
| **驱动页回收时序** | cpumap 的入队注释明确写了：必须先把 `xdp_frame` 排进队列，否则会和驱动的 page-refcnt 回收技巧（如 ixgbe）竞争 |

> **推论：`XDP_REDIRECT` 的延迟不是单包的，是批的。**
> 一个批次里第 16 个包和第 1 个包同时被送走，
> 但第 16 个包在队列里躺了更久。这一点与
> [chapter-06/02](../../chapter-06-af-xdp/notes/02-af-xdp-lwn.md) 第四节讲的
> AF_XDP flush 语义是同一个机制的两种表现。

---

## 二、四个目的地总览

| 目标 | 写法 | 载体 | 终点 | 是否构建 `sk_buff` |
|------|------|------|------|-------------------|
| **另一张网卡** | `bpf_redirect_map(&devmap, ifindex, flags)` | `xdp_frame` → per-CPU 批量队列（16） | `ndo_xdp_xmit()` | ❌ 不构建 |
| **另一个 CPU** | `bpf_redirect_map(&cpumap, cpu, flags)` | `xdp_frame` → `ptr_ring` → kthread | `netif_receive_skb_list()` | ✅ **构建**（在目标 CPU 上批量构建） |
| **AF_XDP socket** | `bpf_redirect_map(&xsks_map, qid, flags)` | 直接填 RX ring | 用户态 UMEM | ❌ 不构建（零拷贝） |
| **按 ifindex 转发** | `bpf_redirect(ifindex, 0)` | 同 devmap（走通用路径） | `ndo_xdp_xmit()` | ❌ 不构建 |

`xdp_do_redirect()` 的分流（`net/core/filter.c`）：

```c
int xdp_do_redirect(struct net_device *dev, struct xdp_buff *xdp,
		    struct bpf_prog *xdp_prog)
{
	struct bpf_redirect_info *ri = this_cpu_ptr(&bpf_redirect_info);
	enum bpf_map_type map_type = ri->map_type;

	if (map_type == BPF_MAP_TYPE_XSKMAP)
		return __xdp_do_redirect_xsk(ri, dev, xdp, xdp_prog);

	return __xdp_do_redirect_frame(ri, dev, xdp_convert_buff_to_frame(xdp),
				       xdp_prog);
}
```

**除 XSKMAP 之外，所有 redirect 都要先做一次 `xdp_convert_buff_to_frame()`**
——把 `xdp_buff` 转成 `xdp_frame`。这一步会把 `struct xdp_frame` 结构
**写进包自己的 headroom 里**（这也是 XDP 强制要求 headroom 的硬原因，
详见 [chapter-03/04](../../chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md)）。

> **所以 XSKMAP 是唯一"不做 frame 转换"的目的地**——因为包本来就在 UMEM 里，
> 不需要搬。这也是零拷贝路径最短的结构性原因。

---

## 三、DEVMAP：内核态 L2 转发

### 3.1 批量队列：每个 (目标设备, CPU) 一个，容量 16

```c
/* include/net/xdp.h:190 */
#define XDP_BULK_QUEUE_SIZE	16
/* include/net/xdp.h:384 */
#define DEV_MAP_BULK_SIZE XDP_BULK_QUEUE_SIZE
```

```c
/* kernel/bpf/devmap.c:56 */
struct xdp_dev_bulk_queue {
	struct xdp_frame *q[DEV_MAP_BULK_SIZE];
	struct net_device *dev;
	struct net_device *dev_rx;
	struct bpf_prog *xdp_prog;
	struct list_head flush_node;
	unsigned int count;
};
```

入队（`bq_enqueue()`，devmap.c:441）：

```c
	struct xdp_dev_bulk_queue *bq = this_cpu_ptr(dev->xdp_bulkq);

	if (unlikely(bq->count == DEV_MAP_BULK_SIZE))
		bq_xmit_all(bq, 0);       /* 满了就先发一批 */

	/* Ingress dev_rx will be the same for all xdp_frame's in
	 * bulk_queue, because bq stored per-CPU and must be flushed
	 * from net_device drivers NAPI func end.
	 */
	if (!bq->dev_rx) {
		bq->dev_rx = dev_rx;
		bq->xdp_prog = xdp_prog;
		list_add(&bq->flush_node, flush_list);
	}

	bq->q[bq->count++] = xdpf;
```

**两个易错点：**

1. **批量队列是 per-(目标设备, CPU) 的**，不是全局的。
   从 3 个不同入口网卡 redirect 到同一个出口网卡，会有 3×CPU 个队列在竞争。
2. **队列满（16）会立刻 `bq_xmit_all()`**，不等 NAPI poll 结束。
   所以高包速下"批量"其实是被 16 切碎的多次发送，不是一次大批量。

### 3.2 发送：二级 XDP 程序 + `ndo_xdp_xmit()`

```c
/* kernel/bpf/devmap.c:361 */
static void bq_xmit_all(struct xdp_dev_bulk_queue *bq, u32 flags)
{
	...
	for (i = 0; i < cnt; i++)
		prefetch(bq->q[i]);

	if (bq->xdp_prog) {
		to_send = dev_map_bpf_prog_run(bq->xdp_prog, bq->q, cnt, dev);
		if (!to_send)
			goto out;
	}

	sent = dev->netdev_ops->ndo_xdp_xmit(dev, to_send, bq->q, flags);
	if (sent < 0) {
		/* If ndo_xdp_xmit fails with an errno, no frames have
		 * been xmit'ed. */
		err = sent;
		sent = 0;
	}

	/* If not all frames have been transmitted, it is our
	 * responsibility to free them */
	for (i = sent; unlikely(i < to_send); i++)
		xdp_return_frame_rx_napi(bq->q[i]);

out:
	bq->count = 0;
	trace_xdp_devmap_xmit(bq->dev_rx, dev, sent, cnt - sent, err);
}
```

> ### ⚠️ 关键差异：devmap 没有重排队
> 普通 TX 路径里，驱动返回 `NETDEV_TX_BUSY` 时内核会 requeue 稍后重试。
> **devmap 不会**——`ndo_xdp_xmit()` 没发完的帧被直接 `xdp_return_frame_rx_napi()`
> 释放回内存池，也就是**丢弃**。
>
> 这是 XDP 转发丢包的头号来源，而且它**不产生任何 errno 给调用者**：
> 你只能在 `trace_xdp_devmap_xmit` 的 `drops` 参数里看到（`cnt - sent`）。

### 3.3 目标合法性检查（`is_valid_dst()`，devmap.c:533）

```c
static bool is_valid_dst(struct bpf_dtab_netdev *obj, struct xdp_frame *xdpf)
{
	if (!obj)
		return false;
	if (!(obj->dev->xdp_features & NETDEV_XDP_ACT_NDO_XMIT))
		return false;
	if (unlikely(!(obj->dev->xdp_features & NETDEV_XDP_ACT_NDO_XMIT_SG) &&
		     xdp_frame_has_frags(xdpf)))
		return false;
	if (xdp_ok_fwd_dev(obj->dev, xdp_get_frame_len(xdpf)))
		return false;
	return true;
}
```

| 拒绝原因 | 条件 | 说明 |
|---------|------|------|
| 出口设备没有 `ndo_xdp_xmit` | `NETDEV_XDP_ACT_NDO_XMIT` 缺失 | 虚拟设备（如 veth 的某些模式）、隧道设备常见 |
| 多段帧但设备不支持 SG | `NETDEV_XDP_ACT_NDO_XMIT_SG` 缺失 | 开了 `XDP_USE_SG` / 大 MTU 时要特别注意 |
| 帧长超过出口 MTU | `xdp_ok_fwd_dev()` | **入口 MTU > 出口 MTU 时静默丢包** |

### 3.4 devmap 的两个高级特性

| 特性 | 说明 |
|------|------|
| **二级 XDP 程序** | devmap 的 value 可以带 `bpf_prog.fd`，在**出口设备上下文**再跑一个 XDP 程序（`BPF_XDP_DEVMAP` 类型）。适合做出口侧的最后一道过滤/改写 |
| **`BPF_F_BROADCAST` 组播** | `dev_map_redirect_clone()`（devmap.c:687）会为每个目标 `xdpf_clone()` 一份。注意：**克隆需要分配内存，失败就是丢包** |

---

## 四、CPUMAP：⚠️ 它不省 `sk_buff`，只是把分配挪到目标 CPU

这是本篇最需要纠正的认知。

### 4.1 v6.6 的实现：kthread + `ptr_ring`

```c
/* kernel/bpf/cpumap.c:57 */
struct bpf_cpu_map_entry {
	u32 cpu;    /* kthread CPU and map index */
	int map_id;

	/* XDP can run multiple RX-ring queues, need __percpu enqueue store */
	struct xdp_bulk_queue __percpu *bulkq;

	/* Queue with potential multi-producers, and single-consumer kthread */
	struct ptr_ring *queue;
	struct task_struct *kthread;

	struct bpf_cpumap_val value;
	struct bpf_prog *prog;
	...
};
```

**v6.6 的 cpumap 用的是 kthread，不是 NAPI。** 每个目标 CPU 一个
`kthread` + 一个 `ptr_ring`。

### 4.2 kthread 主循环（`cpu_map_kthread_run()`，cpumap.c:262）

```c
	while (!kthread_should_stop() || !__ptr_ring_empty(rcpu->queue)) {
		...
		/* ① 队列空就睡，被唤醒再继续 */
		if (__ptr_ring_empty(rcpu->queue)) {
			set_current_state(TASK_INTERRUPTIBLE);
			if (__ptr_ring_empty(rcpu->queue)) {
				schedule();
				sched = 1;
			} else {
				__set_current_state(TASK_RUNNING);
			}
		} else {
			sched = cond_resched();
		}

		/* ② 批量出队，一次最多 CPUMAP_BATCH 个 */
		n = __ptr_ring_consume_batched(rcpu->queue, frames, CPUMAP_BATCH);
		...
			prefetchw(page);          /* 把 page 拉到本 CPU */

		/* ③ 在目标 CPU 上再跑一个 XDP 程序 */
		nframes = cpu_map_bpf_prog_run(rcpu, frames, xdp_n, &stats, &list);

		/* ④ 批量分配 skb 头 */
		if (nframes) {
			m = kmem_cache_alloc_bulk(skbuff_cache, gfp, nframes, skbs);
			if (unlikely(m == 0)) {
				...
				kmem_alloc_drops += nframes;   /* 分配失败 → 全丢 */
			}
		}

		/* ⑤ xdp_frame → sk_buff */
		local_bh_disable();
		for (i = 0; i < nframes; i++) {
			skb = __xdp_build_skb_from_frame(xdpf, skb, xdpf->dev_rx);
			if (!skb) { xdp_return_frame(xdpf); continue; }
			list_add_tail(&skb->list, &list);
		}
		/* ⑥ 送进协议栈 */
		netif_receive_skb_list(&list);

		trace_xdp_cpumap_kthread(rcpu->map_id, n, kmem_alloc_drops, sched, &stats);
		local_bh_enable();
	}
```

### 4.3 所以 cpumap 到底是什么

```
XDP（入口 CPU）
  → xdp_convert_buff_to_frame()
  → per-CPU 批量队列（CPU_MAP_BULK_SIZE = 8）
  → [xdp_do_flush] → ptr_ring（qsize，上限 16384）
  → 唤醒目标 CPU 的 kthread          ← ⚠️ 一次调度
  → 目标 CPU：批量出队（CPUMAP_BATCH = 8）
  → 跑二级 XDP 程序
  → kmem_cache_alloc_bulk(skbuff_cache)   ← ⚠️ 仍然分配 skb
  → __xdp_build_skb_from_frame()
  → netif_receive_skb_list()        ← 进入协议栈
```

**三个必须记住的事实：**

1. **`sk_buff` 还是要分配的**，只是从"入口 CPU 上逐包分配"变成了
   "目标 CPU 上批量分配（`kmem_cache_alloc_bulk`）"。
   cpumap 省的是**入口 CPU 的开销和锁竞争**，不是 skb 本身。
2. **有一次线程调度**。kthread 队列空时会 `schedule()` 睡下去，
   唤醒再跑。这决定了 **cpumap 不是低延迟机制**——它优化的是
   "多核间的负载均衡"，不是"单包延迟"。
3. **终点是协议栈**（`netif_receive_skb_list()`），不是用户态。
   想要旁路到用户态请用 [XSKMAP / AF_XDP](../../chapter-06-af-xdp/)。

### 4.4 CPUMAP vs RPS

| 维度 | RPS（传统） | CPUMAP |
|------|------------|--------|
| 分发点 | 收包软中断里，`sk_buff` **已分配之后** | XDP 层，`sk_buff` **分配之前** |
| 搬运的东西 | 已构建的 `sk_buff` | `xdp_frame`（更小的结构体） |
| 跨 CPU 载体 | per-CPU `input_pkt_queue` + IPI | `ptr_ring` + kthread 唤醒 |
| skb 分配位置 | 入口 CPU | **目标 CPU，批量分配** |
| 调度策略 | 内核固定的 hash | **BPF 程序自定义**（可以按 UDP 端口、组播组、五元组任意分发） |
| 能否过滤 | ❌ 只能选 CPU | ✅ 二级 XDP 程序可以在搬运前后各过滤一次 |
| 延迟 | 较低（软中断内完成） | **较高（多一次线程调度）** |

> **选型**：想"按业务规则把不同行情流钉到不同核"→ cpumap 合适
> （灵活、能过滤）。想"最低延迟地把包分散开"→ RSS / flow steering 更直接，
> 它在硬件层就分好了，根本不用搬运。

### 4.5 cpumap 的容量与统计

| 参数 | 值 | 位置 |
|------|-----|------|
| `CPU_MAP_BULK_SIZE`（入队批量） | **8**（注释：8 == 64 位架构上的一条 cacheline） | cpumap.c:45 |
| `CPUMAP_BATCH`（出队批量） | **8** | cpumap.c:236 |
| `qsize` 上限 | **16384**（超过返回 `-EOVERFLOW`） | cpumap.c:540 |
| `max_entries` 上限 | `NR_CPUS`（超过返回 `-E2BIG`） | cpumap.c:96 |
| value 可以只给 `qsize` | 或 `qsize` + `bpf_prog.fd` | cpumap.c:90 |
| `qsize == 0` | **等价于删除该条目** | cpumap.c:547 |

```c
/* include/net/xdp.h:212 */
struct xdp_cpumap_stats {
	unsigned int redirect;   /* 二级程序返回 XDP_REDIRECT 的数量 */
	unsigned int pass;       /* 送到协议栈的数量 */
	unsigned int drop;       /* 二级程序丢弃的数量 */
};
```

---

## 五、XSKMAP：唯一"不做 frame 转换"的目的地

```c
/* net/core/filter.c — xdp_do_redirect() */
	if (map_type == BPF_MAP_TYPE_XSKMAP)
		return __xdp_do_redirect_xsk(ri, dev, xdp, xdp_prog);
	/* 其他所有类型都要先转换 */
	return __xdp_do_redirect_frame(ri, dev, xdp_convert_buff_to_frame(xdp), ...);
```

机制与批量语义见：

- [chapter-06/01](../../chapter-06-af-xdp/notes/01-af-xdp.md) 第六节：`xsk_rcv()` 逐包判定 zc/copy
- [chapter-06/02](../../chapter-06-af-xdp/notes/02-af-xdp-lwn.md) 第四节：`__xsk_map_flush()` 与批量发布

---

## 六、观测：redirect 的 tracepoint 是唯一的可见性来源

`XDP_REDIRECT` 的失败**不会返回 errno 给调用者**（BPF 程序只拿到 `XDP_REDIRECT`），
所有丢弃都只在 tracepoint 里可见。

| tracepoint | 触发 | 关键参数 | 含义 |
|-----------|------|---------|------|
| `xdp_redirect` | `bpf_redirect()` 成功 | `tgt_index`, `act` | 按 ifindex 转发 |
| `xdp_redirect_err` | `bpf_redirect()` 失败 | `err` | **唯一能看到 `XDP_ABORTED` 的地方** |
| `xdp_redirect_map` | map redirect 成功 | `map_id`, `tgt_index` | — |
| `xdp_redirect_map_err` | map redirect 失败（含 lookup 失败） | `err` | — |
| `xdp_devmap_xmit` | `bq_xmit_all()` | `sent`, **`drops`**, `err` | **devmap 丢包看这里**（`cnt - sent`） |
| `xdp_cpumap_enqueue` | cpumap 批量入队 | `processed`, **`drops`**, `to_cpu` | **ptr_ring 满 → 丢包看这里** |
| `xdp_cpumap_kthread` | kthread 一轮处理 | `n`, **`kmem_alloc_drops`**, `sched`, `stats` | **skb 分配失败看这里** |

```bash
# devmap 丢包（出口驱动没发完 → 内核直接释放）
bpftrace -e 'tracepoint:xdp:xdp_devmap_xmit /args->drops > 0/ { @[args->err] = sum(args->drops); }'

# cpumap 入队丢包（ptr_ring 满 → 加大 qsize）
bpftrace -e 'tracepoint:xdp:xdp_cpumap_enqueue /args->drops > 0/ { @[args->to_cpu] = sum(args->drops); }'

# cpumap kthread 里 skb 分配失败
bpftrace -e 'tracepoint:xdp:xdp_cpumap_kthread /args->kmem_alloc_drops > 0/ { @ = sum(args->kmem_alloc_drops); }'

# redirect 目标查找失败（XDP_ABORTED 在这里）
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'

# 概览
perf list 'xdp:*'
```

---

## HFT 要点

- **`XDP_REDIRECT` 是批量的**，包在 per-CPU 队列里等到 NAPI poll 结束才被送走。
  批量大小：devmap **16**、cpumap **8**。别用单包延迟的眼光看它。
- **devmap 没有重排队**。`ndo_xdp_xmit()` 没发完的帧直接被释放（丢弃），
  只在 `trace_xdp_devmap_xmit` 的 `drops` 里可见。
- **devmap 会静默丢弃超过出口 MTU 的帧**（`xdp_ok_fwd_dev()`）。
  做跨网卡转发时两端 MTU 必须一致，否则大包全丢且毫无提示。
- **cpumap 不省 `sk_buff`**，它把分配移到目标 CPU 上批量做。
  它换来的是"入口 CPU 上不分配"和"BPF 自定义分发策略"。
- **cpumap 有一次线程调度**（kthread 睡/醒），**不是低延迟机制**。
  低延迟的分核请用硬件 RSS / flow steering。
- **cpumap 的终点是协议栈**，不是用户态。旁路到用户态只能走 XSKMAP。
- **`xsk_map` 是唯一跳过 `xdp_convert_buff_to_frame()` 的目的地**——
  包已经在 UMEM 里了，不需要搬。这是零拷贝路径最短的结构性原因。

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（4.8+ / v6.6） |
|------|-------------|---------------------|
| 转发决策点 | 路由表 / bridge，都在 `sk_buff` 上 | XDP 层，`sk_buff` 之前 |
| 跨网卡转发 | `dev_queue_xmit()`，重新走 qdisc | devmap → `ndo_xdp_xmit()`，**绕过 qdisc**（也绕过了 tc egress） |
| 跨 CPU 分发 | RPS（skb 已分配后） | cpumap（skb 分配前，BPF 自定义策略） |
| 转发失败处理 | 重排队 / 返回错误 | devmap **直接丢弃**，只在 tracepoint 可见 |
| 批量 | `netif_receive_skb_list()` 等局部批量 | **框架级批量**：三步 redirect + `xdp_do_flush()` |
| 用户态旁路 | 只有 `recvmsg` 拷贝 | XSKMAP 零拷贝 |

---

## 代码自测

<details>
<summary><b>Q1：</b>你在 XDP 程序里对 1000 个包返回 <code>XDP_REDIRECT</code>（目标是 devmap），出口网卡却只发出去 940 个。<code>bpftool prog show</code> 一切正常，没有报错。包去哪了？</summary>

**最可能：出口驱动的 `ndo_xdp_xmit()` 没发完，剩下的被内核直接释放了。**

`bq_xmit_all()`（devmap.c:361）：

```c
	sent = dev->netdev_ops->ndo_xdp_xmit(dev, to_send, bq->q, flags);
	if (sent < 0) {
		/* If ndo_xdp_xmit fails with an errno, no frames have
		 * been xmit'ed. */
		err = sent;
		sent = 0;
	}

	/* If not all frames have been transmitted, it is our
	 * responsibility to free them */
	for (i = sent; unlikely(i < to_send); i++)
		xdp_return_frame_rx_napi(bq->q[i]);

out:
	bq->count = 0;
	trace_xdp_devmap_xmit(bq->dev_rx, dev, sent, cnt - sent, err);
```

**devmap 没有重排队机制。** 普通 TX 路径上驱动返回 `NETDEV_TX_BUSY` 时
内核会 requeue；XDP 转发不会——`sent < to_send` 的差额直接被
`xdp_return_frame_rx_napi()` 送回内存池，也就是**丢了**。

**而且这件事对 BPF 程序完全不可见**：程序拿到的返回值就是 `XDP_REDIRECT`，
丢包数只写在 tracepoint 的参数里。

**怎么确认：**

```bash
bpftrace -e 'tracepoint:xdp:xdp_devmap_xmit /args->drops > 0/ {
    printf("devmap drops=%d err=%d\n", args->drops, args->err); }'
```

**三个常见根因：**

| 根因 | 判据 | 解法 |
|------|------|------|
| 出口驱动的 `ndo_xdp_xmit` 队列满 | drops 零散出现、和突发流量相关 | 降速 / 加大出口队列 / 换驱动 |
| 帧长超过出口 MTU | **所有大包全丢**，小包正常 | `xdp_ok_fwd_dev()` 检查；两端 MTU 配一致 |
| 多段帧但出口不支持 SG | 只有大包/分片包丢 | 检查 `NETDEV_XDP_ACT_NDO_XMIT_SG`；或不开 `XDP_USE_SG` |

**排障顺序：先用 `xdp_ok_fwd_dev` 那条（MTU）排查**，它最容易查也最常见——
入口是 9000 MTU 的行情网卡、出口是 1500 的管理网卡时，大包 100% 全丢。
</details>

<details>
<summary><b>Q2：</b>你想用 cpumap "绕过 sk_buff 分配"来降低行情分发的延迟。这个想法哪里错了？cpumap 到底优化了什么？</summary>

**错在：cpumap 仍然分配 `sk_buff`，只是换了个 CPU 和换了个分配方式。**

看 kthread 主循环（cpumap.c:262）的第 ④⑤ 步：

```c
		/* ④ 批量分配 skb 头 */
		if (nframes) {
			m = kmem_cache_alloc_bulk(skbuff_cache, gfp, nframes, skbs);
			if (unlikely(m == 0)) {
				for (i = 0; i < nframes; i++)
					skbs[i] = NULL; /* effect: xdp_return_frame */
				kmem_alloc_drops += nframes;
			}
		}

		/* ⑤ xdp_frame → sk_buff */
		local_bh_disable();
		for (i = 0; i < nframes; i++) {
			struct xdp_frame *xdpf = frames[i];
			struct sk_buff *skb = skbs[i];

			skb = __xdp_build_skb_from_frame(xdpf, skb, xdpf->dev_rx);
			...
			list_add_tail(&skb->list, &list);
		}
		netif_receive_skb_list(&list);   /* ⑥ 进协议栈 */
```

**cpumap 真正优化的三件事：**

1. **入口 CPU 上不分配 `sk_buff`**。分配被推迟到目标 CPU 上做，
   而且用 `kmem_cache_alloc_bulk()` **批量**分配（一次最多 `CPUMAP_BATCH = 8` 个），
   比逐包 `kmem_cache_alloc()` 便宜。
2. **跨 CPU 搬运的是 `xdp_frame` 而不是 `sk_buff`**。
   `xdp_frame` 比 `sk_buff` 小得多，跨核 cacheline 传输成本低。
3. **分发策略由 BPF 程序自定义**。RPS 只能用内核固定的 hash；
   cpumap 可以按 UDP 端口、组播组、任意业务字段分发，
   还能挂二级 XDP 程序在目标 CPU 上再过滤一次。

**cpumap 没有优化（甚至恶化）的：**

- **延迟**。kthread 队列空时 `schedule()` 睡下去，包来了要唤醒——
  这是一次完整的线程调度。**cpumap 是吞吐/均衡优化，不是延迟优化。**
- **`sk_buff` 的存在性**。终点是 `netif_receive_skb_list()`，也就是协议栈。
  想旁路到用户态，唯一路径是 [XSKMAP / AF_XDP](../../chapter-06-af-xdp/)。

**如果你要的是"低延迟地把流量分到多个核"**：

```
优先：硬件 RSS / flow steering
      ethtool -N eth0 flow-type udp dst-port 12345 action 0
      （硬件层面就分好了，零搬运、零调度）

其次：AF_XDP 每队列一个 socket，用户态自己分发
      （旁路到用户态，无 skb、无协议栈）

最后：cpumap
      （适合"策略复杂 + 最终还是要走协议栈"的场景）
```
</details>

<details>
<summary><b>Q3：</b>XDP 程序对一批包全部返回 <code>XDP_REDIRECT</code>，为什么这些包不是"一个一个"被送走的？批量大小是多少？这意味着什么？</summary>

**因为 `XDP_REDIRECT` 只是"入队"，真正送走要等 `xdp_do_flush()`。**

源码注释（`net/core/filter.c:4170`）把它讲得很清楚：

```
 * 1. bpf_redirect()/bpf_redirect_map() 查目标，存进 per-CPU bpf_redirect_info
 * 2. 返回 XDP_REDIRECT → xdp_do_redirect() 把帧放进 per-CPU 批量队列
 * 3. NAPI poll 结束前 → xdp_do_flush() 冲刷所有批量队列
```

```c
/* net/core/filter.c:4202 */
void xdp_do_flush(void)
{
	__dev_flush();
	__cpu_map_flush();
	__xsk_map_flush();
}
```

**批量大小：**

| 路径 | 常量 | 值 | 位置 |
|------|------|-----|------|
| devmap 入队 | `DEV_MAP_BULK_SIZE` = `XDP_BULK_QUEUE_SIZE` | **16** | `include/net/xdp.h:190/384` |
| cpumap 入队 | `CPU_MAP_BULK_SIZE` | **8** | `kernel/bpf/cpumap.c:45` |
| cpumap 出队 | `CPUMAP_BATCH` | **8** | `kernel/bpf/cpumap.c:236` |
| AF_XDP | 无独立批量，跟随 NAPI poll 的 weight | 默认 64 | — |

**三点含义：**

**① 队列是 per-(目标, CPU) 的**
```c
	struct xdp_dev_bulk_queue *bq = this_cpu_ptr(dev->xdp_bulkq);
```
所以"批量"不是全局的 16，而是每个 CPU、每个出口设备各有一个 16 槽队列。

**② 队列满会提前发送，不等 NAPI poll 结束**
```c
	if (unlikely(bq->count == DEV_MAP_BULK_SIZE))
		bq_xmit_all(bq, 0);
```
高包速下，一个 NAPI poll 里会触发多次 `bq_xmit_all()`，
"批量"实际被 16 切成了多段。

**③ 单包延迟取决于它在批里的位置**
批里第 1 个包和第 16 个包同时被送走，但第 16 个包多等了一段入队时间。
**所以"XDP 转发延迟"谈单包意义有限，要看分布（P50/P99）。**

**另外一条容易忽略的约束**（同一段注释）：

> `xdp_do_flush()` **must be called before `napi_complete_done()`** in the driver,
> as the XDP_REDIRECT logic relies on being inside a single NAPI instance
> through to the `xdp_do_flush()` call for **RCU protection** of all in-kernel
> data structures.

也就是说整个 redirect 过程的内存安全，靠的是"全在同一个 NAPI poll 里"
这个前提，而**没有额外的 `rcu_read_lock()`**。自己写驱动或改 XDP 框架时
动了这里，会产生很难复现的 use-after-free。
</details>

---

→ 本篇：[01 XDP_REDIRECT 的四个目的地](01-xdp-redirect.md)
→ 后一篇：[02 XDP vs DPDK](02-xdp-vs-dpdk.md)
→ 相关：[chapter-06 AF_XDP](../../chapter-06-af-xdp/) · [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) · [chapter-03 xdp_buff ↔ sk_buff](../../chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md)
