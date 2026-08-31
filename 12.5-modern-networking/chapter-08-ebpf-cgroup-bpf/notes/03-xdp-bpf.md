# 03 — XDP 程序：能力边界、返回码与 native/generic 的真实代价

> **来源：** LWN XDP 系列 + **v6.6 源码核对**
> **对应 Rosen：** 无
> **内核版本：** 全部源码引用基于 v6.6

## 文档概述

本篇回答三个问题：

1. **XDP 程序能做什么、不能做什么**（能力边界来自「有没有 skb」，不是来自「还没实现」）。
2. **五个返回码各自意味着什么**，以及**为什么 `XDP_ABORTED == 0` 是最危险的默认值**。
3. **native 与 generic XDP 的真实差异**——不是「快慢两档」，而是**generic 会为每个包额外做一次头部扩展/线性化，且可能静默丢包**。

姊妹篇分工：

| 文件 | 本篇与它的关系 |
|------|---------------|
| [01-ebpf-net-bootlin.md](01-ebpf-net-bootlin.md) | 01 定位 XDP 在栈中的位置；本篇展开 XDP 程序本身 |
| [02-bpf.md](02-bpf.md) | 02 给出 verifier 的全部限流常量；本篇给出 XDP 特有的 verifier 约束（包边界 + helper 失效） |
| [04-cgroup-bpf.md](04-cgroup-bpf.md) | 另一类（cgroup）程序，位置与 XDP 完全不同 |
| [chapter-05-xdp-architecture](../../chapter-05-xdp-architecture/) | XDP 自身的架构（xdp_buff、ring、驱动接口）在那里展开 |

---

## 1. 上下文：`struct xdp_md` 只有 6 个字段

`include/uapi/linux/bpf.h:6294`：

```c
struct xdp_md {
	__u32 data;
	__u32 data_end;
	__u32 data_meta;
	/* Below access go through struct xdp_rxq_info */
	__u32 ingress_ifindex; /* rxq->dev->ifindex */
	__u32 rx_queue_index;  /* rxq->queue_index  */

	__u32 egress_ifindex;  /* txq->dev->ifindex */
};
```

对比 tc-BPF 的 `struct __sk_buff`（几十个可写字段：`mark`、`priority`、`queue_mapping`、`cb[5]`、`hash`、`tstamp`…），XDP 的上下文**极其贫瘠**，而且：

| 字段 | 读 | 写 | 说明 |
|------|----|----|------|
| `data` / `data_end` | ✅ | ❌（只能通过 helper 改） | 包数据的起止 |
| `data_meta` | ✅ | ❌（只能通过 `bpf_xdp_adjust_meta`） | metadata 区，需要驱动预留 |
| `ingress_ifindex` | ✅ | ❌ | 走 `rxq->dev->ifindex` |
| `rx_queue_index` | ✅ | ❌ | **做多队列分流的关键字段** |
| `egress_ifindex` | ✅ | ❌ | 供 devmap / cpumap 的**次级程序**使用 |

> **`rx_queue_index` 是 XDP 里最重要的字段之一。** 多队列网卡上，`bpf_redirect_map(&xsks_map, ctx->rx_queue_index, XDP_PASS)` 是标准写法——把每个队列的包送到对应的 AF_XDP socket。这个字段的来源就是 `struct xdp_rxq_info`（`xdp_md` 注释里写的 "Below access go through struct xdp_rxq_info"），也就是说**读它会走一次间接寻址**。

> **`egress_ifindex` 是为 devmap/cpumap 的两级程序模型准备的**：主程序在 ingress 网卡上执行，redirect 后目标路径上还可以挂一个次级程序，那个程序通过 `egress_ifindex` 知道自己现在在哪个出接口上。

---

## 2. 五个返回码：语义与代价

`enum xdp_action`（`include/uapi/linux/bpf.h:6283`）：

```c
enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};
```

| 返回码 | 值 | 内核做什么 | 后续开销 |
|--------|-----|-----------|---------|
| `XDP_ABORTED` | **0** | `trace_xdp_exception()` + 丢包 | 与 DROP 相同，**外加一次 tracepoint** |
| `XDP_DROP` | 1 | 直接释放页/回收帧 | 最小 |
| `XDP_PASS` | 2 | 交给内核继续处理（`build_skb` + GRO + 协议栈） | 全部协议栈开销 |
| `XDP_TX` | 3 | 从**同一个网卡、同一个队列**发回去 | 需要改 L2 头；不经过 qdisc（native） |
| `XDP_REDIRECT` | 4 | 目标写入 per-CPU `bpf_redirect_info`，在 `xdp_do_flush()` 时统一批量提交 | 见 [chapter-07](../../chapter-07-xdp-redirect-dpdk/notes/01-xdp-redirect.md) |

### 2.1 ⚠️ `XDP_ABORTED == 0`：最危险的默认值

