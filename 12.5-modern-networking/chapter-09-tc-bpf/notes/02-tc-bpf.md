# 02 — tc-BPF：v6.6 的 tcx 双机制、返回码与 `__sk_buff` 可写性

> **来源：** LWN tc-BPF 系列 + **v6.6 源码逐条核对**
> **对应 Rosen:** Ch6（Advanced Routing）/ Ch9（Netfilter）
> **内核版本：** tc-BPF（cls_bpf）4.1+；`direct-action` 4.1+；**tcx 6.6+**

## 文档概述

本篇是 **tc 的 BPF 侧**：程序挂在哪、返回码怎么解释、`__sk_buff` 哪些字段能写、能调哪些 helper。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [01-tc-bootlin.md](01-tc-bootlin.md) | **qdisc**：排队规则、prio/fq/tbf/etf、HFT 出向配置 | 01 讲「什么时候发」，本篇讲「这个类怎么判、要不要丢、要不要改」 |

**v6.6 最重要变化：tc-BPF 有两条挂载路径了。**

```
                    tc-BPF 程序
                         │
        ┌────────────────┴────────────────┐
        │                                 │
   ① legacy（clsact）                ② tcx（6.6+）
   tc filter add ... bpf da           bpftool prog attach ... tcx_ingress
   netlink / tc 命令                  bpf(BPF_LINK_CREATE) / bpf_link 语义
        │                                 │
        └────────────────┬────────────────┘
                         │
              sch_handle_ingress() / sch_handle_egress()
              【tcx 先跑，返回 TC_ACT_UNSPEC 时继续跑 legacy】
```

**这是本篇的核心内容**，也是网上资料几乎全都还没覆盖的部分。

---

## 1. 位置：ingress 在 5412，egress 在 4311

| 方向 | 宿主函数 | 调用点 | 上下文 |
|------|---------|--------|--------|
| **ingress** | `__netif_receive_skb_core()` | `net/core/dev.c:5412` | skb 已分配，**在 L3 之前** |
| **egress** | `__dev_queue_xmit()` | `net/core/dev.c:4311` | skb 完整，**在 qdisc 之前** |

### 1.1 ingress 的完整上下文

```c
	if (skb_skip_tc_classify(skb))
		goto skip_classify;          /* ← 重注入的包跳过整个分类 */

	if (pfmemalloc)
		goto skip_taps;              /* ← 内存紧张时跳过抓包点 */

	list_for_each_entry_rcu(ptype, &ptype_all, list) { ... }        /* 全局抓包点 */
	list_for_each_entry_rcu(ptype, &skb->dev->ptype_all, list) { ...} /* 设备抓包点 */

skip_taps:
#ifdef CONFIG_NET_INGRESS
	if (static_branch_unlikely(&ingress_needed_key)) {
		bool another = false;

		nf_skip_egress(skb, true);
		skb = sch_handle_ingress(skb, &pt_prev, &ret, orig_dev, &another);
		if (another)
			goto another_round;
		...
```

**⭐ 结论：tcpdump 能看到被 tc ingress 丢弃的包，看不到被 XDP 丢弃的包。**

`ptype_all`（AF_PACKET / tcpdump / `libpcap`）在 `dev.c:5394-5403` 遍历，而 `sch_handle_ingress()` 在 `dev.c:5412` —— **抓包点在 tc ingress 之前**。

| hook | tcpdump 能看到被它丢的包吗 | 原因 |
|------|--------------------------|------|
| **XDP** | ❌ **看不到** | XDP 在 `build_skb()` 之前，包还没进 `ptype_all` |
| **tc ingress** | ✅ **能看到** | `ptype_all` 遍历在 `sch_handle_ingress()` 之前 |
| tc egress | ✅ 能看到（走 `dev_queue_xmit_nit`） | — |

**这条差异是调试时的关键判据**：如果你怀疑是 XDP 在丢包，tcpdump 上什么都看不到；如果是 tc ingress 丢的，tcpdump 能看到包进来了但应用收不到。

另外两个细节：

- **`skb_skip_tc_classify()`** → 直接 `goto skip_classify`，**同时跳过抓包点和 tc ingress**。这类包是「不应被重新分类」的（如从 `act_mirred` 重新注入的包）。写 tc 程序时要注意你的规则对这类包不生效。
- **整个 path 由 `static_branch_unlikely(&ingress_needed_key)` 守着**（`CONFIG_NET_INGRESS`），没挂任何 ingress 程序时零开销。

### 1.2 egress 的完整上下文

见 [01](01-tc-bootlin.md) 第 1 节的完整图。要点复述：

```
② if (SKBTX_SCHED_TSTAMP) __skb_tstamp_tx(...)   ← SW 时间戳（tc egress 之前）
④ skb_update_prio(skb)                            ← socket 优先级已写入
⑦ if (egress_needed_key) {
      nf_hook_egress(skb, &rc, dev);              ← ★ Netfilter egress 先跑
      skb = sch_handle_egress(skb, &rc, dev);     ← ★★ tc egress（dev.c:4311）
   }
⑨ txq = netdev_core_pick_tx(...)                  ← 选 TX 队列（tc egress 之后）
⑩ q->enqueue() → q->dequeue()                     ← qdisc 排队（tc egress 之后）
```

**旧笔记说「tc egress 在 qdisc 之后、驱动之前」是错的**——已在 01 篇纠正。

---

## 2. ⭐ v6.6 的双机制：tcx 与 legacy clsact

### 2.1 `sch_handle_ingress()` 的完整实现（`net/core/dev.c:4005`）

