# 04 — xdp_buff ↔ sk_buff：三种转换路径

> **对应 Rosen:** Ch1/Ch11（Rosen 时代 `sk_buff` 是唯一表示）
> **内核版本:** `xdp_buff` 4.8+、`xdp_frame` 4.18+，本文以 **v6.6** 为准
> **源码:** `include/net/xdp.h`、`net/core/xdp.c`、`include/linux/skbuff.h`

## 文档概述

XDP 最重要的设计不是"快"，而是**引入了一种新的包表示**——`xdp_buff`。同一个以太网帧，在内核里可能以三种形态存在：

| 表示 | 本质 | 生命周期 |
|------|------|---------|
| `xdp_buff` | **栈上的临时视图**，指向 DMA 缓冲区 | 只在 XDP 程序执行期间有效 |
| `xdp_frame` | **可脱离队列的帧**（能跨 CPU、能排队） | REDIRECT / cpumap / devmap 期间 |
| `sk_buff` | **完整的协议栈表示** | XDP_PASS 之后直到 socket |

本篇讲这三种形态之间**怎么转换**。原笔记只画了一张"分流图"（DROP / PASS / TX / REDIRECT），没讲转换本身——而转换的实现细节里藏着两个关键事实：

1. **`xdp_frame` 就存在包自己的 headroom 里**（`xdp_convert_buff_to_frame()` 把 `struct xdp_frame` 写在 `xdp->data_hard_start`）
2. **XDP_PASS 是零拷贝的**：page_pool 的页面直接变成 skb 的数据区，不复制

本篇与兄弟篇的分工：

| 篇 | 讲什么 |
|----|--------|
| **04（本篇）** | 三种表示之间的**转换实现** |
| [chapter-01](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) | 各 hook 点的**先后顺序**（XDP 在 skb 之前还是之后） |
| [03 sk_buff 生命周期](03-sock-sk-buff.md) | `sk_buff` 的**分配/克隆/释放** |
| [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) | XDP 程序本身怎么写 |

---

## 一、两种"轻量表示"的真实字段

### `struct xdp_buff`（v6.6 `include/net/xdp.h`）

```c
struct xdp_buff {
	void *data;
	void *data_end;
	void *data_meta;
	void *data_hard_start;
	struct xdp_rxq_info *rxq;
	struct xdp_txq_info *txq;
	u32 frame_sz; /* frame size to deduce data_hard_end/reserved tailroom*/
	u32 flags; /* supported values defined in xdp_buff_flags */
};
```

**8 个字段。** 常见资料说"4 个字段"（`data`/`data_end`/`data_meta`/`data_hard_start`）是 4.8 刚引入时的形态，`rxq` / `txq` / `frame_sz` / `flags` 是后来加的。

注意 `xdp_buff` **不描述缓冲区的总长度**，只有 `frame_sz`（整帧大小）。`data_hard_end` 是**推算**出来的：`data_hard_start + frame_sz`。这跟 `sk_buff` 有 `head`/`end` 显式边界的做法不同。

### `struct xdp_frame`（v6.6 `include/net/xdp.h`）

```c
struct xdp_frame {
	void *data;
	u16 len;
	u16 headroom;
	u32 metasize; /* uses lower 8-bits */
	/* Lifetime of xdp_rxq_info is limited to NAPI/enqueue time,
	 * while mem info is valid on remote CPU.
	 */
	struct xdp_mem_info mem;
	struct net_device *dev_rx; /* used by cpumap */
	u32 frame_sz;
	u32 flags; /* supported values defined in xdp_buff_flags */
};
```

源码里那句注释很关键：

> *"Lifetime of `xdp_rxq_info` is limited to NAPI/enqueue time, while mem info is valid on remote CPU."*

翻译：**`xdp_buff` 里的 `rxq` 指针只在当前 NAPI 周期内有效**，一旦包要跨 CPU（cpumap）或跨设备（devmap），那个指针就会失效。所以 `xdp_frame` 不存 `rxq`，改存 **`mem`（内存类型 + 分配器 id）**——因为内存怎么归还，在远端 CPU 上也是成立的。

### 三形态对照