这是 XDP 编程中最容易踩、最难排查的陷阱。出处在 generic 路径的处理逻辑（`net/core/dev.c:4975-4991`）能看得很清楚：

```c
	act = bpf_prog_run_generic_xdp(skb, xdp, xdp_prog);
	switch (act) {
	case XDP_REDIRECT:
	case XDP_TX:
	case XDP_PASS:
		break;
	default:
		bpf_warn_invalid_xdp_action(skb->dev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(skb->dev, xdp_prog, act);
		fallthrough;
	case XDP_DROP:
	do_drop:
		kfree_skb(skb);
		break;
	}
```

**三个后果：**

1. **任何「忘了 return」的 XDP 程序，返回值默认 0 = `XDP_ABORTED` = 丢包。** 编译器不会警告（BPF 的 R0 初值就是 0），加载不会失败。
2. **任何非法返回值都走 `default` → `XDP_ABORTED`。** 例如把 helper 的负 errno 直接当返回码返回（`return bpf_redirect_map(...)` 写错时很常见）。
3. **`XDP_ABORTED` 与 `XDP_DROP` 的区别只有一次 `trace_xdp_exception()`。** 功能上都是丢包。

**唯一的可见性**：`tracepoint:xdp:xdp_exception`。这个异常**既不被 tcpdump 看到**（AF_PACKET 在 XDP 之后），**也不计入 `ethtool -S` 的 rx 计数**（驱动统计在 XDP 判定之后）。

```bash
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'
```

### 2.2 `XDP_TX` 的两个细节

1. **它是「原路返回」**——从同一个 net_device、同一个队列发出去。所以做 L2 转发必须**自己改目的 MAC 和源 MAC**（`bpf_xdp_adjust_head` 或直接改 `ethhdr`）。
2. **native 模式下 `XDP_TX` 走驱动专属 TX 队列，绕过 qdisc。** 但 **generic 模式的 `XDP_TX`（`generic_xdp_tx()`，`dev.c:5000`）也绕过 qdisc 和网络 taps**，内核源码里的注释明确警告了这个副作用：

```c
/* When doing generic XDP we have to bypass the qdisc layer and the
 * network taps in order to match in-driver-XDP behavior. This also means
 * that XDP packets are able to starve other packets going through a qdisc,
 * and DDOS attacks will be more effective. In-driver-XDP use dedicated TX
 * queues, so they do not have this starvation issue.
 */
void generic_xdp_tx(struct sk_buff *skb, struct bpf_prog *xdp_prog)
```

**结论：generic XDP 下用 `XDP_TX` 可能饿死其他流量，且 DDOS 更有效。native 用专属 TX 队列，无此问题。**

---

## 3. 能力边界：能做什么，不能做什么

### 3.1 能做的（23 个 helper，见 [02](02-bpf.md) 第 4.1 节）

| 能力 | helper | 约束 |
|------|--------|------|
| 读写包内容 | 直接指针访问（`data`..`data_end`）+ `xdp_load_bytes` / `xdp_store_bytes` | **必须先做 `data_end` 边界检查** |
| 增删头部 | `xdp_adjust_head`（push/pop）、`xdp_adjust_tail`（增删尾部） | 见下方约束 |
| 读写 metadata | `xdp_adjust_meta` | 需要驱动预留 metadata 空间；**不是所有驱动都支持** |
| 查包长 | `xdp_get_buff_len` | — |
| 校验和差值 | `csum_diff` | **只算差值，不写回包头**——写回要自己动手 |
| FIB 查表 | `fib_lookup` | 只读查询，不做路由决策 |
| MTU 检查 | `check_mtu` | — |
| 重定向 | `redirect`（指定 ifindex）、`redirect_map`（DEVMAP/CPUMAP/XSKMAP） | `redirect_map` 的 flags 低位 = **未命中时的返回码** |
| 关联 socket | `sk_lookup_tcp` / `sk_lookup_udp` / `skc_lookup_tcp` / `sk_release` | 有性能代价（socket 查找） |
| syncookie | `tcp_{gen,check}_syncookie`、`tcp_raw_*` | DDoS 防护 |
| 输出 | `perf_event_output`、`get_smp_processor_id` | — |

### 3.2 `bpf_xdp_adjust_head()` 的三个硬约束

`net/core/filter.c:3874`：

```c
BPF_CALL_2(bpf_xdp_adjust_head, struct xdp_buff *, xdp, int, offset)
{
	void *xdp_frame_end = xdp->data_hard_start + sizeof(struct xdp_frame);
	unsigned long metalen = xdp_get_metalen(xdp);
	void *data_start = xdp_frame_end + metalen;
	void *data = xdp->data + offset;

	if (unlikely(data < data_start ||
		     data > xdp->data_end - ETH_HLEN))
		return -EINVAL;

	if (metalen)
		memmove(xdp->data_meta + offset,
			xdp->data_meta, metalen);
	xdp->data_meta += offset;
	xdp->data = data;

	return 0;
}
```