```c
sch_handle_ingress(struct sk_buff *skb, struct packet_type **pt_prev, int *ret,
		   struct net_device *orig_dev, bool *another)
{
	struct bpf_mprog_entry *entry = rcu_dereference_bh(skb->dev->tcx_ingress);
	int sch_ret;

	if (!entry)
		return skb;                                    /* ① */
	if (*pt_prev) {
		*ret = deliver_skb(skb, *pt_prev, orig_dev);
		*pt_prev = NULL;
	}

	qdisc_skb_cb(skb)->pkt_len = skb->len;
	tcx_set_ingress(skb, true);

	if (static_branch_unlikely(&tcx_needed_key)) {          /* ② */
		sch_ret = tcx_run(entry, skb, true);
		if (sch_ret != TC_ACT_UNSPEC)
			goto ingress_verdict;
	}
	sch_ret = tc_run(tcx_entry(entry), skb);                /* ③ */
ingress_verdict:
	switch (sch_ret) {
	case TC_ACT_REDIRECT:
		__skb_push(skb, skb->mac_len);
		if (skb_do_redirect(skb) == -EAGAIN) {
			__skb_pull(skb, skb->mac_len);
			*another = true;
			break;
		}
		*ret = NET_RX_SUCCESS;
		return NULL;
	case TC_ACT_SHOT:
		kfree_skb_reason(skb, SKB_DROP_REASON_TC_INGRESS);
		*ret = NET_RX_DROP;
		return NULL;
	/* used by tc_run */
	case TC_ACT_STOLEN:
	case TC_ACT_QUEUED:
	case TC_ACT_TRAP:
		consume_skb(skb);
		fallthrough;
	case TC_ACT_CONSUMED:
		*ret = NET_RX_SUCCESS;
		return NULL;
	}

	return skb;
}
```

**四点解读：**

1. **① `entry` 是 `dev->tcx_ingress`**（`struct bpf_mprog_entry`），它**同时**承载 tcx 程序和 legacy clsact 的 filter 链。只要任一存在，`entry` 就非 NULL。
2. **② tcx 先跑**，由 `tcx_needed_key` 静态分支守着（没有任何 tcx 程序时零开销）。
3. **③ tcx 返回 `TC_ACT_UNSPEC`（-1）时才继续跑 legacy**。也就是说**两条路会串联执行**，不是二选一。
4. **`sch_handle_egress()` 的结构完全相同**（`dev.c:4061`），唯一区别是 `tcx_run(entry, skb, false)` —— `needs_mac = false`。

### 2.2 `needs_mac`：ingress push，egress 不 push

`tcx_run()`（`dev.c:3983-4000`）：

```c
static __always_inline enum tcx_action_base
tcx_run(const struct bpf_mprog_entry *entry, struct sk_buff *skb,
	const bool needs_mac)
{
	const struct bpf_mprog_fp *fp;
	const struct bpf_prog *prog;
	int ret = TCX_NEXT;

	if (needs_mac)
		__skb_push(skb, skb->mac_len);          /* ← ingress 才做 */
	bpf_mprog_foreach_prog(entry, fp, prog) {
		bpf_compute_data_pointers(skb);
		ret = bpf_prog_run(prog, skb);
		if (ret != TCX_NEXT)
			break;
	}
	if (needs_mac)
		__skb_pull(skb, skb->mac_len);
	return tcx_action_code(skb, ret);
}
```

以及 `sch_handle_egress()` 里的注释：

```c
	case TC_ACT_REDIRECT:
		/* No need to push/pop skb's mac_header here on egress! */
		skb_do_redirect(skb);
```

**含义：**

| 方向 | `needs_mac` | 原因 |
|------|------------|------|
| **ingress** | `true` | 到达 `__netif_receive_skb_core` 时 L2 头已被 `__skb_pull()` 掉（skb->data 指向 L3），需要 push 回才能让 BPF 看到以太头 |
| **egress** | `false` | 出向时 L2 头还在（`__dev_queue_xmit` 开头有 `skb_reset_mac_header`），不需要动 |

**写程序时的直接影响**：同一个 BPF 程序**不能不加区分地同时用在 ingress 和 egress**——两侧看到的 `skb->data` 起点不同。

### 2.3 ⭐ tcx 的返回码：`TCX_NEXT` 是链条的「继续」

`enum tcx_action_base`（`include/uapi/linux/bpf.h:6265-6270`）：

```c
enum tcx_action_base {
	TCX_NEXT	= -1,
	TCX_PASS	= 0,
	TCX_DROP	= 2,
	TCX_REDIRECT	= 7,
};
```

**这些值被刻意对齐到 `TC_ACT_*`：**

| tcx | 值 | 对应的 legacy TC_ACT | 值 |
|-----|-----|---------------------|-----|
| `TCX_NEXT` | -1 | `TC_ACT_UNSPEC` | -1 |
| `TCX_PASS` | 0 | `TC_ACT_OK` | 0 |
| `TCX_DROP` | 2 | `TC_ACT_SHOT` | 2 |
| `TCX_REDIRECT` | 7 | `TC_ACT_REDIRECT` | 7 |

所以**同一套返回值在两条路径上都适用**——这是设计上的兼容。

**归一化逻辑**（`include/net/tcx.h`）：

```c
static inline enum tcx_action_base tcx_action_code(struct sk_buff *skb,
						   int code)
{
	switch (code) {
	case TCX_PASS:
		skb->tc_index = qdisc_skb_cb(skb)->tc_classid;
		fallthrough;
	case TCX_DROP:
	case TCX_REDIRECT:
		return code;
	case TCX_NEXT:
	default:
		return TCX_NEXT;
	}
}
```

**⚠️ 一个隐蔽的坑**：`default: return TCX_NEXT`。

`tcx_run()` 里的 `break` 条件是 `ret != TCX_NEXT`（**原始值**），但归一化时只认 `-1/0/2/7`：

