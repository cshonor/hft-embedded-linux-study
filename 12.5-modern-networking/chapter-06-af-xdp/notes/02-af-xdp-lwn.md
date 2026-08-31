# 02 — AF_XDP 工程实践：值不值、延迟预算、唤醒与批处理

> **对应 Rosen:** 无（AF_XDP 4.18+ 才存在）
> **内核版本：** 以 **v6.6** 为准，行为与常量均取自源码
> **源码索引：** `net/xdp/xsk.c`（`xsk_wakeup` 502、`__xsk_recvmsg` 873、`__xsk_sendmsg` 828、`xsk_poll` 910、`__xsk_map_flush` 382）、
> `net/xdp/xsk_buff_pool.c`（`xp_assign_dev` 149）、`include/uapi/linux/if_xdp.h`

---

## 文档概述

[01 篇](01-af-xdp.md)讲的是"接口每一步的约束"，本篇回答四个工程问题：

1. **值不值得用**——copy 模式和零拷贝的真实差距在哪一跳
2. **延迟从哪来**——端到端预算表，以及为什么你的实测总比理论慢几微秒
3. **什么时候必须 `poll()`/`sendto()`**——`XDP_USE_NEED_WAKEUP` 的完整语义
4. **批处理的账怎么算**——RX ring 的生产者索引到底在什么时候更新

| 笔记 | 侧重 |
|------|------|
| [01 接口精读](01-af-xdp.md) | 每一步的约束、errno、静默降级 |
| **02（本篇）** | 决策、延迟预算、唤醒、批处理、排障清单 |
| [03 UMEM 布局](03-af-xdp-umem-layout.md) | frame/chunk/headroom、copy vs zc 微观差异 |

---

## 一、先做决策：三条用户态收包路线

```
                      内核协议栈                        旁路程度
   ┌────────────────────────────────────────────────────┼──────────┐
   │                                                    │          │
  recvmsg()/recvfrom()                                  │  0%      │
   └─ 走完整协议栈 + 一次 copy_to_user                    │          │
   │                                                    │          │
  PF_PACKET / AF_PACKET (mmap v3)                       │  ~30%    │
   └─ 绕过 L3/L4，仍在 skb 层，零拷贝到 mmap ring          │          │
   │                                                    │          │
  AF_XDP zero-copy                                      │  ~95%    │
   └─ 无 skb、无协议栈、无拷贝，网卡直接写 UMEM             │          │
   │                                                    │          │
  DPDK                                                  │  100%    │
   └─ 连内核驱动都不要，用户态 PMD 独占网卡                 │          │
   └────────────────────────────────────────────────────┴──────────┘
```

| 维度 | 内核 socket | **AF_XDP zc** | DPDK |
|------|-----------|--------------|------|
| 是否分配 `sk_buff` | 是 | **否** | 否 |
| 是否经过协议栈 | 是 | **否** | 否 |
| 数据拷贝次数 | 1（`copy_to_user`） | **0** | 0 |
| 网卡是否独占 | 否 | **否（按队列）** | **是** |
| 内核网络功能是否可用 | 全可用 | **其他队列仍可用** | 不可用 |
| 需要 BPF 程序 | 否 | **是** | 否 |
| 需要 hugepage | 否 | 建议 | 必需 |
| 驱动 | 任意 | **需 `NETDEV_XDP_ACT_ZC`** | 需 PMD 支持 |
| 运维复杂度 | 低 | **中** | 高 |

> ### 决策口诀
> - **行情是小包、要跟内核共存（SSH/监控/组播管理流还要走栈）→ AF_XDP**
> - **只要这一张网卡的全部带宽、能接受它从内核消失 → DPDK**
> - **驱动不支持零拷贝 → 别用 AF_XDP**，走内核栈 + busy poll（`SO_BUSY_POLL`）
> - **包量很小（< 100 Kpps）→ 用内核 socket**，AF_XDP 的复杂度换不来什么

---

## 二、为什么 copy 模式不值得：每一跳的账

从 [01 篇](01-af-xdp.md)第六节知道，`xsk_rcv()`（xsk.c:348）按
`xdp->rxq->mem.type` 逐包分流。两条路径的差别：

```
【零拷贝】
  NIC DMA → UMEM frame（驱动 Rx 描述符直接指向 UMEM）
      → 驱动构造 xdp_buff（mem.type = MEM_TYPE_XSK_BUFF_POOL）
      → XDP_REDIRECT
      → __xsk_rcv_zc()：取 addr、填 desc          ← 就这两步
      → RX ring
      → 用户态读到"网卡刚写过的那块内存"

【copy 模式】
  NIC DMA → page_pool page（驱动自己的普通收包路径）
      → 驱动构造 xdp_buff（mem.type = MEM_TYPE_PAGE_*）
      → XDP_REDIRECT
      → __xsk_rcv()：
           xsk_buff_alloc(pool)        ← ① 从 UMEM 拿一个 frame
           memcpy(frame, pkt, len)     ← ② 整包复制
           xdp_return_buff(xdp)        ← ③ 把驱动的 page 还回 page_pool
      → RX ring
```