| 维度 | `xdp_buff` | `xdp_frame` | `sk_buff` |
|------|-----------|------------|-----------|
| 存储位置 | **栈上**（驱动 `napi_poll` 的局部变量） | **包自己的 headroom 里** | 独立 `sk_buff` 结构 + 数据区 |
| 字段数 | 8 | 8 | 几十个（v6.6 已外移部分到 `skb_ext`） |
| 能跨 CPU 吗 | ❌（`rxq` 会失效） | ✅（改用 `mem`） | ✅ |
| 能排队吗 | ❌ | ✅ | ✅ |
| 见过协议栈吗 | ❌ | ❌ | ✅ |
| 有 `len` 吗 | 需算 `data_end - data` | ✅ `len` | ✅ `len` |
| 分配成本 | **零**（栈上构造） | 零（写 headroom） | 一次 slab 分配 |

---

## 二、三种转换路径

```
                         NIC DMA（写入 page_pool 的页面）
                                   │
                                   v
                    ┌──────────────────────────────┐
                    │  xdp_buff（栈上临时视图）      │
                    │  data / data_end / rxq ...    │
                    └──────────────────────────────┘
                                   │
                          XDP 程序返回动作
                                   │
        ┌──────────────┬───────────┼────────────┬──────────────┐
        │              │           │            │              │
    XDP_DROP      XDP_PASS     XDP_TX     XDP_REDIRECT    XDP_ABORTED
        │              │           │            │              │
        │              │           │            │              │
   page 归还      【转换 A】   原路反弹     【转换 B】        丢弃 +
   page_pool    napi_build_skb()      xdp_convert_         trace_xdp_
        │              │              buff_to_frame()       exception
        │              │                    │
        │              v                    │
        │      ┌───────────────┐            ├─────────> cpumap（跨 CPU）
        │      │   sk_buff     │            │                │
        │      │ 数据区 = 同一个│            ├─────────> devmap（跨设备）
        │      │ page_pool 页   │            │                │
        │      │ （零拷贝！）    │            └─────────> AF_XDP（用户态）
        │      └───────────────┘                             │
        │              │                                     │
        │              v                             【转换 C】
        │        协议栈 → socket                 xdp_build_skb_from_frame()
        │                                                    │
        │                                                    v
        │                                            ┌───────────────┐
        │                                            │   sk_buff     │
        │                                            └───────────────┘
        v                                                    │
    结束（最便宜）                                     协议栈 → socket
```

### 转换 A：`xdp_buff` → `sk_buff`（XDP_PASS）

驱动在 XDP 返回 PASS 后调用（v6.6 `include/linux/skbuff.h:1273`）：

```c
struct sk_buff *napi_build_skb(void *data, unsigned int frag_size);
```

**只分配 `sk_buff` 头，数据区不拷贝**——`data` 直接指向 page_pool 的那个页面。这就是"零拷贝接进协议栈"的含义。

⚠️ 注意：这里**没有**一个叫 `xdp_update_skb()` 的函数把 xdp_buff 的字段搬过去。驱动要自己用 `napi_build_skb()` 构造 skb，再手工设置 protocol / pkt_type 等。这一点和很多资料的简化描述不同——**XDP_PASS 之后是驱动的责任，不是一个统一的内核辅助函数**。

（内核里确实有 `xdp_update_skb_shared_info()`，见 `include/net/xdp.h:226`，但它只处理 `skb_shared_info` 的分片信息，用于 multi-buf 场景，不是通用的"xdp_buff 转 skb"。）

### 转换 B：`xdp_buff` → `xdp_frame`（REDIRECT 用）

**本篇最有意思的一段**（v6.6 `include/net/xdp.h:291`）：

```c
/* Convert xdp_buff to xdp_frame */
static inline
struct xdp_frame *xdp_convert_buff_to_frame(struct xdp_buff *xdp)
{
	struct xdp_frame *xdp_frame;

	if (xdp->rxq->mem.type == MEM_TYPE_XSK_BUFF_POOL)
		return xdp_convert_zc_to_xdp_frame(xdp);

	/* Store info in top of packet */
	xdp_frame = xdp->data_hard_start;
	if (unlikely(xdp_update_frame_from_buff(xdp, xdp_frame) < 0))
		return NULL;

	/* rxq only valid until napi_schedule ends, convert to xdp_mem_info */
	xdp_frame->mem = xdp->rxq->mem;

	return xdp_frame;
}
```

三个关键点：

**① `xdp_frame` 写在包的 headroom 里**（`xdp_frame = xdp->data_hard_start;`）。

那个注释 `/* Store info in top of packet */` 就是字面意思：`struct xdp_frame` 被**就地写进包前面的那段空白**。零额外分配。

**这解释了 XDP 为什么强制要求 headroom**。包前面那段空白（默认 256 字节）不是"为了方便加封装头"这么简单——它是 `xdp_frame` 的**存储空间**。没有它，REDIRECT 就得为每帧额外分配一个 `xdp_frame` 结构。