| 程序返回值 | 是否 `break` 出链路 | 归一化后 | 最终行为 |
|-----------|------------------|---------|---------|
| `-1`（NEXT） | 否，继续下一个程序 | `TCX_NEXT` | 继续跑后续 tcx 程序 → 全部 NEXT 则落到 legacy |
| `0`（OK） | 是 | `TCX_PASS` | 交付上层 / 入 qdisc |
| `2`（SHOT） | 是 | `TCX_DROP` | 丢包 |
| `7`（REDIRECT） | 是 | `TCX_REDIRECT` | 重定向 |
| **其他任何值**（如 `TC_ACT_STOLEN=4`、`TC_ACT_QUEUED=5`、`TC_ACT_TRAP=8`） | **是**（因为 ≠ -1） | **`TCX_NEXT`** | ⚠️ **终止 tcx 链条，但因为归一化成 -1 = `TC_ACT_UNSPEC`，`sch_handle_*` 不会 `goto verdict`，而是继续跑 legacy `tc_run()`** |

**实践建议：在 tcx 程序里只返回 `-1 / 0 / 2 / 7` 这四个值。** 返回其他值的行为是「跳过剩余 tcx 程序 + 落到 legacy」，几乎肯定不是你想要的。

> **对比 XDP**：XDP 里 `XDP_ABORTED == 0`，「忘记 return」= 丢包。
> **tcx 里相反**：默认返回 0 = `TCX_PASS` = **放行**。所以 tcx 的「忘 return」不会造成静默丢包，但会造成**静默放行**（程序等于没生效）。

### 2.4 tcx vs legacy：怎么选

| 维度 | legacy clsact（`tc filter add ... bpf da`） | **tcx（`bpftool prog attach ... tcx_*`）** |
|------|-------------------------------------------|------------------------------------------|
| 挂载方式 | netlink + `tc` 命令 | `bpf(BPF_LINK_CREATE)` / bpftool |
| 生命周期 | 需显式 `tc filter del`；`tc qdisc del` 会一次全清 | **bpf_link**：进程退出自动 detach；可 pin 到 bpffs |
| 可见性 | `tc filter show` | `bpftool link show dev eth0` |
| 多程序 | 一个 filter 链，按 prio 排序 | **bpf_mprog 数组**，按 attach 顺序，`BPF_F_BEFORE/AFTER/ID` 可插队 |
| 原子替换 | 删除 + 添加（有窗口期） | `BPF_F_REPLACE` 原子替换 |
| 需要 clsact qdisc | **是** | **否** |
| 与 legacy 共存 | — | **串联**：tcx 先、legacy 后 |

**推荐**：新项目用 tcx（生命周期可控、可插队、可原子替换）。存量系统继续用 legacy，两者会正确串联。

```bash
# tcx 挂载（v6.6+）
bpftool prog load tc_prog.bpf.o /sys/fs/bpf/tc_prog
bpftool prog attach id <PROG_ID> tcx_ingress dev eth0
bpftool prog attach id <PROG_ID> tcx_egress  dev eth0

# 查看
bpftool link show dev eth0
bpftool net show

# 插队（在已有程序前后）
bpftool prog attach id <NEW> tcx_ingress dev eth0 link_before <EXISTING_LINK_ID>
```

---

## 3. 返回码：legacy 的完整语义

`include/uapi/linux/pkt_cls.h:63-79`：

```c
#define TC_ACT_UNSPEC	(-1)
#define TC_ACT_OK		0
#define TC_ACT_RECLASSIFY	1
#define TC_ACT_SHOT		2
#define TC_ACT_PIPE		3
#define TC_ACT_STOLEN		4
#define TC_ACT_QUEUED		5
#define TC_ACT_REPEAT		6
#define TC_ACT_REDIRECT		7
#define TC_ACT_TRAP		8
#define TC_ACT_VALUE_MAX	TC_ACT_TRAP
```

**ingress 侧的处理**（`sch_handle_ingress()` 的 `switch`）：

| 返回码 | 行为 |
|--------|------|
| `TC_ACT_REDIRECT`(7) | `__skb_push(skb, mac_len)` → `skb_do_redirect()`；返回 `-EAGAIN` 时 `*another = true` 重新走一轮 |
| `TC_ACT_SHOT`(2) | `kfree_skb_reason(skb, SKB_DROP_REASON_TC_INGRESS)` |
| `TC_ACT_STOLEN`(4) / `TC_ACT_QUEUED`(5) / `TC_ACT_TRAP`(8) | `consume_skb(skb)`（**计入已接收，不算丢包**） |
| `TC_ACT_CONSUMED` | 直接 `*ret = NET_RX_SUCCESS` |
| 其他（`OK`/`UNSPEC`/`RECLASSIFY`/`PIPE`/`REPEAT`） | **继续正常收包路径** |

**egress 侧的处理**（`sch_handle_egress()`）——几乎一致，但注释指出 egress 不需要 push/pop mac header。

### 3.1 `da`（direct-action）到底是什么

```bash
tc filter add dev eth0 ingress bpf da obj p.bpf.o sec rx
#                            ^^
```

**不加 `da` 时**：cls_bpf 的返回值被解释成「要执行的 action 编号」（`res.classid` 之类），filter 命中后还要再走一遍 tc 的 action 链。

**加 `da` 时**：BPF 程序的返回值**直接**就是 filter 的 verdict（`TC_ACT_OK` / `TC_ACT_SHOT` / `TC_ACT_REDIRECT` …），**跳过整个 action 链**。

| | 不加 `da` | 加 `da` |
|---|----------|--------|
| 返回值含义 | action 索引/编号 | **verdict 本身** |
| 是否走 action 链 | 是（额外一次间接跳转） | 否 |
| 典型用途 | 需要 `action mirred`/`action skbedit` 串联 | **几乎所有现代用法** |

