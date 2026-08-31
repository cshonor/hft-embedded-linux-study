# 01 — AF_XDP 接口精读：从 socket() 到收包的每一步约束

> **内核文档：** `Documentation/networking/af_xdp.rst`
> **对应 Rosen:** 无（AF_XDP 4.18+ 才存在，3.x 时代无此概念）
> **内核版本：** 以 **v6.6** 为准。所有常量、errno、判定条件均取自源码，行号标注如下：
> - `include/uapi/linux/if_xdp.h` — 用户态 ABI（socket 选项、`xdp_umem_reg`、`xdp_statistics`）
> - `net/xdp/xsk.c` — socket 层（`xsk_bind` 1082、`xsk_rcv` 348、`xsk_rcv_check` 309）
> - `net/xdp/xdp_umem.c` — UMEM 注册校验（`xdp_umem_reg` 151）
> - `net/xdp/xsk_buff_pool.c` — buffer pool（`xp_assign_dev` 149）
> - `include/linux/filter.h` — `__bpf_xdp_redirect_map` 1498（redirect flags 语义）

---

## 文档概述

官方 `af_xdp.rst` 讲的是"怎么用"，本篇讲的是"**为什么必须这么用**"——
把每一步背后的内核校验条件挖出来，因为 AF_XDP 的失败模式有一个共同特征：
**大部分错误不是崩，而是静默降级。** 你以为在跑零拷贝，其实在走 copy 模式；
你以为没匹配到 socket 的包会走协议栈，其实被 `XDP_ABORTED` 全丢了。

### 本篇与同章其他笔记的分工

| 笔记 | 回答什么 | 关键词 |
|------|---------|--------|
| **01（本篇）** | 接口的**每一步约束**与失败模式 | UMEM 注册校验、bind 判定、redirect flags、统计量语义 |
| [02 工程实践](02-af-xdp-lwn.md) | 值不值得用、延迟预算、wakeup、批处理、选型 | 决策、调优、排障清单 |
| [03 UMEM 布局](03-af-xdp-umem-layout.md) | UMEM 内存怎么排布、copy 比 zc 多付什么 | frame/chunk/headroom、所有权交接 |

---

## 一、AF_XDP 在整栈里的位置

```
                         ┌──────────── 内核态 ────────────┐
NIC ──DMA──→ 驱动 Rx ring ──→ NAPI poll ──→ XDP hook
                                              │
                                              │ bpf_redirect_map(&xsks_map, queue_id, XDP_PASS)
                                              ↓
                                   __xsk_map_redirect()      xsk.c:368
                                              ↓
                                     xsk_rcv()                xsk.c:348
                                    ┌─────────┴─────────┐
                    mem.type == MEM_TYPE_XSK_BUFF_POOL ? │
                                    │                   │
                             是 → xsk_rcv_zc()      否 → __xsk_rcv()（copy）
                            （零拷贝，填 desc）      （分配 UMEM frame + memcpy）
                                    │                   │
                                    └─────────┬─────────┘
                                              ↓
                                   RX ring（producer++）
                                              ↓  xsk_flush()  xsk.c:326
                                   ┌──────────┴──────────┐
                                   ↓                     ↓
                              用户态看到包            FILL ring 释放
                                                              └──────────── 用户态 ────────────┘
```

关键点：**AF_XDP 不是"绕过 XDP"，而是"XDP 的一个 destination"**。
没有 XDP 程序（或 `libxdp`/`xdp-loader` 自带的 `xdpsock` 程序）把包 redirect 进 `xsks_map`，
socket 就永远不会收到一个包——**它自己不会"抓"包**。

---

## 二、socket 生命周期：八步，每步一个坑

```
①  socket(AF_XDP, SOCK_RAW, 0)
②  分配 UMEM          ← 必须页对齐，malloc 不行
③  setsockopt(XDP_UMEM_REG)          ← 四条硬校验
④  setsockopt(XDP_RX_RING / XDP_TX_RING)
⑤  setsockopt(XDP_UMEM_FILL_RING / XDP_UMEM_COMPLETION_RING)
⑥  mmap() 四个 ring                  ← 偏移来自 getsockopt(XDP_MMAP_OFFSETS)
⑦  bind(sockfd, sockaddr_xdp)        ← zc/copy 在此判决
⑧  预填 FILL ring → 加载 XDP 程序 → 轮询 RX ring
```

顺序不是建议，是**强制**：