| 开销项 | copy | zero-copy | 说明 |
|--------|------|-----------|------|
| UMEM frame 分配 | 有 | 无 | 走 `xsk_buff_alloc()`，池空则 `rx_dropped++` |
| payload memcpy | **有（整包）** | 无 | 随包长线性增长 |
| 缓存触碰次数 | 2 次（源页 + 目标 frame） | 1 次 | copy 后源页还在 cache 里，污染后续访问 |
| `xdp_return_buff()` | 有 | 无 | 页回收路径 |
| 内存带宽 | 2× 包长 | 1× 包长 | 10 Gbps 线速下这是硬指标 |

> **关键认知：copy 模式只省掉了协议栈遍历，没省掉分配和拷贝。**
> 它相对 `recvmsg()` 的优势很有限，却要你付出写 BPF 程序、独占队列、
> 自己管 FILL ring 的代价。**不要用它。**

---

## 三、延迟预算：为什么实测总比"零拷贝"慢几微秒

零拷贝常被宣传为"亚微秒"，但实测端到端往往 2–10 μs。差额在这里：

| 阶段 | 说明 | 可控性 |
|------|------|-------|
| ① NIC DMA 写 UMEM | 由网卡决定，PCIe 往返 | ❌ 硬件 |
| ② **等待 NAPI poll 被调度** | 中断 → 软中断 → NAPI poll | ⚠️ 可用 busy poll 消除（见第五节） |
| ③ NAPI poll 循环跑 XDP | 每包执行 BPF + `__xsk_map_redirect()` | ✅ 程序复杂度 |
| ④ **等待 `xdp_do_flush()`** | **RX ring 的 producer 只在 flush 时更新** | ⚠️ 见第四节 |
| ⑤ 用户态循环看到 desc | 取决于你轮询的频率 | ✅ |
| ⑥ 用户态处理 | 你的策略代码 | ✅ |
| ⑦ 归还 FILL ring | 必须做，否则丢包 | ✅ |

**② 和 ④ 是最容易被忽略的两项**，它们能把"零拷贝"变成几微秒的等待。

- **② 的消除**：开 busy poll（第五节），让 `recvmsg()` 直接驱动 NAPI，
  跳过中断 → 软中断的调度延迟。
- **④ 的消除**：理解它，接受它——它是批处理换吞吐的必然代价。见下节。

---

## 四、批处理与 flush：RX ring 的生产者索引何时更新

这是本篇最有价值的一节。

### 4.1 redirect 是批量的，不是即时的

```c
/* net/xdp/xsk.c:368 */
int __xsk_map_redirect(struct xdp_sock *xs, struct xdp_buff *xdp)
{
	struct list_head *flush_list = this_cpu_ptr(&xskmap_flush_list);
	int err;

	err = xsk_rcv(xs, xdp);
	if (err)
		return err;

	if (!xs->flush_node.prev)
		list_add(&xs->flush_node, flush_list);   /* 只挂链表，还没提交 */

	return 0;
}

/* net/xdp/xsk.c:382 */
void __xsk_map_flush(void)
{
	struct list_head *flush_list = this_cpu_ptr(&xskmap_flush_list);
	struct xdp_sock *xs, *tmp;

	list_for_each_entry_safe(xs, tmp, flush_list, flush_node) {
		xsk_flush(xs);
		__list_del_clearprev(&xs->flush_node);
	}
}

/* net/xdp/xsk.c:326 */
static void xsk_flush(struct xdp_sock *xs)
{
	xskq_prod_submit(xs->rx);        /* ⭐ 现在才 publish producer */
	__xskq_cons_release(xs->pool->fq);
	sock_def_readable(&xs->sk);      /* 唤醒阻塞在 poll/select 的 */
}
```

**`__xsk_map_flush()` 由驱动在 NAPI poll 结束时通过 `xdp_do_flush()` 调用。**

### 4.2 这意味着什么

```
驱动 NAPI poll（一次处理 weight 个包，默认 64）
  ├─ 包 1 → XDP_REDIRECT → 进 RX ring 的 desc 数组（未发布 producer）
  ├─ 包 2 → XDP_REDIRECT → ...
  ├─ ...
  ├─ 包 N → XDP_REDIRECT → ...
  └─ poll 结束 → xdp_do_flush() → __xsk_map_flush() → xskq_prod_submit()
                                                        ↑
                                          此刻用户态才第一次看到这 N 个包
```

| 后果 | 影响 |
|------|------|
| **你永远一次看到一批包** | 单包延迟 = 该包在批次中的位置决定（队尾的包等得更久） |
| **低包速时延迟反而高** | 100 pps 时，一个包要等 NAPI poll 结束才被 publish |
| **`xsk_ring_cons__peek()` 返回的数量是批量的** | 循环要能处理 N 个，不能只处理 1 个 |
| **`sock_def_readable()` 也在 flush 里** | 阻塞式 `poll()` 的唤醒粒度是"每 NAPI poll 一次"，不是每包 |

### 4.3 批处理策略对照