> **不加 `da` 是最隐蔽的 bug 来源之一**：你返回 `TC_ACT_OK`(0)，结果被当成「执行第 0 号 action」，行为完全不可预测。**任何时候都用 `da`**。

### 3.2 丢包的可见性

`kfree_skb_reason(skb, SKB_DROP_REASON_TC_INGRESS / TC_EGRESS)` —— v6.6 的 tc 丢包带 **drop reason**，可以精确观测：

```bash
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
#   关注 SKB_DROP_REASON_TC_INGRESS / TC_EGRESS

# 或者用 tc 自带的统计
tc -s filter show dev eth0 ingress
```

---

## 4. `__sk_buff`：哪些字段能写

`struct __sk_buff`（`include/uapi/linux/bpf.h:6074`）共 30 个具名字段（`cb[5]` 和 `remote_ip6[4]`/`local_ip6[4]` 是数组）：

```c
struct __sk_buff {
	__u32 len;
	__u32 pkt_type;
	__u32 mark;
	__u32 queue_mapping;
	__u32 protocol;
	__u32 vlan_present;
	__u32 vlan_tci;
	__u32 vlan_proto;
	__u32 priority;
	__u32 ingress_ifindex;
	__u32 ifindex;
	__u32 tc_index;
	__u32 cb[5];
	__u32 hash;
	__u32 tc_classid;
	__u32 data;
	__u32 data_end;
	__u32 napi_id;

	/* Accessed by BPF_PROG_TYPE_sk_skb types from here to ... */
	__u32 family;
	__u32 remote_ip4;	/* Stored in network byte order */
	__u32 local_ip4;	/* Stored in network byte order */
	__u32 remote_ip6[4];	/* Stored in network byte order */
	__u32 local_ip6[4];	/* Stored in network byte order */
	__u32 remote_port;	/* Stored in network byte order */
	__u32 local_port;	/* stored in host byte order */
	/* ... here. */

	__u32 data_meta;
	__bpf_md_ptr(struct bpf_flow_keys *, flow_keys);
	__u64 tstamp;
	__u32 wire_len;
	__u32 gso_segs;
	__bpf_md_ptr(struct bpf_sock *, sk);
	__u32 gso_size;
	__u8  tstamp_type;
	__u32 :24;		/* Padding, future use. */
	__u64 hwtstamp;
};
```

### 4.1 可写性规则（`bpf_skb_is_valid_access()`，`net/core/filter.c:8413`）

**默认规则**：`default` 分支只在 `type == BPF_WRITE && size != 4` 时拒绝 → **所有 4 字节字段都可写**。

**例外（写被拒绝 / 完全禁止）：**

| 字段 | 读 | 写 | 依据 |
|------|----|----|------|
| 所有 4 字节字段（`mark`、`priority`、`tc_index`、`cb[0..4]`、`hash`、`tc_classid`、`queue_mapping`、`pkt_type`、`protocol`、`vlan_tci`、`ingress_ifindex`、`ifindex`、`napi_id`、`gso_segs`、`gso_size`、`wire_len`、`family`、`remote_ip4`、`local_ip4`、`remote_ip6[4]`、`local_ip6[4]`、`remote_port`、`local_port`） | ✅ | ✅ | `default` 分支 |
| `tstamp` | ✅ | ✅（**必须 8 字节访存**） | `if (size != sizeof(__u64)) return false;` |
| `hwtstamp` | ✅（8 字节） | ❌ | `if (type == BPF_WRITE \|\| size != sizeof(__u64)) return false;` |
| `sk` | ✅（8 字节，得到 `PTR_TO_SOCK_COMMON_OR_NULL`） | ❌ | `if (type == BPF_WRITE \|\| size != sizeof(__u64)) return false;` |
| `flow_keys` | ❌ | ❌ | `return false;` |
| `tstamp_type` | ❌ | ❌ | `return false;` |
| `tstamp_type` 与 `hwtstamp` 之间的 padding | ❌ | ❌ | 「Explicitly prohibit access to padding」 |
| `data` / `data_end` / `data_meta` | ✅ | ❌（只能通过 helper） | `if (size != size_default) return false;`（读） |

**HFT 最关心的三个：**

| 字段 | 用途 | 字节序陷阱 |
|------|------|-----------|
| `mark` | 配合策略路由 / nftables / tc filter 做分流 | — |
| `priority` | 决定 `prio` qdisc 的 band；**注意 `skb_update_prio()` 已在 tc egress 前跑过** | — |
| `tstamp` | ⭐ **读：RX 硬件时间戳；写：配合 `bpf_skb_set_tstamp()` 设 TX 时间戳** | ns |
| `hwtstamp` | ⭐ **只读**：网卡硬件时间戳（ns） | ns |

> **⚠️ `local_port` 是主机字节序，`remote_port` 是网络字节序。** 这个不对称是历史遗留，源码注释里明确写了：`__u32 remote_port; /* Stored in network byte order */` vs `__u32 local_port; /* stored in host byte order */`。做 map key 时要分别处理。

### 4.2 ⭐ 对比：`SOCKET_FILTER` 程序的可写范围小得多

`sk_filter_is_valid_access()`（`net/core/filter.c:8478`）：