| 步骤 | 顺序约束 | 违反的后果 |
|------|---------|-----------|
| ③ 之前要 ② | UMEM 必须先分配 | — |
| ④ ⑤ 必须在 ⑦ 之前 | `xsk_bind` 检查 `xs->state != XSK_READY` → **`-EBUSY`**（xsk.c:1108） | bind 直接失败 |
| ③ 只能做一次 | `xsk_setsockopt` 检查 `xs->state != XSK_READY \|\| xs->umem` → **`-EINVAL`**（xsk.c:1308） | 不能改 UMEM |
| ④ ⑤ 只能做一次 | 同上（xsk.c:1281 / 1335） | 不能改 ring 大小 |
| ⑦ 之前必须 ③ | `!xs->umem \|\| !xsk_validate_queues(xs)` → **`-EINVAL`**（xsk.c:1214） | bind 失败 |

> **最常见的初学错误**：先 `bind()` 再 `setsockopt()`。
> 得到的不是"参数错误"，而是 `-EBUSY`，错误信息完全指向不了真实原因。

### 状态机

```
     XSK_UNBOUND ──(setsockopt 配好 rx/tx + fq/cq)──→ XSK_READY
                                                          │
                                                      bind()
                                                          ↓
                                                      XSK_BOUND
```

只有 `XSK_BOUND` 状态的 socket 才会被 XDP redirect 命中——
`xsk_rcv_check()` 第一件事就是 `if (!xsk_is_bound(xs)) return -ENXIO;`（xsk.c:311）。
**所以"我 bind 了但收不到包"要查的第一件事是：bind 到底返回了什么。**

---

## 三、UMEM 注册：内核的四条硬校验与三笔账

### 3.1 `xdp_umem_reg` 的四条硬校验（`xdp_umem_reg()`，xdp_umem.c:151）

```c
/* include/uapi/linux/if_xdp.h:73 */
struct xdp_umem_reg {
	__u64 addr;        /* 用户态内存起始地址 */
	__u64 len;         /* 总字节数 */
	__u32 chunk_size;  /* 每个 chunk 的长度 */
	__u32 headroom;    /* 每个 chunk 前置保留 */
	__u32 flags;       /* XDP_UMEM_UNALIGNED_CHUNK_FLAG */
};
```

| # | 校验（源码位置） | 条件 | errno | 说明 |
|---|-----------------|------|-------|------|
| 1 | xdp_umem.c:160 | `chunk_size < 2048 \|\| chunk_size > PAGE_SIZE` | `-EINVAL` | **下界 2048**（`XDP_UMEM_MIN_CHUNK_SHIFT = 11`，`xdp_sock_drv.h:12`），**上界一页** |
| 2 | xdp_umem.c:173 | 非 unaligned 模式下 `chunk_size` 不是 2 的幂 | `-EINVAL` | 所以实际只有 **2048 / 4096** 两个选择（4K 页系统） |
| 3 | xdp_umem.c:176 | `!PAGE_ALIGNED(addr)` | `-EINVAL` | **必须页对齐 → `malloc()` 直接失败**，用 `mmap` / `posix_memalign` / hugepage |
| 4 | xdp_umem.c:196/199 | `len % chunk_size != 0`（非 unaligned）<br>`headroom >= chunk_size - 256` | `-EINVAL` | UMEM 必须被 chunk 整除；headroom 不能把空间吃光 |

**⚠️ 第 1 条最反直觉：`chunk_size` 的上限是一页（x86_64 上 4096）。**
想用 AF_XDP 收 9000 字节的 jumbo frame？**单个 chunk 装不下，做不到。**
唯一出路是 `XDP_USE_SG`（multi-buffer，5.19+），把一个包切成多个描述符
（`XDP_PKT_CONTD` 标志）。这也是 MTU 超过 `frame_len` 的包的归宿——
见第四节。

### 3.2 内核为 UMEM 付的三笔账

`xdp_umem_reg()` 除了校验，还做了三件事（xdp_umem.c:214-224）：

```c
err = xdp_umem_account_pages(umem);   /* ① RLIMIT_MEMLOCK 记账 */
if (err) return err;
err = xdp_umem_pin_pages(umem, (unsigned long)addr);  /* ② 钉页 */
if (err) goto out_account;
err = xdp_umem_addr_map(umem, umem->pgs, umem->npgs); /* ③ vmap */
```

| 步骤 | 源码 | 后果 |
|------|------|------|
| ① 记账 | `capable(CAP_IPC_LOCK)` 才跳过；否则 `rlimit(RLIMIT_MEMLOCK)` 超限 → **`-ENOBUFS`**（xdp_umem.c:144） | **`ulimit -l` 不够，注册直接失败。** 这是容器/CI 环境最常见的启动失败原因 |
| ② 钉页 | `pin_user_pages(..., FOLL_WRITE \| FOLL_LONGTERM, ...)`（xdp_umem.c:105） | UMEM **常驻内存，永不换出**；`FOLL_LONGTERM` 对某些特殊内存（如某些 GUP-longterm 不支持的页）会失败 |
| ③ 映射 | `vmap(pages, nr_pages, VM_MAP, PAGE_KERNEL)`（xdp_umem.c:49） | 内核建立自己的虚拟映射，用于 copy 模式访问 |