> 通用 XDP（generic XDP，非 native）在 `net/core/dev.c:4954` 附近有这样的检查：headroom 不足时**先拷贝一份 skb** 补出 `XDP_PACKET_HEADROOM` 字节再跑 XDP 程序。这就是通用 XDP 比 native XDP 慢的一个原因——它可能多一次拷贝。

**② `rxq` 换成 `mem`**：那句注释 `rxq only valid until napi_schedule ends` 说明了一切。`xdp_buff` 的 `rxq` 指针生命周期只到 NAPI 周期结束，跨 CPU 就废了；`xdp_frame` 存 `struct xdp_mem_info`（内存类型 + 分配器 id），在远端 CPU 上也能正确归还内存。

**③ AF_XDP 零拷贝走特殊分支**：`MEM_TYPE_XSK_BUFF_POOL` 时调用 `xdp_convert_zc_to_xdp_frame()`，因为它的内存来自用户态 UMEM，归还逻辑不同。

### 转换 C：`xdp_frame` → `sk_buff`（cpumap / veth / devmap）

当帧被转到另一个 CPU（cpumap）或另一个设备（veth、devmap）后，接收侧要把它变成 skb 才能上协议栈（v6.6 `net/core/xdp.c:656`）：

```c
struct sk_buff *xdp_build_skb_from_frame(struct xdp_frame *xdpf,
					 struct net_device *dev)
{
	struct sk_buff *skb;

	skb = kmem_cache_alloc(skbuff_cache, GFP_ATOMIC);
	if (unlikely(!skb))
		return NULL;

	memset(skb, 0, offsetof(struct sk_buff, tail));

	return __xdp_build_skb_from_frame(xdpf, skb, dev);
}
```

两个细节：

- **用的是 `skbuff_cache`**，不是 `skbuff_fclone_cache` —— 说明这条路<b>不打算 clone</b>。
- `memset(skb, 0, offsetof(struct sk_buff, tail))` —— **只清零到 `tail` 字段为止**，`tail` 之后的字段不清。这是精心设计的：热路径上能少清几个字节就少清。（对照本篇 [03 篇](03-sock-sk-buff.md) 的 `sk_buff` 布局图，你就明白为什么要清到 `tail` 为止。）

---

## 三、内存类型：`xdp_mem_info` 决定"归还给谁"

`xdp_frame->mem` 的类型决定这个帧最终还给哪个分配器：

| `MEM_TYPE_*` | 内存来自 | 归还路径 | 用在哪 |
|-------------|---------|---------|--------|
| `MEM_TYPE_PAGE_SHARED` | page_pool（page 引用计数 > 1） | `page_pool_put_page()` | 常规 XDP |
| `MEM_TYPE_PAGE_ORDER0` | page_pool（独占 page） | `page_pool_put_page()` | 常规 XDP |
| `MEM_TYPE_PAGE_POOL` | page_pool | `page_pool_put_page()` | 常规 XDP |
| `MEM_TYPE_XSK_BUFF_POOL` | **AF_XDP 的 UMEM**（用户态内存） | 归还到 XSK 的 FILL ring | AF_XDP 零拷贝 |

最后一行是 AF_XDP 零拷贝的核心：帧**从来不属于内核**，它一直在用户态注册的 UMEM 里，内核只是借用一下"归还权"。所以 `xdp_convert_zc_to_xdp_frame()` 才需要单独一条路径。

> 详见 [chapter-06 AF_XDP](../../chapter-06-af-xdp/)。

---

## 四、为什么 XDP_DROP 那么便宜

原笔记给的数字（传统 ~300 cycles，XDP DROP ~10 cycles）方向是对的，但要说清**省掉了什么**：

| 项目 | 传统路径 | XDP_DROP |
|------|---------|----------|
| 分配数据区 | page_pool / alloc_page | ❌ 省（页面已经在 ring 里） |
| 构造 `xdp_buff` | — | 栈上填 8 个字段，**几百 ns 不到** |
| 分配 `sk_buff` | 一次 slab 分配 | ❌ 省 |
| 初始化 `sk_buff` | 几十个字段的 memset | ❌ 省 |
| GRO 处理 | 查流表、尝试合并 | ❌ 省 |
| 协议栈遍历 | ptype_all → tc → netfilter → L3 → L4 | ❌ 全省 |
| 归还页面 | 从 skb 释放路径 | **直接归还 page_pool**，一步到位 |