读出的三条：

1. **下界受 `sizeof(struct xdp_frame)` 保护。** `data` 不能前移到 `data_hard_start + sizeof(struct xdp_frame) + metalen` 之前。原因：`XDP_REDIRECT` 时包会被转成 `struct xdp_frame`（`xdp_convert_buff_to_frame()`），那块区域要留给这个结构体。**所以你最多能把 data 往前推 `XDP_PACKET_HEADROOM(256) - sizeof(struct xdp_frame)` 左右。**
2. **上界是 `data_end - ETH_HLEN`（14 字节）。** XDP **不允许把包缩到小于一个以太网头**。
3. **metadata 会跟着 data 一起移动**（`memmove`）。有 metadata 时 `adjust_head` 多一次内存搬移。
4. **失败返回 `-EINVAL`**——必须检查返回值，否则你以为 push 了头部，其实没有，后续写包会写到错误的偏移。

### 3.3 ⚠️ 调用 `adjust_*` 之后，verifier 会作废所有包指针

`net/core/filter.c:7750-7775` 的 `bpf_helper_changes_pkt_data()` 列出的 helper 中，XDP 相关的有三个：

```c
	    func == bpf_xdp_adjust_head ||
	    func == bpf_xdp_adjust_meta ||
	    ...
	    func == bpf_xdp_adjust_tail ||
```

含义：**调用这些 helper 之后，所有之前 verifier 已验证过的包指针全部失效**，必须重新做 `data_end` 边界检查。这是新手最常见的 `invalid mem access` 来源：

```c
/* ❌ 错误：adjust_head 之后再解引用旧指针 */
struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end) return XDP_DROP;
bpf_xdp_adjust_head(ctx, 0 - (int)sizeof(struct iphdr));
if (eth->h_proto == ...) { }        /* verifier: R1 invalid mem access */

/* ✅ 正确：adjust 之后重新取指针、重新检查 */
if (bpf_xdp_adjust_head(ctx, 0 - (int)sizeof(struct iphdr)))
        return XDP_DROP;
data = (void *)(long)ctx->data;
data_end = (void *)(long)ctx->data_end;
struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end) return XDP_DROP;
```

UAPI 文档（`include/uapi/linux/bpf.h:2616-2620`）也明确写了这一点：

> A call to this helper is susceptible to change the underlying packet buffer. Therefore, at load time, all checks on pointers previously done by the verifier are invalidated and must be performed again, if the helper is used in combination with direct packet access.

### 3.4 不能做的（这是本质限制，不是缺失）

| 能力 | 为什么 XDP 做不到 |
|------|-----------------|
| 查邻居、改 L2 后转发（`redirect_neigh`） | 需要 `struct neighbour` / `dst_entry`，XDP 层还没做路由 |
| 克隆转发（`clone_redirect`） | 需要 skb 的引用计数 |
| 增量校验和更新（`l3/l4_csum_replace`、`csum_update`） | 只有 `csum_diff`（算差值）；XDP 侧需要自己写回 |
| 任意增删包体（`skb_change_tail`、`skb_adjust_room`） | 需要 skb 的线性/非线性区管理 |
| VLAN push/pop | 属于 skb 元数据 |
| 读写 skb 元数据（`mark`、`priority`、`queue_mapping`、`hash`、`tstamp`） | XDP 时**根本不存在 skb** |
| `bpf_sk_assign`（把包绑到指定 socket） | 需要 socket 查找后的绑定点 |

**判断口诀：需要 skb 或路由/邻居信息的，XDP 一律做不到；只涉及「这段内存里的字节」的，XDP 都能做。** 详见 [02](02-bpf.md) 第 4.2 节的对比表。

---

## 4. native / generic / offload：三种模式的真实差异

`XDP_FLAGS_*`（`include/uapi/linux/if_link.h:1295-1304`）：

```c
#define XDP_FLAGS_UPDATE_IF_NOEXIST	(1U << 0)
#define XDP_FLAGS_SKB_MODE		(1U << 1)   /* generic */
#define XDP_FLAGS_DRV_MODE		(1U << 2)   /* native  */
#define XDP_FLAGS_HW_MODE		(1U << 3)   /* offload */
#define XDP_FLAGS_REPLACE		(1U << 4)
```

### 4.1 位置对比（v6.6 源码实测）

| 模式 | 执行位置 | 有没有 skb |
|------|---------|-----------|
| **native**（`DRV_MODE`） | 驱动 NAPI poll 内，`bpf_prog_run_xdp()`（`include/net/xdp.h:482`） | ❌ 还是 DMA 页 |
| **generic**（`SKB_MODE`） | `__netif_receive_skb_core()` 内，`do_xdp_generic()` 调用点在 **`net/core/dev.c:5373`** | ✅ **skb 已分配** |
| **offload**（`HW_MODE`） | 网卡硬件/固件里 | ❌（数据不在主机内存） |