**HFT 推论：** UMEM 一旦注册就**常驻物理内存**，`top` 里看到的进程 RSS 是实打实的。
一个 2048 chunk × 65536 = 128 MB 的 UMEM 会永久占用 128 MB，且计入 memlock 限额。

### 3.3 `frame_len`：你的包最大能有多大

```c
/* net/xdp/xsk_buff_pool.c:84 */
pool->frame_len = umem->chunk_size - umem->headroom - XDP_PACKET_HEADROOM;
```
（`XDP_PACKET_HEADROOM = 256`，`include/uapi/linux/bpf.h:6276`）

| chunk_size | headroom | 可用包长 `frame_len` | 能装下 1500B 吗 | 能装下 9000B jumbo 吗 |
|-----------|----------|--------------------|----------------|---------------------|
| 2048 | 0 | **1792** | ✅ | ❌ |
| 2048 | 256 | **1536** | ⚠️ 仅剩 36 B 余量（含以太网头 14B） | ❌ |
| 4096 | 0 | **3840** | ✅ | ❌ |
| 4096 | 256 | **3584** | ✅ | ❌ |

**结论：AF_XDP 单描述符路径最大包长是 3584（4K chunk）。**
要收 jumbo 只能开 `XDP_USE_SG`，且驱动要支持（`xdp_zc_max_segs > 1`），
否则 `xp_assign_dev` 返回 `-EOPNOTSUPP`（xsk_buff_pool.c:184）。

---

## 四、bind()：zc / copy 的判决点（本篇最重要的一节）

### 4.1 源码：`xp_assign_dev()`，xsk_buff_pool.c:149

```c
	force_zc = flags & XDP_ZEROCOPY;
	force_copy = flags & XDP_COPY;

	if (force_zc && force_copy)
		return -EINVAL;

	if (xsk_get_pool_from_qid(netdev, queue_id))
		return -EBUSY;          /* 这个 (dev, queue) 已经被别的 xsk 占了 */
	...
	if (force_copy)
		/* For copy-mode, we are done. */
		return 0;

	if ((netdev->xdp_features & NETDEV_XDP_ACT_ZC) != NETDEV_XDP_ACT_ZC) {
		err = -EOPNOTSUPP;
		goto err_unreg_pool;
	}
	...
	err = netdev->netdev_ops->ndo_bpf(netdev, &bpf);   /* XDP_SETUP_XSK_POOL */
	if (err)
		goto err_unreg_pool;

	if (!pool->dma_pages) {
		WARN(1, "Driver did not DMA map zero-copy buffers");
		err = -EINVAL;
		goto err_unreg_xsk;
	}
	pool->umem->zc = true;
	return 0;

err_unreg_xsk:
	xp_disable_drv_zc(pool);
err_unreg_pool:
	if (!force_zc)
		err = 0;             /* ⭐ fallback to copy mode */
	if (err) {
		xsk_clear_pool_at_qid(netdev, queue_id);
		dev_put(netdev);
	}
	return err;
```

### 4.2 判决表

| `XDP_ZEROCOPY` | `XDP_COPY` | 驱动支持 ZC | bind() 返回 | 实际模式 |
|---------------|-----------|------------|------------|---------|
| ❌ | ❌ | ✅ | 0 | **zero-copy** ✅ |
| ❌ | ❌ | ❌ | **0（成功！）** | **copy（静默降级）** ⚠️ |
| ✅ | ❌ | ✅ | 0 | zero-copy ✅ |
| ✅ | ❌ | ❌ | **`-EOPNOTSUPP`** | 失败 |
| ❌ | ✅ | 任意 | 0 | copy ✅ |
| ✅ | ✅ | 任意 | **`-EINVAL`** | 失败 |

> ### ⭐ 一句话结论
> **不显式指定 `XDP_ZEROCOPY`，驱动不支持时 bind() 依然返回 0，只是静默变成 copy 模式。**
> 这是 AF_XDP 头号陷阱：[02 篇](02-af-xdp-lwn.md)会说明 copy 模式比零拷贝慢 2–3 倍，
> 而你在 `ss`/`bpftool` 里看到的一切都是"正常运行"。
>
> **唯一可靠的验证方式**（bind 之后）：
> ```c
> struct xdp_options opts;
> socklen_t len = sizeof(opts);
> getsockopt(xsk_fd, SOL_XDP, XDP_OPTIONS, &opts, &len);
> if (!(opts.flags & XDP_OPTIONS_ZEROCOPY))
>         fprintf(stderr, "⚠️ 降级到 copy 模式，别上生产\n");
> ```

### 4.3 bind() 的其他 errno