```c
static bool sk_filter_is_valid_access(int off, int size,
				      enum bpf_access_type type,
				      const struct bpf_prog *prog,
				      struct bpf_insn_access_aux *info)
{
	switch (off) {
	case bpf_ctx_range(struct __sk_buff, tc_classid):
	case bpf_ctx_range(struct __sk_buff, data):
	case bpf_ctx_range(struct __sk_buff, data_meta):
	case bpf_ctx_range(struct __sk_buff, data_end):
	case bpf_ctx_range_till(struct __sk_buff, family, local_port):
	case bpf_ctx_range(struct __sk_buff, tstamp):
	case bpf_ctx_range(struct __sk_buff, wire_len):
	case bpf_ctx_range(struct __sk_buff, hwtstamp):
		return false;                    /* ← 完全禁止访问 */
	}

	if (type == BPF_WRITE) {
		switch (off) {
		case bpf_ctx_range_till(struct __sk_buff, cb[0], cb[4]):
			break;                       /* ← 只有 cb[5] 可写 */
		default:
			return false;
		}
	}
	...
```

| 程序类型 | 可写字段 | 禁访字段 |
|---------|---------|---------|
| **tc-BPF**（`SCHED_CLS`） | 所有 4 字节字段 + `tstamp` | `hwtstamp`、`sk`、`flow_keys`、`tstamp_type`、padding |
| **`SOCKET_FILTER`**（`SO_ATTACH_BPF`） | **只有 `cb[0..4]`** | `tc_classid`、`data`、`data_meta`、`data_end`、`family..local_port`、`tstamp`、`wire_len`、`hwtstamp` |

**这是「tc-BPF 比 socket filter 强在哪」的源码答案**：`SO_ATTACH_BPF` 的程序连包内容都读不到（禁访 `data`/`data_end`），**只能做 accept/reject 决策**；tc-BPF 能读包、能改包、能改元数据。

---

## 5. Helper 能力集：81 个 vs XDP 的 23 个

`tc_cls_act_func_proto()`（`net/core/filter.c:7968`）有 **81 个 `case BPF_FUNC_*` 分支**（含 `#ifdef` 项），是网络类程序里能力集最大的。

**XDP 有而 tc 没有的**：无（XDP 的 23 个是 tc 的子集加上 XDP 专属的 `xdp_adjust_*` / `xdp_load_bytes` / `xdp_store_bytes` / `xdp_get_buff_len`）。

**tc 有而 XDP 没有的关键能力：**

| 类别 | helper | 为什么 XDP 没有 |
|------|--------|----------------|
| 邻居转发 | `redirect_neigh`、`redirect_peer` | 需要 `struct neighbour` / `dst_entry`，XDP 层还没路由 |
| 克隆转发 | `clone_redirect` | 需要 skb 引用计数 |
| 包长度调整 | `skb_change_tail`、`skb_adjust_room`、`skb_change_head` | 需要 skb 线性/非线性区管理 |
| 校验和增量写回 | `l3_csum_replace`、`l4_csum_replace`、`csum_update`、`csum_level` | XDP 只有 `csum_diff`（算差值） |
| VLAN | `skb_vlan_push`、`skb_vlan_pop` | skb 元数据 |
| socket 绑定 | `sk_assign` | 需要 socket 查找后的绑定点 |
| 隧道 | `skb_get/set_tunnel_key`、`skb_get/set_tunnel_opt` | skb metadata |
| hash | `set_hash`、`set_hash_invalid`、`get_hash_recalc` | skb 元数据 |
| 时间戳 | **`skb_set_tstamp`** | XDP 时还没有 skb |
| GRO/拉取线性 | `skb_pull_data` | XDP 无需（本来就线性） |

> ⭐ **`bpf_skb_set_tstamp()` 是 tc-BPF 独有的延迟测量能力**：配合 `SO_TIMESTAMPING` 可以在内核里直接给包打时间戳，不需要用户态往返。

---

## 6. 完整示例：行情流打标记 + 延迟测量

```c
/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <bpf/bpf_helpers.h>

struct {
        __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
        __uint(max_entries, 4);
        __type(key, __u32);
        __type(value, __u64);
} stats SEC(".maps");

enum { S_TOTAL = 0, S_MARKET = 1, S_OTHER = 2 };

static __always_inline void bump(__u32 k)
{
        __u64 *v = bpf_map_lookup_elem(&stats, &k);
        if (v) *v += 1;
}

SEC("tc-ingress")
int tc_ingress(struct __sk_buff *skb)
{
        void *data     = (void *)(long)skb->data;
        void *data_end = (void *)(long)skb->data_end;

        bump(S_TOTAL);

        /* ⚠️ tc ingress 的 data 指向【L3】——mac header 已被 pull 掉。
         * tcx_run() 里有 __skb_push(skb, skb->mac_len)，
         * 但那是在 bpf_prog_run 之前 push、之后 pull，
         * 程序内 ctx->data 仍是 L3 起点。
         * 所以这里从 iphdr 开始解析。 */
        struct iphdr *ip = data;
        if ((void *)(ip + 1) > data_end)
                return TC_ACT_OK;              /* 不是 IP，放行 */
        if (ip->ihl < 5)
                return TC_ACT_SHOT;

        if (ip->protocol != IPPROTO_UDP)
                return TC_ACT_OK;

        struct udphdr *udp = (void *)ip + (ip->ihl * 4);
        if ((void *)(udp + 1) > data_end)
                return TC_ACT_SHOT;

        if (udp->dest == bpf_htons(9090)) {
                skb->mark = 0x9090;            /* 行情流标记 → 策略路由/nft 用 */
                skb->priority = 1;             /* → prio qdisc 的 band 0 */
                bump(S_MARKET);
                return TC_ACT_OK;
        }

        bump(S_OTHER);
        return TC_ACT_OK;
}

char _license[] SEC("license") = "GPL";
```

**配套的 tc 配置：**