### 4.2 ⚠️ generic XDP 的隐藏代价：每个包可能做一次头部扩展 + 线性化

这是本篇**最重要**的一节。`netif_receive_generic_xdp()`（`net/core/dev.c:4941`）：

```c
static u32 netif_receive_generic_xdp(struct sk_buff *skb,
				     struct xdp_buff *xdp,
				     struct bpf_prog *xdp_prog)
{
	u32 act = XDP_DROP;

	/* Reinjected packets coming from act_mirred or similar should
	 * not get XDP generic processing.
	 */
	if (skb_is_redirected(skb))
		return XDP_PASS;

	/* XDP packets must be linear and must have sufficient headroom
	 * of XDP_PACKET_HEADROOM bytes. This is the guarantee that also
	 * native XDP provides, thus we need to do it here as well.
	 */
	if (skb_cloned(skb) || skb_is_nonlinear(skb) ||
	    skb_headroom(skb) < XDP_PACKET_HEADROOM) {
		int hroom = XDP_PACKET_HEADROOM - skb_headroom(skb);
		int troom = skb->tail + skb->data_len - skb->end;

		/* In case we have to go down the path and also linearize,
		 * then lets do the pskb_expand_head() work just once here.
		 */
		if (pskb_expand_head(skb,
				     hroom > 0 ? ALIGN(hroom, NET_SKB_PAD) : 0,
				     troom > 0 ? troom + 128 : 0, GFP_ATOMIC))
			goto do_drop;
		if (skb_linearize(skb))
			goto do_drop;
	}

	act = bpf_prog_run_generic_xdp(skb, xdp, xdp_prog);
	...
```

**四个必须知道的后果：**

1. **generic XDP 为了「模拟」native 的保证（线性 + 256 字节 headroom），会为每个不满足条件的包执行 `pskb_expand_head()` + `skb_linearize()`。** 这两个操作涉及**内存分配和数据拷贝**——正是 XDP 想省掉的东西。`XDP_PACKET_HEADROOM = 256`（`include/uapi/linux/bpf.h:6276`）。
2. **分配失败 → `goto do_drop` → 静默丢包。** 这在内存压力下会表现为「XDP 程序明明是 `XDP_PASS`，包却不见了」。
3. **被重定向的包（`skb_is_redirected(skb)`）直接 `return XDP_PASS`，跳过 XDP 处理。** 从 `act_mirred` 等 tc action 重新注入的包不会再过一遍 generic XDP——这是个正确但容易困惑的行为。
4. **generic XDP 的位置在 tc ingress 之前**（`do_xdp_generic` 在 `dev.c:5373`，`sch_handle_ingress` 在 `dev.c:5412`），所以「generic XDP 仍然比 tc-BPF 早」是对的，但它**早的那部分已经被 skb 分配 + 可能的线性化抵消了**。

### 4.3 三模式选择表

| 维度 | native | generic | offload |
|------|--------|---------|---------|
| 需要驱动支持 | ✅ 需要（ixgbe/i40e/mlx5/bnxt…） | ❌ 任何网卡 | 需要 SmartNIC（Netronome 等） |
| 有无 skb | ❌ | ✅ **有**（XDP 的主要优势丧失） | ❌（不在主机内存） |
| 额外 per-packet 工作 | 无 | `pskb_expand_head` + `skb_linearize` | 无（在硬件里） |
| `XDP_TX` 队列 | 驱动专属 TX 队列 | **绕过 qdisc，会饿死其他流量** | 硬件队列 |
| 支持 XDP_REDIRECT | ✅ 全部（DEVMAP/CPUMAP/XSKMAP） | ✅ 但走 skb 路径 | 受限 |
| **AF_XDP 零拷贝** | ✅ | ❌ **不可能** | 受限 |
| 用途 | 生产 | **开发/功能验证** | 特殊硬件 |

> **结论：generic XDP 只应该用于「在没有支持的硬件上验证程序逻辑是否正确」，不能用于任何性能评估或生产部署。** 用 generic 模式测出来的数字与 native 完全不可比。

### 4.4 如何确认自己没掉进 generic

```bash
# 方法 1：xdp-tools
xdp-loader status eth0
#   输出里的 mode 字段必须是 native（或 offload）

# 方法 2：bpftool
bpftool net show
#   xdp:
#   eth0(2) driver id 42        ← driver = native ✅
#   eth0(2) generic id 42       ← generic ❌（要排查）
#   eth0(2) offload id 42

# 方法 3：加载时显式指定并要求 DRV_MODE
ip link set dev eth0 xdpgeneric obj prog.bpf.o sec xdp    # 强制 generic
ip link set dev eth0 xdpdrv     obj prog.bpf.o sec xdp    # 强制 native，驱动不支持会失败
ip link set dev eth0 xdpoffload obj prog.bpf.o sec xdp    # 强制 offload
```