| errno | 条件（源码） | 排查方向 |
|-------|------------|---------|
| `-EBUSY` | `xsk_get_pool_from_qid()` 已存在（xsk_buff_pool.c:164） | 该 (dev, queue_id) 已被另一个 xsk 占用——**一个队列只能有一个 xsk** |
| `-EINVAL` | `XDP_COPY` 与 `XDP_ZEROCOPY` 同时给 | 二选一 |
| `-EINVAL` | flag 含未定义位（xsk.c:1098） | 只允许 `XDP_SHARED_UMEM/COPY/ZEROCOPY/USE_NEED_WAKEUP/USE_SG` |
| `-EINVAL` | `XDP_SHARED_UMEM` 与上述任一 flag 同时给（xsk.c:1130） | **共享 UMEM 的 socket 不许单独指定模式**，继承首个 socket 的 |
| `-EOPNOTSUPP` | 驱动无 `NETDEV_XDP_ACT_ZC`，或 `xdp_zc_max_segs == 1` 却要 `XDP_USE_SG` | 换驱动/换网卡；查 `ethtool -i` + 驱动源码 |
| `-EINVAL` | 驱动没 DMA 映射（`WARN` 打内核日志） | 驱动 bug，`dmesg` 里能看到那句 WARN |
| `-ENODEV` | ifindex 不存在 | — |

---

## 五、XDP 程序侧：`xsks_map` 的 flags 陷阱

AF_XDP 必须配一个 XDP 程序把包 redirect 进 socket。典型写法：

```c
SEC("xdp")
int xdp_sock_prog(struct xdp_md *ctx)
{
    int index = ctx->rx_queue_index;
    /* 只把感兴趣的 UDP 行情包送进用户态，其余走内核栈 */
    if (!is_market_data(ctx))
        return XDP_PASS;
    return bpf_redirect_map(&xsks_map, index, XDP_PASS);
}
```

### 5.1 flags 的低位是"lookup 失败时的返回值"

```c
/* include/linux/filter.h:1498 */
static __always_inline long __bpf_xdp_redirect_map(struct bpf_map *map, u64 index,
						   u64 flags, const u64 flag_mask,
						   void *lookup_elem(struct bpf_map *map, u32 key))
{
	const u64 action_mask = XDP_ABORTED | XDP_DROP | XDP_PASS | XDP_TX;

	/* Lower bits of the flags are used as return code on lookup failure */
	if (unlikely(flags & ~(action_mask | flag_mask)))
		return XDP_ABORTED;

	ri->tgt_value = lookup_elem(map, index);
	if (unlikely(!ri->tgt_value) && !(flags & BPF_F_BROADCAST)) {
		...
		return flags & action_mask;      /* ⭐ 查不到就返回这个 */
	}
	...
}
```

| 你写的 flags | `xsks_map[queue_id]` 查不到时 | 后果 |
|-------------|---------------------------|------|
| **`0`** | 返回 `XDP_ABORTED`（=0） | ⚠️ **包被丢弃 + 触发 tracepoint**。你没挂 socket 的队列、ARP、ICMP、SSH 全丢 |
| `XDP_PASS` | 返回 `XDP_PASS` | ✅ 交给内核协议栈（推荐默认） |
| `XDP_DROP` | 返回 `XDP_DROP` | 丢弃，但**不打 tracepoint**（比 ABORTED 干净） |
| `XDP_TX` | 返回 `XDP_TX` | 原路反弹回去 |
| 含非法位（如 `XDP_REDIRECT`） | 立刻返回 `XDP_ABORTED` | **连查都不查，所有包全丢** |

### 5.2 两种写法的差别

```c
/* ❌ 致命：查不到 → XDP_ABORTED → 静默丢包 */
return bpf_redirect_map(&xsks_map, idx, 0);

/* ✅ 正确：查不到 → 交给内核协议栈，其他队列/其他协议照常工作 */
return bpf_redirect_map(&xsks_map, idx, XDP_PASS);
```

**为什么这条对 HFT 特别危险：**
`XDP_ABORTED` 的包**不会出现在 tcpdump 里**（tcpdump 的 `AF_PACKET` 挂在
`ptype_all`，`dev.c:5394`，在 XDP 之后），也不会计入任何 per-queue 的 RX 计数。
你只会看到"网络莫名其妙不通"，而所有常规工具都说一切正常。

```bash
# 唯一能看到它的地方
bpftool prog show id <ID>     # 看不到 ABORTED
cat /sys/kernel/debug/tracing/events/xdp/xdp_redirect_err/format   # 需要开 tracepoint
perf trace -e xdp:*           # 或
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'
```

---

## 六、收包路径：`xsk_rcv()` 逐包判定 zc / copy

bind() 时的 flag 决定"**能不能**"用零拷贝，而**实际走哪条路由每个包自己决定**：