```bash
DEV=eth0

# 1. 挂 clsact（提供 ingress + egress 两个 block，且不排队）
tc qdisc add dev $DEV clsact

# 2. 挂 legacy tc-BPF（da 模式）
tc filter add dev $DEV ingress bpf da obj tc_prog.bpf.o sec tc-ingress

# 3. 出向用 prio + tbf 做分流与限速（BPF 只打标记，整形交给 qdisc）
tc qdisc add dev $DEV root handle 1: prio
tc qdisc add dev $DEV parent 1:1 handle 10: fq flow_limit 100
tc qdisc add dev $DEV parent 1:3 handle 30: tbf rate 50mbit burst 32kb latency 20ms

# 4. 观测
tc -s filter show dev $DEV ingress
bpftool map dump name stats
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
```

**⚠️ 一个必须验证的假设**：上面代码假设 `skb->data` 在 tc ingress 时指向 L3（IP 头）。这在**标准路径**成立（`__netif_receive_skb_core` 前 `__skb_pull` 掉了 mac header，tcx 的 `needs_mac` 只在 `bpf_prog_run` 前后临时 push/pull）。

但**不要依赖记忆**——用下面的代码实测确认：

```c
        /* 调试期：打印 data 处的前 2 字节，判断是不是 L3 */
        if ((void *)data + 2 <= data_end) {
                __u8 b0 = *(__u8 *)data;
                bpf_printk("first byte = 0x%02x\n", b0);
                /* 0x45 = IPv4 (version 4, ihl 5) → data 指向 L3
                 * 0x?? 且 eth->h_proto 可见 → data 指向 L2 */
        }
```

---

## 7. HFT 要点

1. **v6.6 起 tc-BPF 有两条挂载路径，且会串联执行**（tcx 先、legacy 后）。新建用 tcx（bpf_link 语义、可插队、可原子替换、无需 clsact qdisc），存量继续用 legacy。混用时必须清楚顺序。
2. **tcx 只认 `-1/0/2/7` 四个返回值**，其他值被归一化成 `TCX_NEXT`，行为是「终止 tcx 链 + 落到 legacy」。**这不是你想要的**。
3. **tcx 的「忘 return」= 放行**（返回 0 = `TCX_PASS`），与 XDP 的「忘 return = 丢包」（`XDP_ABORTED == 0`）**完全相反**。两边的安全默认值不同，跨类型搬代码时要特别注意。
4. ⭐ **tcpdump 能看到被 tc ingress 丢弃的包，看不到被 XDP 丢弃的**。这是调试时区分「XDP 丢的」还是「tc 丢的」的第一判据。
5. **tc egress 在 qdisc 之前**——BPF 只能分类+丢包，**整形必须靠 qdisc**（见 [01](01-tc-bootlin.md)）。
6. **`skb->priority` 在 tc egress 之前已被 `skb_update_prio()` 设定**（从 socket 的 `SO_PRIORITY`）。在 tc egress 里覆盖它会破坏上层意图，谨慎。
7. **`local_port` 是主机字节序、`remote_port` 是网络字节序**——源码注释明确的不对称，做 map key 时必须分别处理。
8. **`hwtstamp` 只读且必须 8 字节访存**；`tstamp` 可读写（8 字节）。配合 `bpf_skb_set_tstamp()` 做内核态打点。
9. **`SO_ATTACH_BPF` 的 socket filter 连包内容都读不到**（禁访 `data`/`data_end`），只能 accept/reject。要做内容过滤必须用 tc-BPF 或 XDP。

---

## 8. 与 Rosen Ch6/Ch9 的差异

| 维度 | Rosen 3.x | v6.6 |
|------|-----------|------|
| 分类器 | `u32` / `fw` / `route`（classic BPF） | **`cls_bpf`（eBPF）为主** + v6.6 的 **tcx** |
| 出向 filter | 无（`ingress` qdisc 只有 RX） | **`clsact` 提供 ingress+egress**；tcx 直接 `tcx_egress` |
| 程序能力 | classic BPF 只读包、返回长度 | 81 个 helper，能改包、改元数据、重定向、打时间戳 |
| 挂载生命周期 | `tc filter add/del` | tcx 走 **bpf_link**（进程退出自动 detach） |
| 丢包可见性 | 只能数 `tc -s` 的 drop | **`SKB_DROP_REASON_TC_INGRESS/EGRESS` + `tracepoint:skb:kfree_skb`** |
| 与 Netfilter 关系 | tc 是独立子系统 | **Netfilter egress 在 tc egress 之前**（`dev.c:4306` vs `:4311`） |

---

## 9. 代码自测

<details>
<summary>Q1：你写了一个 tcx 程序，想让「非目标端口的包直接丢弃」，代码是 <code>if (port != 9090) return TC_ACT_STOLEN;</code>。上线后发现：目标包正常，非目标包<strong>也被正常交付了</strong>，而 <code>TC_ACT_SHOT</code> 换成 2 之后就对了。为什么 <code>TC_ACT_STOLEN</code>(4) 不工作？</summary>

**因为 tcx 的返回值归一化只认四个值，`TC_ACT_STOLEN`(4) 被当成了 `TCX_NEXT`。**

看 `include/net/tcx.h` 的归一化函数：

```c
static inline enum tcx_action_base tcx_action_code(struct sk_buff *skb,
						   int code)
{
	switch (code) {
	case TCX_PASS:
		skb->tc_index = qdisc_skb_cb(skb)->tc_classid;
		fallthrough;
	case TCX_DROP:
	case TCX_REDIRECT:
		return code;
	case TCX_NEXT:
	default:
		return TCX_NEXT;          /* ← 4 落到这里 */
	}
}
```

`enum tcx_action_base`（`include/uapi/linux/bpf.h:6265-6270`）只有四个值：

```c
enum tcx_action_base {
	TCX_NEXT	= -1,
	TCX_PASS	= 0,
	TCX_DROP	= 2,
	TCX_REDIRECT	= 7,
};
```

**完整的执行链条：**