| 策略 | 单包延迟 | 吞吐 | 适用 |
|------|---------|------|------|
| 一次处理 1 个 desc | 最低（批内第一个包） | 低（ring 操作开销占比高） | 极致延迟、低包速 |
| 一次处理 16–64 个 | 中 | 高 | **通用推荐** |
| 一次处理 256+ | 高（队尾包要等） | 最高 | 批处理/回放、不在乎尾延迟 |

**HFT 建议：从 32 开始测。** `TX_BATCH_SIZE` 在内核里就是 32（xsk.c:35），
而 NAPI poll 的 weight 默认 64——32 大致对应"半个 NAPI 批次"，
不会因为等一批而引入额外抖动。

### 4.4 FILL ring 归还也是批量的

`xsk_flush()` 里同时做了 `__xskq_cons_release(xs->pool->fq)`——
**内核释放 FILL ring 的消费位置也在 flush 时**。

所以用户态的最佳实践是"**收多少、还多少**"，在同一循环里完成：

```c
/* libbpf xsk.h 风格 */
uint32_t idx_rx = 0, idx_fill = 0, rcvd;

rcvd = xsk_ring_cons__peek(&rx, BATCH, &idx_rx);
if (!rcvd) {
        /* 没包：按需唤醒驱动（见第五节） */
        if (xsk_ring_prod__needs_wakeup(&fill))
            recvfrom(xsk_fd, NULL, 0, MSG_DONTWAIT, NULL, NULL);
        continue;
}

/* 1. 预留同等数量的 FILL 槽位 —— 关键：先预留，避免处理中途发现没槽位 */
while (xsk_ring_prod__reserve(&fill, rcvd, &idx_fill) != rcvd)
        ;   /* 实际要处理 FILL ring 满的情况，通常不会发生（收多少还多少） */

for (uint32_t i = 0; i < rcvd; i++) {
        const struct xdp_desc *d = xsk_ring_cons__rx_desc(&rx, idx_rx + i);
        uint64_t addr = d->addr;

        process(xsk_umem__get_data(umem_area, addr), d->len);   /* 你的策略 */

        *xsk_ring_prod__fill_addr(&fill, idx_fill + i) = addr;  /* 归还同一个 frame */
}
xsk_ring_prod__submit(&fill, rcvd);
xsk_ring_cons__release(&rx, rcvd);
```

> **⚠️ 为什么"收多少还多少"能保证 FILL ring 不满？**
> 因为每个包都对应一个从 FILL ring 取走的 frame，你归还的数量正是取走的数量。
> FILL ring 的空闲槽位在数学上始终 ≥ 在途包数。
> **破坏这个不变量的做法**（比如把 addr 存起来延迟归还做零拷贝转发）会直接导致
> `rx_fill_ring_empty_descs` 上涨。

---

## 五、`XDP_USE_NEED_WAKEUP`：什么时候必须 `poll()` / `sendto()`

### 5.1 机制

零拷贝模式下，**包是在驱动的 NAPI 上下文里被投递的**。
如果驱动此时没有在轮询（比如 RX 中断被关闭、或者 NAPI 已经退出），
你填再多 FILL ring 也没人去取。所以需要"踢"驱动一下。

```c
/* net/xdp/xsk.c:502 —— 踢驱动的本质 */
static int xsk_wakeup(struct xdp_sock *xs, u8 flags)
{
	struct net_device *dev = xs->dev;
	return dev->netdev_ops->ndo_xsk_wakeup(dev, xs->queue_id, flags);
}
```

驱动通过 `xsk_set_rx_need_wakeup()` / `xsk_set_tx_need_wakeup()`（xsk.c:39 / 49）
设置标志位，用户态通过 mmap 出来的 ring flags 读：

```c
/* xsk.c:39 —— 注意：RX 的标志位打在 FILL ring 上 */
void xsk_set_rx_need_wakeup(struct xsk_buff_pool *pool)
{
	if (pool->cached_need_wakeup & XDP_WAKEUP_RX)
		return;
	pool->fq->ring->flags |= XDP_RING_NEED_WAKEUP;
	pool->cached_need_wakeup |= XDP_WAKEUP_RX;
}
```

| 方向 | 用户态检查哪个 ring 的 flags | 唤醒方式 |
|------|---------------------------|---------|
| RX | **FILL ring** | `recvfrom(fd, ..., MSG_DONTWAIT)` 或 `poll()` |
| TX | **TX ring** | `sendto(fd, ..., MSG_DONTWAIT)` 或 `poll()` |

libbpf 封装：`xsk_ring_prod__needs_wakeup(&fill)` / `xsk_ring_prod__needs_wakeup(&tx)`。

### 5.2 三条规则

**规则一：不设 `XDP_USE_NEED_WAKEUP` 就要无条件 syscall**

```c
/* xp_assign_dev()，xsk_buff_pool.c:172 */
	if (flags & XDP_USE_NEED_WAKEUP)
		pool->uses_need_wakeup = true;
	/* Tx needs to be explicitly woken up the first time. Also
	 * for supporting drivers that do not implement this feature.
	 * They will always have to call sendto() or poll().
	 */
	pool->cached_need_wakeup = XDP_WAKEUP_TX;
```