```c
/* net/xdp/xsk.c:348 */
static int xsk_rcv(struct xdp_sock *xs, struct xdp_buff *xdp)
{
	u32 len = xdp_get_buff_len(xdp);
	int err;

	err = xsk_rcv_check(xs, xdp, len);
	if (err)
		return err;

	if (xdp->rxq->mem.type == MEM_TYPE_XSK_BUFF_POOL) {
		len = xdp->data_end - xdp->data;
		return xsk_rcv_zc(xs, xdp, len);      /* 零拷贝 */
	}

	err = __xsk_rcv(xs, xdp, len);            /* copy 模式 */
	if (!err)
		xdp_return_buff(xdp);                 /* 把驱动的 page 还回去 */
	return err;
}
```

> **判定依据是 `xdp->rxq->mem.type`——包来自哪种内存池。**
> 来自 XSK buff pool（驱动为 zc 准备的 UMEM）→ 零拷贝；
> 来自驱动的普通 page_pool → copy 模式（拷一份进 UMEM，再把原页还给 page_pool）。
>
> 这条源码也终结了一个常见误解：**AF_XDP 零拷贝不使用 page_pool。**
> UMEM 由内核自己 DMA 映射（`xp_dma_map()`，pool 自带的 `dma_pages` 数组，
> `include/net/xsk_buff_pool.h:68`）。"AF_XDP 基于 page_pool"只在 copy 模式下成立。
> 详见 [chapter-04/01](../../chapter-04-page-pool/notes/01-page-pool.md)。

### 6.1 三道检查（`xsk_rcv_check()`，xsk.c:309）

| 检查 | 失败返回 | 后果 |
|------|---------|------|
| `!xsk_is_bound(xs)` | `-ENXIO` | socket 未 bind，包继续走 XDP 的后续处理 |
| `xs->dev != xdp->rxq->dev \|\| xs->queue_id != xdp->rxq->queue_index` | `-EINVAL` | **设备/队列不匹配**。`xsks_map` 里塞了错的 socket 就是这个错 |
| `len > xsk_pool_get_rx_frame_size(pool) && !xs->sg` | `-ENOSPC` + **`xs->rx_dropped++`** | 包比 `frame_len` 大且没开 `XDP_USE_SG` → **丢弃** |

⚠️ 第三条是"包太大"的唯一出口，**它计入的是 `rx_dropped`，不是 `rx_ring_full`**。
如果你的 MTU 设成了 9000 而 chunk 是 2048，你会看到 `rx_dropped` 疯涨而
`rx_ring_full` 为 0——这正好是"包太大"和"用户读太慢"的区分方法。

### 6.2 copy 模式的三个失败点（`__xsk_rcv()`，xsk.c:223）

```c
	if (len <= frame_size && !xdp_buff_has_frags(xdp)) {
		xsk_xdp = xsk_buff_alloc(xs->pool);
		if (!xsk_xdp) {
			xs->rx_dropped++;          /* ① UMEM 里没空闲 frame */
			return -ENOMEM;
		}
		...
	}

	num_desc = (len - 1) / frame_size + 1;
	if (!xsk_buff_can_alloc(xs->pool, num_desc)) {
		xs->rx_dropped++;              /* ② 同上，多描述符场景 */
		return -ENOMEM;
	}
	if (xskq_prod_nb_free(xs->rx, num_desc) < num_desc) {
		xs->rx_queue_full++;           /* ③ RX ring 满 → 用户态读太慢 */
		return -ENOBUFS;
	}
```

- **① ② `rx_dropped`** = UMEM 侧没资源了 → **FILL ring 空**（你归还 frame 太慢）
- **③ `rx_queue_full`** = RX ring 侧没位置了 → **用户态消费太慢**

两者症状都是"丢包"，但解法完全相反：前者要加大 FILL ring 水位，后者要加快消费循环。

### 6.3 零拷贝路径极简（`__xsk_rcv_zc()`，xsk.c:139）

```c
static int __xsk_rcv_zc(struct xdp_sock *xs, struct xdp_buff_xsk *xskb, u32 len, u32 flags)
{
	u64 addr;
	int err;

	addr = xp_get_handle(xskb);
	err = xskq_prod_reserve_desc(xs->rx, addr, len, flags);
	if (err) {
		xs->rx_queue_full++;
		return err;
	}
	xp_release(xskb);
	return 0;
}
```

**只有两步：取地址 → 填描述符。** 没有分配、没有拷贝。
零拷贝模式下 `rx_dropped` 基本不会涨（除了 `xsk_rcv_check` 的包太大分支），
**丢包几乎全在 `rx_ring_full` 和 `rx_fill_ring_empty_descs`**。

---

## 七、观测：`xdp_statistics` 六个计数器怎么用