**推荐做法：用 `xdpdrv` 而不是 `xdp`。** 前者在驱动不支持时**直接失败**，后者会**静默降级到 generic**——你以为在用 XDP，其实在跑一个退化版本。

---

## 5. 成本结构：XDP 到底省掉了什么

> **本节不给 cycles 数字。** 网上流传的「XDP DROP ~10 cycles / PASS ~20 cycles」这类表格没有可复现的测量条件（网卡、驱动、CPU、包长、是否 GRO、是否 busy poll 都会改变结果），照搬只会误导。**下面给的是「消除了哪些工作」，这个是可以从源码确认的结构性事实。**

### 5.1 一次 `XDP_DROP`（native）省掉的

| 工作项 | 传统路径（无 XDP） | XDP_DROP |
|--------|------------------|----------|
| `build_skb()` 分配 sk_buff | ✅ 要 | ❌ 省 |
| GRO（`napi_gro_receive`） | ✅ 要 | ❌ 省 |
| tc ingress | ✅ 要 | ❌ 省 |
| `ip_rcv` / 路由查找 | ✅ 要 | ❌ 省 |
| Netfilter PRE_ROUTING | ✅ 要 | ❌ 省 |
| `ip_local_deliver` / UDP or TCP 栈 | ✅ 要 | ❌ 省 |
| socket 查找 | ✅ 要 | ❌ 省 |
| `sock_queue_rcv_skb` + `sk_filter` | ✅ 要 | ❌ 省 |
| 拷贝到用户态 | ✅ 要 | ❌ 省 |
| **剩下要做的** | — | 驱动 NAPI poll + XDP 程序 + 页回收 |

**这就是为什么「尽早丢」的价值最大**：一个包被 XDP 丢掉，内核为它做的全部工作就是 NAPI poll 和你的程序。

### 5.2 一次 `XDP_PASS` 之后仍然要付的

`XDP_PASS` **不省任何协议栈开销**，只省掉了 XDP 之前可能存在的其他过滤。它的价值在于：**让「需要上送的包」以最短路径走到 `build_skb()`**。

### 5.3 一次 `XDP_REDIRECT` 到 AF_XDP 的分摊成本

| 阶段 | 说明 | 章节 |
|------|------|------|
| ① XDP 程序执行 | 每包一次 | 本篇 |
| ② `xdp_do_redirect()` 入批量队列 | 每包一次，写 per-CPU `bpf_redirect_info` | [ch07/01](../../chapter-07-xdp-redirect-dpdk/notes/01-xdp-redirect.md) |
| ③ **等待 `xdp_do_flush()`** | **批量提交，包在队列里等待 NAPI poll 结束** | 同上 |
| ④ `xsk_rcv_zc()` / `__xsk_rcv()` | 写 RX ring（zc 只有句柄） | [ch06/01](../../chapter-06-af-xdp/notes/01-af-xdp.md) |
| ⑤ 用户态 `xsk_ring_cons__peek` | **一次性看到整批** | [ch06/02](../../chapter-06-af-xdp/notes/02-af-xdp-lwn.md) |

**第 ③ 步的「等待」是隐藏成本**：包在批量队列里停留的时间取决于 NAPI poll 还要处理多少包。这是「吞吐优先」和「延迟优先」的直接权衡——详见 [ch06/02](../../chapter-06-af-xdp/notes/02-af-xdp-lwn.md) 的延迟预算表。

### 5.4 如何测

三层方法，与 [01](01-ebpf-net-bootlin.md) 第 5 节一致：

```bash
# L1 结构层：确认模式与挂载
bpftool net show
xdp-loader status eth0

# L2 聚合层：per-程序执行次数与耗时（⚠️ 有埋点开销）
sysctl -w kernel.bpf_stats_enabled=1
bpftool prog show id <ID>          # run_cnt / run_time_ns
sysctl -w kernel.bpf_stats_enabled=0

# L3 端到端：硬件时间戳（最可信）
ethtool -T eth0                    # 确认支持 HW timestamps
# 配合 SO_TIMESTAMPING 在用户态取收包时刻

# L4 内核层：XDP 异常与丢包原因
bpftrace -e 'tracepoint:xdp:xdp_exception     { @[args->act] = count(); }'
bpftrace -e 'tracepoint:xdp:xdp_redirect_err  { @[args->err] = count(); }'
```

---

## 6. 实战：一行都不能错的行情组播过滤模板

原始笔记里那段代码有一个**致命 bug**（见第 7 节 Q1）。下面是修正后的可用版本：