不设该 flag 时 `uses_need_wakeup = false`，驱动**不会**维护那个标志位，
你就只能每次循环无条件调用 `recvfrom()`/`sendto()`——
在低包速时这是每秒几十万次无谓的 syscall。

**规则二：TX 第一次必须唤醒**

`cached_need_wakeup` 初值就是 `XDP_WAKEUP_TX`。
**第一次 `sendto()` 之前，光把描述符写进 TX ring 不会触发发送。**

**规则三：开了 busy poll 就不需要唤醒**

```c
/* net/xdp/xsk.c:807 */
static bool xsk_no_wakeup(struct sock *sk)
{
#ifdef CONFIG_NET_RX_BUSY_POLL
	/* Prefer busy-polling, skip the wakeup. */
	return READ_ONCE(sk->sk_prefer_busy_poll) && READ_ONCE(sk->sk_ll_usec) &&
		READ_ONCE(sk->sk_napi_id) >= MIN_NAPI_ID;
#else
	return false;
#endif
}
```

```c
/* __xsk_recvmsg()，xsk.c:873 */
	if (sk_can_busy_loop(sk))
		sk_busy_loop(sk, 1);        /* ⭐ 直接驱动 NAPI，不等中断 */

	if (xsk_no_wakeup(sk))
		return 0;                   /* ⭐ 跳过 ndo_xsk_wakeup */

	if (xs->pool->cached_need_wakeup & XDP_WAKEUP_RX && xs->zc)
		return xsk_wakeup(xs, XDP_WAKEUP_RX);
```

**busy poll + NEED_WAKEUP 是最优组合**：`recvfrom()` 进来先跑
`sk_busy_loop()` 直接在当前上下文驱动 NAPI 收包（消除第 ② 项延迟），
然后**跳过** `ndo_xsk_wakeup`（省掉一次 IPI / doorbell）。

```c
int busy_poll_usec = 50;
setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL,      &busy_poll_usec, sizeof(int));
int prefer = 1;
setsockopt(fd, SOL_SOCKET, SO_PREFER_BUSY_POLL, &prefer,        sizeof(int));
```

> ⚠️ 前提是内核开了 `CONFIG_NET_RX_BUSY_POLL`。另外 `sk_napi_id` 要有效——
> 由 `sk_mark_napi_id_once_xdp()`（`xsk_rcv_check` 里，xsk.c:322）在首次收包时设置，
> 所以**必须先收到至少一个包，busy poll 才完全生效**。

### 5.3 ⚠️ `recvmsg()` 必须是非阻塞的

```c
/* __xsk_recvmsg()，xsk.c:879 */
	if (unlikely(need_wait))
		return -EOPNOTSUPP;
```

`MSG_DONTWAIT` 不给 → **`-EOPNOTSUPP`**。AF_XDP 不支持阻塞接收，
休眠/唤醒一律走 `poll()`。

### 5.4 `poll()` 有副作用

```c
/* xsk_poll()，xsk.c:910 */
	pool = xs->pool;
	if (pool->cached_need_wakeup) {
		if (xs->zc)
			xsk_wakeup(xs, pool->cached_need_wakeup);
		else if (xs->tx)
			/* Poll needs to drive Tx also in copy mode */
			xsk_generic_xmit(sk);
	}
```

**`poll()` 不只是"等"，它还会去踢驱动。**
在忙轮询循环里每轮都 `poll()` 等于每轮一次 syscall + 一次 `ndo_xsk_wakeup`。
正确做法是：**只在确实要休眠时才 `poll()`，忙时用 `xsk_ring_prod__needs_wakeup()` 判断。**

### 5.5 TX 侧的一个细节

```c
/* xsk.c:291 */
static bool xsk_tx_writeable(struct xdp_sock *xs)
{
	if (xskq_cons_present_entries(xs->tx) > xs->tx->nentries / 2)
		return false;
	return true;
}
```

`poll()` 的 `POLLOUT` 只在 **TX ring 未完成的条目少于一半**时才置位。
这是个粗糙的背压信号，不要用它做精确流控——用 COMPLETION ring 的回收速率。

---

## 六、容量规划：UMEM 和 ring 该多大

### 6.1 四个 ring 的推荐值

| ring | 推荐大小 | 理由 |
|------|---------|------|
| FILL | **≥ RX ring 大小 + 批处理余量**，常取 2048–4096 | 水位是生命线；空了就丢包（`rx_fill_ring_empty_descs`） |
| RX | 2048 | 太小吃不下 NAPI 批次；太大则批内延迟高 |
| TX | 2048 | 同上 |
| COMPLETION | ≥ TX | 必须能装下所有在途发送帧 |

> **所有 ring 大小必须是 2 的幂**（`xsk_queue` 用 `ring_mask = nentries - 1` 做取模）。

### 6.2 UMEM 大小

```
UMEM_size = chunk_size × frame_count
```

| 场景 | chunk_size | frame_count | UMEM |
|------|-----------|-------------|------|
| 行情小包（< 1500 B） | 2048 | 4096–16384 | 8–32 MB |
| 需要 headroom 256 | 4096 | 4096 | 16 MB |
| 高吞吐（10 Gbps） | 2048 | 65536 | 128 MB |