```c
/* include/uapi/linux/if_xdp.h:81 */
struct xdp_statistics {
	__u64 rx_dropped;                  /* 其他原因丢弃 */
	__u64 rx_invalid_descs;            /* 描述符非法 */
	__u64 tx_invalid_descs;            /* 发送描述符非法 */
	__u64 rx_ring_full;                /* RX ring 满 */
	__u64 rx_fill_ring_empty_descs;    /* 从 FILL ring 取不到东西 */
	__u64 tx_ring_empty_descs;         /* 从 TX ring 取不到东西 */
};
```

### 7.1 诊断对照表

| 计数器 | 源码产生点 | 含义 | 处置 |
|--------|-----------|------|------|
| `rx_ring_full` | xsk.c:262 / 145 (`__xsk_rcv_zc`) | **RX ring 满 → 用户态消费太慢** | 加大 RX ring；加快循环；批处理收包 |
| `rx_dropped` | xsk.c:241 / 258（copy：无空闲 frame）<br>xsk.c:318（**包 > frame_len 且未开 SG**） | 两个完全不同的原因 | 先看 chunk/MTU：`rx_dropped` 涨而 `rx_ring_full` 为 0 → **包太大**；反之 → FILL ring 空 |
| `rx_fill_ring_empty_descs` | `xskq_nb_queue_empty_descs(pool->fq)`（xsk.c:1408） | **FILL ring 空 → 你归还 frame 太慢**（ZC 模式丢包主因） | 加大 FILL ring；提高归还频率；别在收包循环里做重活 |
| `rx_invalid_descs` | xsk_queue.h:199 | 用户态往 RX ring 侧提交了非法描述符 | 几乎必是用户态 bug（addr 越界、len=0） |
| `tx_invalid_descs` | 同上 / xsk.c:570 | 待发包的描述符非法 | 检查 TX 路径的 addr/len |
| `tx_ring_empty_descs` | xsk.c:488 | 驱动想发但 TX ring 空 | 正常（空闲时必然涨），不是错误 |

### 7.2 ⚠️ 一个隐蔽的坑：v1 结构体会把两个计数器折叠

```c
/* net/xdp/xsk.c:1389 */
	if (len < sizeof(struct xdp_statistics_v1)) {
		return -EINVAL;
	} else if (len < sizeof(stats)) {
		extra_stats = false;
		stats_size = sizeof(struct xdp_statistics_v1);
	} else {
		stats_size = sizeof(stats);
	}
	...
	stats.rx_dropped = xs->rx_dropped;
	if (extra_stats) {
		stats.rx_ring_full = xs->rx_queue_full;
		stats.rx_fill_ring_empty_descs = xskq_nb_queue_empty_descs(xs->pool->fq);
		stats.tx_ring_empty_descs = xskq_nb_queue_empty_descs(xs->tx);
	} else {
		stats.rx_dropped += xs->rx_queue_full;   /* ⭐ 折叠！ */
	}
```

如果你传的 `optlen` 只有旧的 `struct xdp_statistics_v1` 大小（3 个 `__u64`），
内核**只填前三个字段，并把 `rx_queue_full` 加进 `rx_dropped`**——
于是"用户读太慢"和"包太大"彻底混在一起，无法诊断。

> **务必用完整的 `struct xdp_statistics`（6 个字段），并让 `optlen` 等于它的大小。**

### 7.3 观测命令

```bash
# xdp-tools 自带的统计（最方便）
xdp-stat -i eth0            # 或旧名 xdp_stats

# 自己取（C 代码）
struct xdp_statistics stats;
socklen_t optlen = sizeof(stats);        # ⚠️ 必须是完整结构体的大小
getsockopt(fd, SOL_XDP, XDP_STATISTICS, &stats, &optlen);

# 驱动层：确认包确实进了网卡（这是"包根本没到"与"包被 XDP 丢了"的分水岭）
ethtool -S eth0 | grep -E 'rx_packets|rx_dropped|xdp'

# 确认 XDP 程序在跑、在哪个模式
bpftool net show dev eth0
bpftool prog show id <ID>

# 确认这个 socket 是真零拷贝（不是静默降级的 copy）
#   → 见 4.2 的 getsockopt(XDP_OPTIONS)
```

---

## HFT 要点

- **第一件事永远是验证 `XDP_OPTIONS_ZEROCOPY`**。不显式要求 `XDP_ZEROCOPY` 就会静默降级，
  而降级后的性能（见 [02 篇](02-af-xdp-lwn.md)）不值得这套开发成本。
- **`bpf_redirect_map(&xsks_map, idx, 0)` 是自杀式写法**。低位 flags 是 lookup 失败时的返回值，
  0 = `XDP_ABORTED` = 静默丢光非目标流量。默认写 `XDP_PASS`。
- **`chunk_size` 只能 2048 或 4096**（4K 页系统），可用包长上限 3584。
  HFT 行情包通常 100–1500 B，2048 就够，还能省一半 TLB 压力和 cache footprint。