**注意"省"的顺序**：XDP_DROP 不是"做了某件事很快"，而是**什么都没做**。它连 `sk_buff` 都没生成，也就没有"释放 skb"这一步——页面直接从 XDP 层就还回 page_pool 了。

这也是为什么 `xdp_buff` 必须是栈上的临时结构：它不需要分配，也就不需要释放。

---

## 五、观测

```bash
# XDP 各动作的计数（需要驱动支持，或用 XDP 程序自己维护 map）
bpftool prog show
bpftool map dump id <id>

# XDP 异常（XDP_ABORTED / 程序错误）
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'

# XDP_REDIRECT 是否成功（失败会走 xdp_redirect_err）
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'
bpftrace -e 'tracepoint:xdp:xdp_redirect { @[args->prog_id] = count(); }'

# 确认是 native 还是 generic（generic 会慢很多且可能多一次拷贝）
bpftool net show dev eth0
#   显示 offloaded / native / generic

# page_pool 统计（⚠️ v6.6 主线没有 /proc/net/page_pools，见 ch04/01）
grep PAGE_POOL_STATS /boot/config-$(uname -r)
ethtool -S eth0 | grep -i pp_
#   关注 rx_pp_alloc_fast（缓存命中率）与 rx_pp_recycle_released_ref（页面漏出）
#   → [chapter-04/01](../../chapter-04-page-pool/notes/01-page-pool.md)
```

---

## HFT 要点

- **`xdp_buff` 是栈上的，零分配**——这是它快的根本原因，不要理解成"结构小所以快"。
- **`xdp_frame` 存在包自己的 headroom 里**：这是 XDP 强制 headroom 的真正理由（不只是为了加封装头）。
- **`xdp_buff` 不能跨 CPU，`xdp_frame` 能**：因为 `rxq` 指针的生命周期只到 NAPI 周期结束，跨 CPU 必须换成 `mem`。做 cpumap 分流时这条是硬约束。
- **XDP_PASS 是零拷贝的**：`napi_build_skb()` 只分配 skb 头，数据区沿用 page_pool 的页面。但**之后就是驱动的责任**，没有统一的转换函数。
- **通用 XDP 可能多一次拷贝**：headroom 不足时内核要先拷贝 skb 补出 `XDP_PACKET_HEADROOM`。所以 generic XDP 的性能数据和 native 不可比。
- **`xdp_build_skb_from_frame()` 只用 `skbuff_cache`**：cpumap / veth 这条路径明确不打算 clone，所以别指望它走 fclone 快路径。
- **`memset` 只到 `tail`**：连清零都精打细算，可见 `sk_buff` 初始化的成本有多敏感。
- **AF_XDP 零拷贝走独立的 `MEM_TYPE_XSK_BUFF_POOL` 路径**：帧一直在用户态 UMEM 里，内核只有借用权。这是 AF_XDP 零拷贝与"AF_XDP copy 模式"的本质区别。

## 与 Rosen 3.x 的差异

Rosen 写作时（2.6 时代）XDP 还不存在，`sk_buff` 是收包路径上**唯一**的包表示。所以：

| Rosen 3.x 的图景 | 现在（5.x/6.x） |
|-----------------|----------------|
| 收包必先分配 `sk_buff` | XDP 阶段只有 `xdp_buff`（栈上，零分配） |
| 一种表示贯穿全程 | **三种表示**：`xdp_buff` / `xdp_frame` / `sk_buff` |
| 数据区必有一次 `alloc_page` + 拷贝 | page_pool 页面直接变成 skb 数据区，**零拷贝** |
| 丢包 = 释放 skb | XDP_DROP 直接还 page_pool，skb 从未存在 |
| 无跨 CPU 的包表示 | `xdp_frame` 专为跨 CPU / 跨设备设计 |
| 无用户态内存直通 | `MEM_TYPE_XSK_BUFF_POOL` 让帧始终住在用户态 UMEM |

**方法论上的差异**：Rosen 教的是"包沿着协议栈逐层上移"；XDP 引入的是"**先决定这个包值不值得走协议栈**"。前者优化的是每一层的成本，后者优化的是**要不要走**。

---

## 代码自测

<details>
<summary>Q1：你在写 XDP 程序做 <code>XDP_REDIRECT</code> 到 cpumap，发现跨 CPU 之后程序崩了或行为异常。最可能踩了什么坑？</summary>

<b>答：</b>最可能是<b>试图在远端 CPU 上使用 `xdp_buff` 里的 `rxq` 信息</b>，或者依赖了 `xdp_buff` 本身的生命周期。