**约束（[01 篇](01-af-xdp.md)第三节）：**
- `chunk_size ∈ [2048, PAGE_SIZE]` 且为 2 的幂 → 实际只有 2048 / 4096
- `UMEM_size % chunk_size == 0`（不开 unaligned 模式）
- UMEM 起始地址**必须页对齐**
- **吃 `RLIMIT_MEMLOCK`**（`pin_user_pages(FOLL_LONGTERM)`）

### 6.3 在途帧守恒（务必理解）

```
frame_count = FILL 中的 + 驱动持有的 + RX 中的 + 用户态处理中的 + COMPLETION 中的
```

只要任何一个环节"囤积"帧，FILL ring 水位就会下降，最终丢包。
**典型囤积点：用户态把 addr 存起来做延迟处理（转发、批量落盘）。**

---

## 七、多队列与多进程

### 7.1 硬约束：一个 (dev, queue_id) 只能有一个 xsk

```c
/* xp_assign_dev()，xsk_buff_pool.c:164 */
	if (xsk_get_pool_from_qid(netdev, queue_id))
		return -EBUSY;
```

所以：

| 需求 | 方案 |
|------|------|
| 多进程都要收同一队列 | ❌ 做不到。必须用户态单进程收 + 分发 |
| 多进程各收一个队列 | ✅ `ethtool -L eth0 combined N`，每队列一个 xsk |
| 多队列、共享一个 UMEM | ✅ `XDP_SHARED_UMEM`（见下） |
| 想用 `SO_REUSEPORT` 做负载均衡 | ❌ **AF_XDP 不支持**。`SO_REUSEPORT` 是 L4 的概念 |

### 7.2 `XDP_SHARED_UMEM`

```c
/* xsk_bind()，xsk.c:1128 */
	if (flags & XDP_SHARED_UMEM) {
		if ((flags & XDP_COPY) || (flags & XDP_ZEROCOPY) ||
		    (flags & XDP_USE_NEED_WAKEUP) || (flags & XDP_USE_SG)) {
			/* Cannot specify flags for shared sockets. */
			err = -EINVAL;
			...
		}
		if (xs->umem) {
			/* We have already our own. */
			err = -EINVAL;
			...
		}
		...
	}
```

| 约束 | 说明 |
|------|------|
| **不许带任何模式 flag** | 模式由第一个 socket 决定，后续继承 |
| **不许自己有 UMEM** | 只能共享别人的 |
| **同一个 (dev, queue_id)** | 共享 buffer pool（`xp_get_pool`） |
| **不同 (dev, queue_id)** | 通过 `xp_assign_dev_shared()` 挂到别的设备/队列 |
| FILL / COMPLETION ring | **每个 socket 各自一套**（只有 UMEM 是共享的） |

**HFT 用法**：一个进程管 UMEM + 一个 xsk per queue，用共享 UMEM 让多个 worker 线程
（每个线程自己的 FILL/CQ）共用同一块内存池，省内存且减少 NUMA 抖动。

### 7.3 与 RSS / flow steering 配合

```
# 把行情组播流固定到 0 号队列（让 AF_XDP 独占它）
ethtool -N eth0 flow-type udp dst-port 12345 action 0

# 或者用 RSS 把特定流哈希到指定队列
ethtool -X eth0 equal 4       # 4 个队列均分
ethtool -L eth0 combined 4    # 确认队列数
```

⚠️ **顺序很重要**：先配好队列数和流规则，再 bind xsk 并加载 XDP 程序。
中途改队列数会触发驱动 reconfigure，xsk 会被强制解绑。

---

## 八、与内核共存：哪些流量还走协议栈

AF_XDP 只接管**被 XDP 程序 redirect 进来的包**。其他包照常走内核栈。

```
                         ┌─ 行情 UDP（XDP 匹配）→ xsks_map → AF_XDP → 用户态
包进 0 号队列 → XDP prog ─┤
                         └─ 其他（ARP/ICMP/SSH）→ XDP_PASS → 内核协议栈

包进 1~N 号队列 → 没有 XDP 命中 → 全部走内核栈
```

| 流量 | 去向 | 注意 |
|------|------|------|
| 匹配规则的行情包 | AF_XDP | 内核看不到，tcpdump 抓不到 |
| 同队列的非匹配包 | 内核栈（若 flags = `XDP_PASS`） | **flags 写 0 会全丢**，见 [01 篇第五节](01-af-xdp.md) |
| 其他队列的包 | 内核栈 | 完全不受影响 |
| 本机发出的包 | 内核栈 → TX | AF_XDP 不接管 TX，除非你显式用 TX ring 发 |

> **运维意义**：你可以把 AF_XDP 部署在生产机器上而不影响 SSH、监控、NTP。
> 这是它相对 DPDK 最大的工程优势——DPDK 一旦接管网卡，
> 你就得再插一张卡跑管理流量。

---

## 九、排障清单