- **UMEM 会 `pin_user_pages(FOLL_LONGTERM)` 常驻**，并且吃 `RLIMIT_MEMLOCK`。
  容器里起不来先查 `ulimit -l`，errno 是 `-ENOBUFS` 而不是 `-ENOMEM`。
- **一个 (dev, queue_id) 只能有一个 xsk**（`-EBUSY`）。多策略进程要靠 `SO_REUSEPORT` 不行，
  得靠多队列 + `XDP_SHARED_UMEM` 或用户态分发。
- **丢包诊断先看 `rx_dropped` 与 `rx_ring_full` 的比值**：
  `rx_dropped` 独涨 = 包太大或 UMEM 无空闲 frame；`rx_ring_full` 独涨 = 消费太慢。
- **`headroom` 是从 `frame_len` 里扣的**。2048 chunk + 256 headroom 只剩 1536 B，
  刚好卡在标准 MTU 边缘——行情网关若带 VLAN tag 或需要 `bpf_xdp_adjust_head()` 加封装头，
  请直接用 4096。

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（4.18+ / v6.6） |
|------|-------------|---------------------|
| 用户态收包 | `socket()` + `recvmsg()`，必经协议栈 + 一次拷贝 | AF_XDP：XDP_REDIRECT 直送 UMEM，零拷贝 |
| Rx 缓冲区归属 | 内核 `alloc_page`，用完释放 | 用户态 UMEM，内核 DMA 映射到它 |
| 缓冲区归还 | 内核自动 | **用户态必须显式填 FILL ring**，不填就丢包 |
| 旁路粒度 | 无（要么走栈，要么 PF_PACKET 拷贝一份） | **按队列旁路**，一张网卡可以 0 号队列给 AF_XDP、其余走内核栈 |
| 丢包可见性 | `netstat -s`、网卡计数 | 每个 socket 独立的 `xdp_statistics` 六计数器 |
| 驱动契约 | `ndo_start_xmit` | 增加 `ndo_bpf(XDP_SETUP_XSK_POOL)` + `xdp_features` 声明 |
| 内存约束 | 只受进程地址空间限制 | **额外受 `RLIMIT_MEMLOCK` 与页对齐约束** |

---

## 代码自测

<details>
<summary><b>Q1：</b>你调用 <code>bind()</code> 返回 0，一切正常。但收包延迟是预期的 3 倍。最可能的原因是什么？怎么一次验证？</summary>

**最可能：静默降级到 copy 模式。**

`xp_assign_dev()`（xsk_buff_pool.c:149）在驱动不支持 `NETDEV_XDP_ACT_ZC` 时走
`err_unreg_pool:` 标签，然后：

```c
	if (!force_zc)
		err = 0; /* fallback to copy mode */
```

只要你在 `sockaddr_xdp.sxdp_flags` 里**没写** `XDP_ZEROCOPY`，`force_zc` 为假，
`bind()` 返回 0，socket 正常工作——只是每个包都要分配 UMEM frame 并 memcpy 一次。

**一次性验证（bind 之后立刻做）：**

```c
#include <linux/if_xdp.h>

struct xdp_options opts = {};
socklen_t len = sizeof(opts);
if (getsockopt(xfd, SOL_XDP, XDP_OPTIONS, &opts, &len) == 0) {
        printf("zero-copy: %s\n",
               (opts.flags & XDP_OPTIONS_ZEROCOPY) ? "YES" : "NO  ← 降级了");
}
```

**根治：bind 时就加 flag。**

```c
struct sockaddr_xdp sxdp = {
        .sxdp_family   = AF_XDP,
        .sxdp_ifindex  = ifindex,
        .sxdp_queue_id = qid,
        .sxdp_flags    = XDP_ZEROCOPY | XDP_USE_NEED_WAKEUP,
};
if (bind(xfd, (struct sockaddr *)&sxdp, sizeof(sxdp)) < 0) {
        /* 现在不支持会明确报 -EOPNOTSUPP，而不是假装成功 */
        perror("bind");
        return -1;
}
```

**为什么不一开始就用 `XDP_ZEROCOPY`？**
因为很多人想要"有 zc 就用，没有就退化"。但 AF_XDP 的价值几乎全在 zc 上——
copy 模式比零拷贝慢 2–3 倍（见 [03 篇](03-af-xdp-umem-layout.md)），
还不如直接走内核栈 + busy poll。**宁可失败，不要降级。**
</details>

<details>
<summary><b>Q2：</b>生产上把 XDP 程序挂上去之后，SSH 断了、ARP 不通，但 <code>tcpdump</code> 什么异常包都看不到，<code>ethtool -S</code> 的 rx 计数照常涨。为什么？</summary>

**`bpf_redirect_map()` 的第三个参数写成了 0。**

`__bpf_xdp_redirect_map()`（include/linux/filter.h:1498）的语义是：
**flags 的低位是"lookup 失败时的返回值"**。