```c
/* dev.c:3983 tcx_run() */
bpf_mprog_foreach_prog(entry, fp, prog) {
	bpf_compute_data_pointers(skb);
	ret = bpf_prog_run(prog, skb);
	if (ret != TCX_NEXT)          /* 原始值 4 != -1 → break */
		break;
}
...
return tcx_action_code(skb, ret);   /* 4 → default → TCX_NEXT(-1) */

/* dev.c:4023 sch_handle_ingress() */
if (static_branch_unlikely(&tcx_needed_key)) {
	sch_ret = tcx_run(entry, skb, true);
	if (sch_ret != TC_ACT_UNSPEC)    /* TCX_NEXT(-1) == TC_ACT_UNSPEC(-1) → 不 goto */
		goto ingress_verdict;
}
sch_ret = tc_run(tcx_entry(entry), skb);   /* ← 继续跑 legacy filter 链 */
```

**所以 `TC_ACT_STOLEN`(4) 的实际行为是：**

1. **终止 tcx 程序链**（因为原始值 4 ≠ `TCX_NEXT`，`break` 了）——后面的 tcx 程序不会跑；
2. **归一化成 `TCX_NEXT`(-1)**；
3. `sch_handle_ingress()` 看到 -1 = `TC_ACT_UNSPEC`，**不进 verdict 分支**，继续跑 legacy `tc_run()`；
4. legacy 链上没有 filter 的话，`tc_run` 返回 `TC_ACT_UNSPEC`，`switch` 落到 default → **正常交付**。

**最终：包被正常交付。** 这正是你观察到的现象。

**修法：只用 `-1 / 0 / 2 / 7`。**

```c
if (port != 9090)
        return TCX_DROP;        /* 或 return 2，或直接 return TC_ACT_SHOT */
```

**为什么设计成这样？** `TCX_NEXT`(-1) 与 legacy 的 `TC_ACT_UNSPEC`(-1) 同值，是为了让「tcx 没有最终裁决」和「legacy 没有 filter 命中」这两种情况汇合到同一条路径：**继续往下走**。而 `TC_ACT_STOLEN`/`QUEUED`/`TRAP` 这些 legacy 的复杂语义在 tcx 里没有对应物，于是被保守地归一化成「继续」。

**对比记忆（很重要）：**

| hook | 「忘 return」的默认值 | 后果 |
|------|---------------------|------|
| **XDP** | 0 = `XDP_ABORTED` | **丢包**（静默，tcpdump 看不到） |
| **tcx** | 0 = `TCX_PASS` | **放行**（程序等于没生效） |

两个方向相反，跨类型搬代码时必须检查。

</details>

<details>
<summary>Q2：你想确认「包到底是被 XDP 丢的还是被 tc ingress 丢的」。tcpdump 抓到了包，说明什么？抓不到又说明什么？</summary>

**这是本篇最实用的一条判据。**

关键点在 `ptype_all`（AF_PACKET，tcpdump/libpcap 的抓包点）与两个 hook 的**相对位置**：

```c
/* net/core/dev.c:5394-5403，__netif_receive_skb_core() 内 */
	list_for_each_entry_rcu(ptype, &ptype_all, list) {          /* 全局抓包点 */
		if (pt_prev)
			ret = deliver_skb(skb, pt_prev, orig_dev);
		pt_prev = ptype;
	}

	list_for_each_entry_rcu(ptype, &skb->dev->ptype_all, list) { /* 设备抓包点 */
		if (pt_prev)
			ret = deliver_skb(skb, pt_prev, orig_dev);
		pt_prev = ptype;
	}

skip_taps:
#ifdef CONFIG_NET_INGRESS
	if (static_branch_unlikely(&ingress_needed_key)) {
		skb = sch_handle_ingress(skb, &pt_prev, &ret, orig_dev, &another);
		/*                        ↑ dev.c:5412，tc ingress 在抓包点【之后】 */
```

| 结果 | 说明 |
|------|------|
| **tcpdump 抓到了包** | 包至少走到了 `ptype_all`。**排除 XDP 丢弃**（XDP 在 `build_skb()` 之前，包还没成型，更到不了 `ptype_all`）。<br>丢包发生在 tc ingress 或之后。 |
| **tcpdump 抓不到包** | 两种可能：<br>① **被 XDP 丢/重定向**了（最常见）<br>② 被 tc ingress 之前的其他机制丢（极少见） |

**所以完整的排查流程：**

```bash
# 1. tcpdump 能看到 → 不是 XDP 丢的
tcpdump -i eth0 -nn 'udp port 9090'

# 2. 看 tc ingress 的 filter 命中与统计
tc -s filter show dev eth0 ingress

# 3. 看 tc 丢包的 drop reason（v6.6 有）
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
#   关注 SKB_DROP_REASON_TC_INGRESS

# 4. tcpdump 看不到 → 查 XDP
bpftool net show
xdp-loader status eth0
bpftrace -e 'tracepoint:xdp:xdp_exception    { @[args->act] = count(); }'
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'
```

**背后的原理（一句话）：**

```
驱动 NAPI poll
   ↓
【XDP】bpf_prog_run_xdp()          ← tcpdump 看不到
   ↓ XDP_PASS
build_skb() / napi_gro_receive()
   ↓
__netif_receive_skb_core()
   ↓
ptype_all（tcpdump）                ← 从这里开始 tcpdump 可见
   ↓
【tc ingress】sch_handle_ingress()  ← tcpdump 能看到
   ↓
ip_rcv() → ...
```

**注意一个例外**：`skb_skip_tc_classify(skb)` 为真时会 `goto skip_classify`，**同时跳过抓包点和 tc ingress**。这类包（如 `act_mirred` 重新注入的）tcpdump 也看不到，且 tc ingress 不生效。