| # | 症状 | 检查 | 根因 / 解法 |
|---|------|------|------------|
| 1 | bind 成功但延迟 3 倍 | `getsockopt(XDP_OPTIONS)` 查 `XDP_OPTIONS_ZEROCOPY` | **静默降级到 copy**，bind 时加 `XDP_ZEROCOPY` |
| 2 | 收不到任何包 | `bpftool net show dev eth0` | **XDP 程序没挂** / 挂在了别的 ifindex |
| 3 | 收不到任何包（程序已挂） | XDP 程序的 match 逻辑 | 包没被 redirect；用 `bpftool prog show` 看 `run_cnt` |
| 4 | 只有部分队列收不到 | `xsks_map` 的 key | key 必须是 `ctx->rx_queue_index`，写死 0 就只有 0 号队列有 |
| 5 | SSH/ARP 全断 | XDP 程序的 redirect flags | **写了 0 → `XDP_ABORTED`**，改成 `XDP_PASS` |
| 6 | `rx_fill_ring_empty_descs` 涨 | 归还 frame 的逻辑 | 处理循环太慢 / 囤积了 addr / FILL ring 太小 |
| 7 | `rx_ring_full` 涨 | 消费循环 | 批处理太小 / 循环里做了重活（日志、系统调用） |
| 8 | `rx_dropped` 涨、`rx_ring_full` 为 0 | `frame_len` vs MTU | **包太大**：`frame_len = chunk_size - 256 - headroom`；开 `XDP_USE_SG` 或加大 chunk |
| 9 | 三个计数器混在一起看不清 | `getsockopt` 的 `optlen` | **传小了**会走 v1 分支把 `rx_queue_full` 折进 `rx_dropped` |
| 10 | UMEM 注册失败 `-ENOBUFS` | `ulimit -l` | **RLIMIT_MEMLOCK 不够**（`xdp_umem_account_pages`） |
| 11 | UMEM 注册失败 `-EINVAL` | UMEM 地址对齐 | 必须 `PAGE_ALIGNED`；`malloc()` 不行 |
| 12 | bind 返回 `-EBUSY` | 队列占用 | 该 (dev, queue) 已有 xsk；或 `setsockopt` 不在 `XSK_READY` 状态 |
| 13 | TX 第一次发不出去 | 是否调用过 `sendto()`/`poll()` | `cached_need_wakeup` 初值 `XDP_WAKEUP_TX`，**必须显式唤醒一次** |
| 14 | 低包速时延迟高 | 是否开 busy poll | 未开则要等中断；开 `SO_BUSY_POLL` + `SO_PREFER_BUSY_POLL` |
| 15 | 延迟抖动大 | NAPI flush 批次 | 见第四节；减小批处理大小 / 减小 `netdev_budget` |

---

## 十、观测命令速查

```bash
# ① 有没有挂 XDP、挂的什么
bpftool net show dev eth0
bpftool prog show id <ID>          # run_cnt / run_time → 单包 BPF 耗时

# ② xdp-tools 自带的统计（最方便）
xdp-stat -i eth0

# ③ 驱动层：包到底有没有进网卡
ethtool -S eth0 | grep -E 'rx_packets|rx_dropped|rx_missed|xdp'

# ④ 队列与流 steering
ethtool -l eth0                    # 当前/最大队列数
ethtool -n eth0                    # 已配置的 flow 规则

# ⑤ 中断与软中断分布（确认队列绑核正确）
cat /proc/irq/<irq>/smp_affinity_list
cat /proc/softirqs | grep NET_RX

# ⑥ redirect 错误（XDP_ABORTED 的唯一可见处）
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'

# ⑦ 确认是真零拷贝（C 代码，bind 之后）
#    getsockopt(fd, SOL_XDP, XDP_OPTIONS, &opts, &len)
#    opts.flags & XDP_OPTIONS_ZEROCOPY
```

---

## HFT 要点

- **决策树**：驱动不支持 zc → 不用 AF_XDP；要独占整卡 → 用 DPDK；
  要跟内核共存 → AF_XDP。copy 模式是伪旁路，别用。
- **延迟的两大隐藏项**：NAPI 调度等待（用 busy poll 消除）、
  `xdp_do_flush()` 批量发布（无法消除，只能用批处理大小去权衡）。
- **`XDP_USE_NEED_WAKEUP` + busy poll 是标准配置**。
  前者避免无谓 syscall，后者让 `recvfrom()` 直接在调用上下文驱动 NAPI。