```c
	const u64 action_mask = XDP_ABORTED | XDP_DROP | XDP_PASS | XDP_TX;
	...
	ri->tgt_value = lookup_elem(map, index);
	if (unlikely(!ri->tgt_value) && !(flags & BPF_F_BROADCAST)) {
		return flags & action_mask;
	}
```

`XDP_ABORTED` 的定义是 **0**（`include/uapi/linux/bpf.h`），所以 `flags = 0`
时，任何**没有对应 AF_XDP socket 的队列 / 任何非行情协议**的包都返回 `XDP_ABORTED`
→ 丢包 + 触发 `xdp_redirect_err` tracepoint。

**为什么 tcpdump 看不到？**
因为 AF_PACKET（tcpdump 的抓包点）挂在 `ptype_all`，位于 `dev.c:5394`，
在 **XDP 之后**。XDP 丢掉的包根本走不到 tcpdump。
（这个顺序见 [chapter-01](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md)）

**为什么 ethtool 计数照常涨？**
驱动 `rx_packets` 统计的是"DMA 完成、进了驱动的包"，XDP 在其之后执行，
XDP 丢包不会回退驱动计数。

**怎么确认？** 只有 tracepoint 看得到：

```bash
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { printf("err=%d\n", args->err); }'
# 或
perf trace -e xdp:xdp_redirect_err
```

**修复：**

```c
/* 行情包进 socket，其余全部交回内核协议栈 */
return bpf_redirect_map(&xsks_map, ctx->rx_queue_index, XDP_PASS);
```

如果确实想"非目标流量直接丢"，用 `XDP_DROP` 而不是 0——语义相同但不打 tracepoint，
不会把 tracepoint 通道刷爆。
</details>

<details>
<summary><b>Q3：</b>零拷贝模式下 <code>rx_dropped</code> 几乎为 0，但 <code>rx_ring_full</code> 在涨。而另一个环境里 <code>rx_dropped</code> 疯涨、<code>rx_ring_full</code> 为 0。两者原因分别是什么？</summary>

**它们指向完全不同的瓶颈。**

| 现象 | 计数器来源 | 含义 | 解法 |
|------|-----------|------|------|
| `rx_ring_full` 涨 | xsk.c:262（`__xsk_rcv`）<br>xsk.c:145（`__xsk_rcv_zc`） | **RX ring 没有空位** → 用户态消费循环跟不上 | 加快消费循环 / 批处理 / 加大 RX ring / 把重活移出循环 |
| `rx_dropped` 涨、`rx_ring_full` 为 0 | xsk.c:318（`xsk_rcv_check`）<br>xsk.c:241/258（copy 模式无空闲 frame） | 两种可能，见下 | 分别处理 |

`rx_dropped` 的两个来源要再拆：

```c
/* ① 包太大（xsk_rcv_check, xsk.c:317） */
	if (len > xsk_pool_get_rx_frame_size(xs->pool) && !xs->sg) {
		xs->rx_dropped++;
		return -ENOSPC;
	}

/* ② copy 模式下 UMEM 无空闲 frame（xsk.c:240 / 257） */
	xsk_xdp = xsk_buff_alloc(xs->pool);
	if (!xsk_xdp) {
		xs->rx_dropped++;
		return -ENOMEM;
	}
```

**区分 ① 和 ② 的办法：**

- **零拷贝模式下 ② 基本不会发生**（不需要临时分配 UMEM frame，包本来就在 UMEM 里），
  所以 **zc 模式下 `rx_dropped` 涨 ≈ 包太大**。
  检查 `MTU` vs `frame_len = chunk_size - 256 - headroom`（4K chunk、headroom 0 → 3584）。
- **copy 模式下 ① ② 都可能**。此时看 `rx_fill_ring_empty_descs`：
  它涨 → 是 ②（FILL ring 空，你归还 frame 太慢）。

**一句话诊断口诀：**

```
rx_ring_full              涨 → 收包循环慢
rx_fill_ring_empty_descs  涨 → 归还 frame 慢（ZC 模式头号丢包源）
rx_dropped 涨而两者为 0    → 包比 frame_len 大（MTU/chunk 不匹配）
```

**⚠️ 前提：你传的 `optlen` 必须是 `sizeof(struct xdp_statistics)`。**
传小了内核会走 v1 分支，把 `rx_queue_full` 折进 `rx_dropped`（xsk.c:1412），
三个原因彻底混在一起，无从下手。
</details>

---

→ 本篇：[01 AF_XDP 接口精读](01-af-xdp.md)
→ 后一篇：[02 AF_XDP 工程实践](02-af-xdp-lwn.md)
→ 相关：[03 UMEM 布局与 copy/zc 差异](03-af-xdp-umem-layout.md) · [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) · [chapter-04 page_pool](../../chapter-04-page-pool/)