**egress 方向的对应**：出向抓包走 `dev_queue_xmit_nit()`，位置在 qdisc 之后、`ndo_start_xmit()` 之前——**tc egress 丢的包 tcpdump 也看不到**（因为 tc egress 在 qdisc 之前，包还没到 `dev_queue_xmit_nit`）。

</details>

<details>
<summary>Q3：你在 tc-BPF 程序里写 <code>skb->mark = 0x9090;</code> 成功了，但同一份代码改成 <code>SO_ATTACH_BPF</code> 的 socket filter 之后，verifier 报 <code>invalid bpf_context access</code>。为什么 tc 能写，socket filter 不能？</summary>

**因为两种程序类型的 `__sk_buff` 访问校验函数不同，socket filter 的严格得多。**

**tc-BPF 用的是 `bpf_skb_is_valid_access()`**（`net/core/filter.c:8413`）：

```c
	switch (off) {
	...
	case bpf_ctx_range(struct __sk_buff, hwtstamp):
		if (type == BPF_WRITE || size != sizeof(__u64))
			return false;                 /* hwtstamp 只读 */
		break;
	case bpf_ctx_range(struct __sk_buff, tstamp):
		if (size != sizeof(__u64))
			return false;                 /* tstamp 可读写，但要 8 字节 */
		break;
	case offsetof(struct __sk_buff, sk):
		if (type == BPF_WRITE || size != sizeof(__u64))
			return false;                 /* sk 只读 */
		break;
	...
	default:
		/* Only narrow read access allowed for now. */
		if (type == BPF_WRITE) {
			if (size != size_default)     /* 写必须是 4 字节 */
				return false;
		} else {
			...
		}
	}

	return true;
```

→ **`default` 分支允许所有 4 字节字段写入**，所以 `mark`、`priority`、`tc_index`、`cb[5]`、`hash`、`tc_classid` 等全部可写。

**socket filter 用的是 `sk_filter_is_valid_access()`**（`net/core/filter.c:8478`），它**先做减法再委托**：

```c
static bool sk_filter_is_valid_access(int off, int size,
				      enum bpf_access_type type,
				      const struct bpf_prog *prog,
				      struct bpf_insn_access_aux *info)
{
	switch (off) {
	case bpf_ctx_range(struct __sk_buff, tc_classid):
	case bpf_ctx_range(struct __sk_buff, data):
	case bpf_ctx_range(struct __sk_buff, data_meta):
	case bpf_ctx_range(struct __sk_buff, data_end):
	case bpf_ctx_range_till(struct __sk_buff, family, local_port):
	case bpf_ctx_range(struct __sk_buff, tstamp):
	case bpf_ctx_range(struct __sk_buff, wire_len):
	case bpf_ctx_range(struct __sk_buff, hwtstamp):
		return false;                     /* ← 完全禁止访问 */
	}

	if (type == BPF_WRITE) {
		switch (off) {
		case bpf_ctx_range_till(struct __sk_buff, cb[0], cb[4]):
			break;                        /* ← 只有 cb[5] 可写 */
		default:
			return false;                 /* ← 其他一律不可写 */
		}
	}

	return bpf_skb_is_valid_access(off, size, type, prog, info);
}
```

**对比表：**

| 能力 | tc-BPF（`SCHED_CLS`） | `SOCKET_FILTER` |
|------|---------------------|-----------------|
| 读包内容（`data`/`data_end`） | ✅ | ❌ **被禁** |
| 写 `mark` / `priority` / `hash` / `tc_index` | ✅ | ❌ |
| 写 `cb[0..4]` | ✅ | ✅ **（唯一可写的）** |
| 读 `remote_ip4` / `local_port` 等 | ✅ | ❌ |
| 读 `tstamp` / `hwtstamp` | ✅ / ✅ | ❌ / ❌ |
| helper 数量 | **81** | **5** |

**这个限制不是随意的，而是由位置决定的：**

`SO_ATTACH_BPF` 的程序运行在 `sk_filter_trim_cap()` 里（`net/core/filter.c:124`），位置是 socket 接收队列的入口。设计上它被定位为**「这个包要不要交给这个 socket」的决策器**——只需要看包内容做判断（实际上连内容都读不了，只能看 `len`/`protocol` 这类元数据），不需要修改任何东西。

而 tc-BPF 运行在 `__netif_receive_skb_core()` / `__dev_queue_xmit()`，**是数据路径的中途站**，需要完整的读写能力来做分类、改写、重定向。

**实际影响：**

- 想做「按端口过滤」→ 不能用 socket filter（读不到 `data`）。需要用 **tc-BPF** 或 **XDP**。
- 想在 socket 上做一个「只接受某类包」的兜底 → socket filter 够用，但它只能 accept/reject，且拿到的信息很有限。
- 想在 socket filter 里「给包打标记供后续使用」→ **只能写 `cb[5]`**（5 个 32 位字），且这个 cb 在包离开这个 socket 的路径后就可能失效。

**修法**：把程序类型从 `BPF_PROG_TYPE_SOCKET_FILTER` 改成 `BPF_PROG_TYPE_SCHED_CLS`，挂到 tc 上。

</details>

---

## 导航

- **上一篇：** [01-tc-bootlin.md](01-tc-bootlin.md) — qdisc 体系与 HFT 出向调度
- **相关：** [chapter-08-ebpf-cgroup-bpf/](../../chapter-08-ebpf-cgroup-bpf/) eBPF 框架与 verifier · [chapter-05-xdp-architecture/](../../chapter-05-xdp-architecture/) XDP（tcpdump 看不到的那个 hook）· [chapter-03-tx-path-skbbuff/](../../chapter-03-tx-path-skbbuff/) 发包路径
- **章节主页：** [README](../README.md)