- **批处理从 32 起测**，不要盲目加大——队尾包的延迟由批次大小决定。
- **"收多少还多少" 是 FILL ring 不变量的保证**。任何囤积 addr 的优化都会打破它。
- **一个队列一个 xsk，`SO_REUSEPORT` 无效**。多核扩展靠多队列 + RSS/flow steering。
- **UMEM 常驻内存且吃 memlock 限额**，容器里部署前先 `ulimit -l`。
- **`recvmsg()` 必须带 `MSG_DONTWAIT`**，否则直接 `-EOPNOTSUPP`。
- **XDP 程序务必用 `XDP_PASS` 作为 redirect 的 fallback flags**，
  否则非行情流量（含你的 SSH）会被 `XDP_ABORTED` 静默丢弃，
  而所有常规工具（tcpdump、ethtool）都显示"一切正常"。

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（4.18+ / v6.6） |
|------|-------------|---------------------|
| 用户态收包 | `recvmsg()` + 一次 `copy_to_user` | AF_XDP：零拷贝，包直接来自网卡 DMA |
| 唤醒模型 | 中断 → 软中断 → 唤醒进程 | **用户态可主动驱动**（busy poll）或显式踢驱动（`ndo_xsk_wakeup`） |
| 批处理 | `recvmmsg()`（系统调用级） | **内核级批处理**：`xdp_do_flush()` 一次发布整个 NAPI 批 |
| 背压 | 内核 socket 缓冲区自动 | **用户态自己管 FILL ring**，管不好就丢包 |
| 队列粒度 | 无（socket 与队列无关） | **socket 绑定到具体 (dev, queue_id)**，一对一 |
| 与内核共存 | 天然共存 | 需要显式用 `XDP_PASS` 让非目标流量回落内核栈 |
| 内存管理 | 内核管 | **用户态管 UMEM，钉页常驻，受 memlock 限额** |

---

## 代码自测

<details>
<summary><b>Q1：</b>你开了 <code>XDP_USE_NEED_WAKEUP</code>，收包循环里每次都调用 <code>recvfrom(..., MSG_DONTWAIT)</code>。高包速下没问题，低包速下 CPU 占用却很高。为什么？正确写法是什么？</summary>

**问题在于无条件调用 syscall。**

`recvfrom()` 落到 `__xsk_recvmsg()`（xsk.c:873）：

```c
	if (sk_can_busy_loop(sk))
		sk_busy_loop(sk, 1);

	if (xsk_no_wakeup(sk))
		return 0;

	if (xs->pool->cached_need_wakeup & XDP_WAKEUP_RX && xs->zc)
		return xsk_wakeup(xs, XDP_WAKEUP_RX);   /* → ndo_xsk_wakeup */
	return 0;
```

高包速下驱动一直在轮询，RX 标志位通常是清的，调用基本空转返回。
**但低包速下驱动可能停了轮询，你不调用就收不到包**——所以看起来"必须每次调"。

**正确的是：用 ring flags 做判断，只在驱动真的要求时才 syscall。**

```c
/* 内核在 xsk_set_rx_need_wakeup() 里把标志打在 FILL ring 上（xsk.c:39） */
static inline void kick_rx_if_needed(int fd, struct xsk_ring_prod *fill)
{
        if (xsk_ring_prod__needs_wakeup(fill))          /* 读 mmap 出来的 flags */
                recvfrom(fd, NULL, 0, MSG_DONTWAIT, NULL, NULL);
}
```

```c
for (;;) {
        rcvd = xsk_ring_cons__peek(&rx, BATCH, &idx_rx);
        if (rcvd) {
                /* ... 处理并归还 FILL ... */
                continue;                                /* 有包：继续忙循环 */
        }
        /* 没包：先问驱动需要唤醒吗，需要才 syscall */
        kick_rx_if_needed(fd, &fill);
        /* 或者：真的要休眠才 poll() */
        /* poll(&pfd, 1, timeout); */
}
```

**⚠️ 别忘了 `poll()` 有副作用**（xsk.c:910）：

```c
	if (pool->cached_need_wakeup) {
		if (xs->zc)
			xsk_wakeup(xs, pool->cached_need_wakeup);
		...
	}
```

在忙循环里每轮 `poll()` = 每轮一次 syscall + 一次 `ndo_xsk_wakeup`（可能是 IPI）。
**`poll()` 只应在你准备休眠时用。**

**更进一步的优化（消除 syscall）**：开 busy poll
（`SO_BUSY_POLL` + `SO_PREFER_BUSY_POLL`）。此时 `xsk_no_wakeup()` 返回真
（xsk.c:807），`recvfrom()` 内部先 `sk_busy_loop()` 直接驱动 NAPI，
然后**跳过** `xsk_wakeup()`——既收了包又没踢驱动。
</details>

<details>
<summary><b>Q2：</b>你把 <code>chunk_size</code> 设成 2048、<code>headroom</code> 设成 256，MTU 是 1500。上线后 <code>rx_dropped</code> 一直涨，<code>rx_ring_full</code> 却是 0。为什么？怎么办？</summary>

**因为可用包长只剩下 1536 字节。**

```c
/* net/xdp/xsk_buff_pool.c:84 */
pool->frame_len = umem->chunk_size - umem->headroom - XDP_PACKET_HEADROOM;
```

`XDP_PACKET_HEADROOM = 256`，所以：

```
frame_len = 2048 - 256(headroom) - 256(XDP_PACKET_HEADROOM) = 1536
```

而一个 1500 字节 MTU 的以太网帧实际是 **1514 字节**（+14 B 以太网头），
带 VLAN tag 是 **1518**，某些场景还有额外的封装。1536 只剩 18~22 字节余量，
**任何超过 1536 的包都会被丢弃**：