```c
/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <bpf/bpf_helpers.h>

/* 目标行情组播组：225.0.0.1 */
#define MARKET_DATA_GROUP  0xe1000001   /* 网络字节序下的 225.0.0.1 */

struct {
        __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
        __uint(max_entries, 8);
        __type(key, __u32);
        __type(value, __u64);
} stats SEC(".maps");

enum {
        S_TOTAL = 0,
        S_NON_IP,
        S_NON_UDP,
        S_HIT,
        S_MISS,
        S_MALFORMED,
};

static __always_inline void bump(__u32 key)
{
        __u64 *v = bpf_map_lookup_elem(&stats, &key);
        if (v)
                *v += 1;
}

SEC("xdp")
int xdp_md_filter(struct xdp_md *ctx)
{
        void *data     = (void *)(long)ctx->data;
        void *data_end = (void *)(long)ctx->data_end;

        bump(S_TOTAL);

        /* ① 每个指针解引用前都必须先做 data_end 检查 */
        struct ethhdr *eth = data;
        if ((void *)(eth + 1) > data_end) {
                bump(S_MALFORMED);
                return XDP_DROP;
        }
        if (eth->h_proto != bpf_htons(ETH_P_IP)) {
                bump(S_NON_IP);
                return XDP_PASS;        /* 非 IP（ARP 等）必须放行 */
        }

        struct iphdr *ip = (void *)(eth + 1);
        if ((void *)(ip + 1) > data_end) {
                bump(S_MALFORMED);
                return XDP_DROP;
        }
        /* ip->ihl 可变，必须校验，不能假设 20 字节 */
        if (ip->ihl < 5) {
                bump(S_MALFORMED);
                return XDP_DROP;
        }
        void *l4 = (void *)ip + (ip->ihl * 4);
        if (ip->protocol != IPPROTO_UDP) {
                bump(S_NON_UDP);
                return XDP_PASS;
        }

        struct udphdr *udp = l4;
        if ((void *)(udp + 1) > data_end) {
                bump(S_MALFORMED);
                return XDP_DROP;
        }

        /* ② 命中目标组播组 → 交给上层（或 redirect 到 AF_XDP） */
        if (ip->daddr == bpf_htonl(MARKET_DATA_GROUP)) {
                bump(S_HIT);
                return XDP_PASS;
        }

        /* ③ 未命中 → 尽早丢弃，省掉整个协议栈 */
        bump(S_MISS);
        return XDP_DROP;
}

char _license[] SEC("license") = "GPL";
```

**四个关键点：**

1. **非 IP 包必须 `XDP_PASS` 而不是 `XDP_DROP`**（原笔记写的是 DROP）。否则 ARP 被丢 → 邻居表老化 → 整机会话中断。
2. **`ip->ihl` 必须校验**。IPv4 头长度可变（options），写死 `+20` 会解析错位。
3. **`PERCPU_ARRAY` 做计数器**，避免多队列并发写的原子竞争。
4. **计数器 map 是唯一可靠的观测手段**——tcpdump 看不到 XDP 之后的包。

**配套观测：**

```bash
bpftool map dump name stats
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'
```

---

## 7. 代码自测

<details>
<summary>Q1：原笔记里的示例代码有 3 个 bug，你能全部找出来吗？（代码：<code>if (eth->h_proto != bpf_htons(ETH_P_IP)) return XDP_DROP;</code> 等）</summary>

原笔记的代码：

```c
    if ((void*)(eth + 1) > data_end) return XDP_DROP;
    if (eth->h_proto != bpf_htons(ETH_P_IP)) return XDP_PASS;

    struct iphdr *ip = (void*)(eth + 1);
    if ((void*)(ip + 1) > data_end) return XDP_DROP;

    if (ip->daddr == bpf_htonl(0xe1000001)) {  // 225.0.0.1
        return XDP_PASS;
    }
    return XDP_DROP;
```

**Bug 1（最致命）：`return XDP_DROP` 丢掉了所有非 IP 包。**

表面上 `eth->h_proto != ETH_P_IP` 时返回的是 `XDP_PASS`（第 2 行，写对了）。但**第 1 行和第 4 行的 `XDP_DROP` 会把「畸形包」和「IP 头不完整的包」丢掉**——这些包里混杂着 VLAN 封装（`ETH_P_8021Q`）、PPPoE、MPLS 等。更要命的是：**如果这段代码后面接了 `bpf_redirect_map()` 分流，`XDP_DROP` 之外还有 `XDP_ABORTED == 0` 的陷阱**。

真正会立刻炸的是另一个变体：很多人把 `XDP_PASS` 写成了 `XDP_DROP`，结果 **ARP 请求全被丢** → 网关 MAC 学不到 → 邻居表老化 → 几分钟后整机上不了网，而 `ping` 不通的第一反应是「网络问题」，想不到是 XDP。

**正确原则：非目标流量且你不确定是什么 → `XDP_PASS`；确认为攻击/噪声 → `XDP_DROP`。**

---

**Bug 2（隐蔽）：`ip->daddr` 的解引用可能越界。**

```c
struct iphdr *ip = (void*)(eth + 1);
if ((void*)(ip + 1) > data_end) return XDP_DROP;
```