`include/net/xdp.h` 里那句注释是硬约束：

> *"Lifetime of `xdp_rxq_info` is limited to NAPI/enqueue time, while mem info is valid on remote CPU."*

`xdp_buff` 是**栈上**的视图，`rxq` 指针只在当前 NAPI 周期有效。跨 CPU 之后，那个指针指向的 `xdp_rxq_info` 可能已经被回收或复用了。

正确做法：REDIRECT 时内核会用 `xdp_convert_buff_to_frame()` 把 `xdp_buff` 转成 `xdp_frame`，其中：

```c
/* rxq only valid until napi_schedule ends, convert to xdp_mem_info */
xdp_frame->mem = xdp->rxq->mem;
```

`xdp_frame` **不保存 `rxq`**，只保存 `mem`（内存类型 + 分配器 id）——因为"这块内存该还给谁"在远端 CPU 上依然成立。

所以排查方向：<b>你的程序里有没有引用 `xdp->rxq` 的字段？有没有把 `xdp_buff` 的指针存下来跨周期使用？</b>两者都是未定义行为。
</details>

<details>
<summary>Q2：为什么 XDP 强制要求包前面留出 headroom？很多人回答"为了 <code>bpf_xdp_adjust_head()</code> 加封装头"，这个答案完整吗？</summary>

<b>答：</b>不完整。加封装头只是用途之一，<b>更硬性的理由是 `xdp_frame` 要存在那里</b>。

看 `xdp_convert_buff_to_frame()`：

```c
/* Store info in top of packet */
xdp_frame = xdp->data_hard_start;
```

`struct xdp_frame` 被**就地写进包前面的空白区**（`data_hard_start` 指向的位置），零额外分配。如果没有那段 headroom，每次 REDIRECT 都得额外分配一个 `xdp_frame` 结构——那就不是"零成本转换"了。

反证：通用 XDP（generic）路径在 `net/core/dev.c:4954` 附近检查 headroom，不足时<b>先拷贝一份 skb</b> 补出 `XDP_PACKET_HEADROOM` 字节。这说明 headroom 是<b>不可协商的先决条件</b>，不是优化选项。

所以完整答案是三层：
1. 存 `xdp_frame`（REDIRECT 必需）
2. `bpf_xdp_adjust_head()` 往前加封装头
3. 协议栈接收时需要的 `skb` headroom
</details>

<details>
<summary>Q3：同是"把包变成 <code>sk_buff</code>"，<code>XDP_PASS</code> 和 <code>xdp_build_skb_from_frame()</code> 有什么本质区别？</summary>

<b>答：</b>两者的数据来源和上下文完全不同：

| | `XDP_PASS`（`napi_build_skb()`） | `xdp_build_skb_from_frame()` |
|---|---|---|
| 输入 | `xdp_buff`（栈上视图） | `xdp_frame`（存在 headroom 里） |
| 调用者 | **驱动自己**，在 `napi_poll` 里 | 内核（cpumap / veth / devmap 的接收侧） |
| 上下文 | 同一个 NAPI 周期、同一个 CPU | 可能是**另一个 CPU** |
| slab cache | 驱动决定 | 固定 `skbuff_cache`（不打算 clone） |
| 内存归还 | page_pool（同 CPU 直接还） | 按 `xdp_frame->mem` 归还，可能跨 CPU 批量还 |

<b>最关键的区别</b>：`XDP_PASS` 发生在<b>同一个 NAPI 周期内</b>，所以 `napi_build_skb()` 之后可以立刻复用 page_pool 的本地缓存、走 per-CPU 快路径；而 `xdp_build_skb_from_frame()` 在远端 CPU 上，必须靠 `mem` 信息判断怎么归还，且用不了本地缓存。

这也是为什么 <b>cpumap 分流虽然能均衡 CPU，但会付出跨 CPU 内存归还的代价</b>——对 HFT，跨核传递往往得不偿失，能用 RSS/flow steering 把包直接送到目标核，就不要用 cpumap 转一手。
</details>

---

→ 前一篇：[03 sk_buff 生命周期](03-sock-sk-buff.md)
→ 本章完，下一章：[chapter-04 page_pool](../../chapter-04-page-pool/)
→ 相关：[chapter-01 hook 点顺序](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) · [chapter-05 XDP 架构](../../chapter-05-xdp-architecture/) · [chapter-06 AF_XDP](../../chapter-06-af-xdp/)