```c
/* xsk_rcv_check()，xsk.c:317 */
	if (len > xsk_pool_get_rx_frame_size(xs->pool) && !xs->sg) {
		xs->rx_dropped++;
		return -ENOSPC;
	}
```

注意它计入的是 **`rx_dropped`**，不是 `rx_ring_full`——
这正是"**包太大**"与"**用户读太慢**"的区分标志。

**三种解法：**

| 解法 | 做法 | 代价 |
|------|------|------|
| **① 加大 chunk（推荐）** | `chunk_size = 4096` → `frame_len = 3584` | UMEM 占用翻倍；每帧浪费更多（小包场景） |
| **② 去掉 headroom** | `headroom = 0` → `frame_len = 1792` | 无法用 `bpf_xdp_adjust_head()` 加/去封装头 |
| **③ 开 `XDP_USE_SG`** | bind 时加 flag，多描述符拼大包 | 用户态要处理 `XDP_PKT_CONTD`；驱动需 `xdp_zc_max_segs > 1`，否则 bind 返回 `-EOPNOTSUPP` |

**推荐 ①**：行情包通常几百字节，4096 的浪费相比"持续丢包"不值一提。
**UMEM 内存很便宜，`rx_dropped` 很贵。**

**顺带的诊断口诀**：

```
rx_ring_full              涨 → 消费太慢
rx_fill_ring_empty_descs  涨 → 归还 frame 太慢
rx_dropped 涨而两者为 0    → 包比 frame_len 大（查 MTU vs frame_len）
```
</details>

<details>
<summary><b>Q3：</b>零拷贝模式下为什么用户态"一下子看到一批包"而不是一个一个？这对单包延迟意味着什么？</summary>

**因为 RX ring 的 producer 索引只在 `xsk_flush()` 时发布，而 flush 发生在 NAPI poll 结束时。**

链路是这样的：

```c
/* 每包：只填 desc，不发布 producer（xsk.c:368） */
int __xsk_map_redirect(struct xdp_sock *xs, struct xdp_buff *xdp)
{
	err = xsk_rcv(xs, xdp);          /* → __xsk_rcv_zc()：取 addr、填 desc */
	if (err)
		return err;
	if (!xs->flush_node.prev)
		list_add(&xs->flush_node, flush_list);   /* 挂 per-CPU 链表 */
	return 0;
}

/* NAPI poll 结束：驱动调 xdp_do_flush() → __xsk_map_flush()（xsk.c:382） */
void __xsk_map_flush(void)
{
	list_for_each_entry_safe(xs, tmp, flush_list, flush_node) {
		xsk_flush(xs);
		...
	}
}

/* xsk.c:326 */
static void xsk_flush(struct xdp_sock *xs)
{
	xskq_prod_submit(xs->rx);        /* ⭐ 此刻才 publish producer */
	__xskq_cons_release(xs->pool->fq);
	sock_def_readable(&xs->sk);
}
```

**所以一个 NAPI poll 批次（默认 weight = 64）里被 redirect 的所有包，
会在 poll 结束的同一瞬间对用户态可见。**

**对延迟的三点含义：**

1. **批内位置决定单包延迟**。第 1 个包和第 64 个包虽然同时被 publish，
   但第 64 个包在网卡里躺了更久。如果你只关心"第一个包到达用户态的时间"，
   小批量更好；如果关心 P99，要控制批大小。

2. **低包速时延迟反而更高**。100 pps 的场景下，一个包进来了，
   但 NAPI poll 还没结束（没填满 weight，也没退出），
   producer 就没发布——**你在等一个永远不会填满的批次结束**。
   驱动通常靠"poll 返回前没更多包了"来结束，所以这里是 poll 的单轮开销，
   但仍然是额外的等待。

3. **`sock_def_readable()` 也在 flush 里**，所以阻塞式 `poll()` 的唤醒粒度
   是"每个 NAPI poll 一次"，不是每包。**不要用 `poll()` 的返回频率去推测包速。**

**怎么调：**

| 手段 | 效果 |
|------|------|
| 减小用户态批处理大小（16/32） | 降低批内等待，但 ring 操作开销占比上升 |
| 减小 NAPI weight（`net.core.dev_weight`，默认 64） | 批次更小 → flush 更频繁 → 延迟降低，吞吐略降 |
| 开 busy poll | 消除等待 NAPI 被调度的延迟（第 ② 项） |
| 接受它 | 如果目标是 P50 而非 P99，批处理的吞吐收益更大 |

**HFT 建议：从 batch = 32 起测**（内核 `TX_BATCH_SIZE` 也是 32，xsk.c:35），
按 P99 延迟和吞吐两条曲线找拐点，不要照抄别人的数字。
</details>

---

→ 本篇：[02 AF_XDP 工程实践](02-af-xdp-lwn.md)
→ 前一篇：[01 AF_XDP 接口精读](01-af-xdp.md)
→ 相关：[03 UMEM 布局与 copy/zc 差异](03-af-xdp-umem-layout.md) · [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) · [chapter-07 XDP redirect 与 DPDK](../../chapter-07-xdp-redirect-dpdk/)