`ip + 1` 只检查了 **20 字节**（`sizeof(struct iphdr)`）。但访问 `ip->daddr` 的偏移是 16–19 字节，在 20 字节内，所以**这里侥幸没问题**。

**真正的问题在别处**：IPv4 头长度由 `ip->ihl * 4` 决定，`ihl` 最小是 5（20 字节），但可以更大（options）。如果后续要访问 UDP 头，写成 `(void *)ip + 20` 就错了——**IP options 存在时 UDP 头不在偏移 20**。而且 `ip->ihl` 本身可能是 0（畸形包），导致 `ip->ihl * 4 = 0`，指针反而往回走。

正确写法：

```c
if (ip->ihl < 5) return XDP_DROP;              /* 先校验 ihl 合法 */
void *l4 = (void *)ip + (ip->ihl * 4);         /* 再按实际头长前进 */
struct udphdr *udp = l4;
if ((void *)(udp + 1) > data_end) return XDP_DROP;
```

---

**Bug 3（所有新手都会踩）：只比较 `daddr` 就认为是目标行情流。**

`225.0.0.1` 是组播地址，但**同一个 MAC 上可能跑着多个组播组**。只比较目的 IP 会漏掉：

- **不同端口的同一组播组**（行情通常一组一端口，但快照/增量/逐笔可能同组不同口）；
- **源地址过滤**——行情网关可能有主备两条流，主用 `10.0.0.1`、备用 `10.0.0.2`，只按 daddr 会把备用流也算进去（或者反过来，你想做主备切换时却区分不了）。

生产环境至少要做 **(daddr, dport)** 二元组，最好加上 **src_addr** 做三元组。

---

**附带的第 4 个问题：`0xe1000001` 的字节序写法容易出错。**

```c
if (ip->daddr == bpf_htonl(0xe1000001))
```

`ip->daddr` 是**网络字节序**。`bpf_htonl(0xe1000001)` 在小端机上把 `0xe1000001` 转成网络序——`0xe1000001` 的字节序列是 `e1 00 00 01`，转成网络序后，整数值在小端机上变成 `0x010000e1`。

这个写法**恰好是对的**（因为 `225.0.0.1` 的点分十进制转整数就是 `0xe1000001`，而 `ip->daddr` 存的正是这个值的大端表示）。但极易被误改成 `bpf_ntohl(ip->daddr) == 0xe1000001`（也对）或直接 `ip->daddr == 0xe1000001`（**错**）。

**建议写法**：统一用 `bpf_ntohl(ip->daddr) == 0xe1000001`，与人类书写的点分十进制顺序一致，不容易搞反。

</details>

<details>
<summary>Q2：你的 XDP 程序在一台开发机上跑得好好的，换到另一台机器后同样的程序「生效了但性能没提升」，而且偶尔有包莫名其妙丢掉。可能是什么原因？</summary>

**第一个怀疑对象：掉进了 generic 模式（`XDP_FLAGS_SKB_MODE`）。**

`ip link set dev eth0 xdp obj ...` 在驱动不支持时**会静默降级到 generic**。generic 模式的 `XDP` 跑在 `do_xdp_generic()`（`net/core/dev.c:5032`），位置是 `__netif_receive_skb_core()` 内的 `dev.c:5373`——**skb 已经分配完了**。

而且它还要为每个包补做 native 的保证（`net/core/dev.c:4955-4970`）：

```c
	if (skb_cloned(skb) || skb_is_nonlinear(skb) ||
	    skb_headroom(skb) < XDP_PACKET_HEADROOM) {
		int hroom = XDP_PACKET_HEADROOM - skb_headroom(skb);
		int troom = skb->tail + skb->data_len - skb->end;

		if (pskb_expand_head(skb,
				     hroom > 0 ? ALIGN(hroom, NET_SKB_PAD) : 0,
				     troom > 0 ? troom + 128 : 0, GFP_ATOMIC))
			goto do_drop;
		if (skb_linearize(skb))
			goto do_drop;
	}
```

**这正好解释了你的两个症状：**

| 症状 | 原因 |
|------|------|
| 性能没提升 | `pskb_expand_head()` + `skb_linearize()` 涉及**内存分配和数据拷贝**——正是 XDP 想省的东西。generic 把这些又加回来了，还多跑一次 XDP 程序。 |
| 偶尔丢包 | `pskb_expand_head()` / `skb_linearize()` 失败（内存压力）→ `goto do_drop` → **静默丢包**。你的 XDP 程序根本没被调用，包就没了。这种丢包**不计入任何计数器**，只在 `tracepoint:skb:kfree_skb` 里能看到。 |

**排查：**

```bash
bpftool net show
#   eth0(2) driver id 42   ← native ✅
#   eth0(2) generic id 42  ← ❌ 就是这里

xdp-loader status eth0     # 看 mode 字段
```

**修复：** 用 `xdpdrv` 强制 native：

```bash
ip link set dev eth0 xdpdrv obj prog.bpf.o sec xdp
# 驱动不支持时会直接报错，不会静默降级
```

支持 native XDP 的主流驱动：`ixgbe`（X550/X710 系列）、`i40e`、`ice`、`mlx5_core`（Mellanox/ NVIDIA）、`bnxt_en`（Broadcom）、`virtio_net`、`veth`、`tun`。**注意 `e1000e`、`r8169` 这类普通桌面网卡不支持。**

**第二个怀疑对象：`XDP_TX` 在 generic 下绕过 qdisc。**

如果你的程序用了 `XDP_TX`，generic 路径的 `generic_xdp_tx()`（`dev.c:5000`）源码注释明确警告：

```
/* When doing generic XDP we have to bypass the qdisc layer and the
 * network taps in order to match in-driver-XDP behavior. This also means
 * that XDP packets are able to starve other packets going through a qdisc,
 * and DDOS attacks will be more effective. In-driver-XDP use dedicated TX
 * queues, so they do not have this starvation issue.
 */
```

即：generic + `XDP_TX` 会绕开 qdisc，**饿死其他流量**。

</details>

<details>
<summary>Q3：你在 XDP 程序里调用 <code>bpf_xdp_adjust_head()</code> push 了一个 IP 头，之后访问原来的 <code>eth</code> 指针，verifier 报 <code>invalid mem access 'inv'</code>。为什么？返回值你已经检查了。</summary>

**因为 `bpf_xdp_adjust_head()` 会让 verifier 作废所有已验证的包指针——这和返回值成不成功无关，是 verifier 在加载时的静态规则。**

`net/core/filter.c` 的 `bpf_helper_changes_pkt_data()`（`:7750-7775`）把这三个 XDP helper 列入了「会改变包数据」的名单：

```c
	    func == bpf_xdp_adjust_head ||
	    func == bpf_xdp_adjust_meta ||
	    ...
	    func == bpf_xdp_adjust_tail ||
```

verifier 在看到对它们的调用后，会**主动 invalidate 所有 `PTR_TO_PACKET` 类型的寄存器**。UAPI 文档（`include/uapi/linux/bpf.h:2616-2620`）写得很明确：

> A call to this helper is susceptible to change the underlying packet buffer. Therefore, at load time, **all checks on pointers previously done by the verifier are invalidated and must be performed again**, if the helper is used in combination with direct packet access.

**正确写法（重新取指针、重新检查）：**

```c
/* push 一个 IP 头 */
if (bpf_xdp_adjust_head(ctx, 0 - (int)sizeof(struct iphdr)))
        return XDP_DROP;

/* ⭐ 必须重新读 ctx->data / ctx->data_end */
void *data     = (void *)(long)ctx->data;
void *data_end = (void *)(long)ctx->data_end;

/* ⭐ 必须重新做边界检查 */
struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
/* 现在才能用 eth */
```

**顺带三条 `adjust_head` 的硬约束**（`net/core/filter.c:3874`）：

```c
	void *xdp_frame_end = xdp->data_hard_start + sizeof(struct xdp_frame);
	unsigned long metalen = xdp_get_metalen(xdp);
	void *data_start = xdp_frame_end + metalen;
	void *data = xdp->data + offset;

	if (unlikely(data < data_start ||
		     data > xdp->data_end - ETH_HLEN))
		return -EINVAL;
```

1. **下界**：`data` 不能前移到 `data_hard_start + sizeof(struct xdp_frame) + metalen` 之前。那块区域要留给 `XDP_REDIRECT` 时转换出的 `struct xdp_frame`。实际可用 headroom 大约是 `XDP_PACKET_HEADROOM(256)` 减去 `sizeof(struct xdp_frame)`。
2. **上界**：`data` 不能后移到 `data_end - ETH_HLEN`（14 字节）之后——**XDP 不允许把包缩到小于一个以太网头**。
3. **metadata 会跟着搬**：如果用了 `bpf_xdp_adjust_meta()`，`adjust_head` 会额外做一次 `memmove(data_meta + offset, data_meta, metalen)`。
4. 失败返回 **`-EINVAL`**——**必须检查返回值**。不检查的话，一旦失败你后续的写包会写到错误偏移，产生一个「看起来合法但内容错乱」的包，这种 bug 极难排查。

</details>

---

## 导航

- **本篇：** [01-ebpf-net-bootlin.md](01-ebpf-net-bootlin.md) · [02-bpf.md](02-bpf.md) · [04-cgroup-bpf.md](04-cgroup-bpf.md)
- **深入 XDP：** [chapter-05-xdp-architecture](../../chapter-05-xdp-architecture/) XDP 架构 · [chapter-06-af-xdp](../../chapter-06-af-xdp/) AF_XDP · [chapter-07-xdp-redirect-dpdk](../../chapter-07-xdp-redirect-dpdk/) redirect 与 DPDK 对比
- **相关：** `06.7-bpf-observability/`
- **章节主页：** [README](../README.md)
