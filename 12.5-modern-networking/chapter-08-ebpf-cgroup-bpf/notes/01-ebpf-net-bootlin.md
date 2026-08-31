# 01 — eBPF 网络：hook 全景、程序类型与工具链实操

> **来源：** Bootlin（eBPF Networking 课程模块）+ Linux v6.6 内核源码核对
> **对应 Rosen：** 无（Rosen 3.x 时代只有 classic BPF，即 `struct sock_fprog` / `sk_filter`，没有 eBPF）
> **内核版本：** 全部结论基于 **v6.6** 源码核对，行号引用 v6.6

## 文档概述

本篇回答一个问题：**eBPF 在 Linux 网络栈里到底能挂在哪里，每个挂载点用什么工具挂、挂上去之后怎么验证它真的生效了。**

这是「全景 + 实操」篇。姊妹篇的分工：

| 文件 | 主题 | 本篇与它的关系 |
|------|------|---------------|
| [02-bpf.md](02-bpf.md) | 程序类型 / map 类型 / verifier 限制 | 本篇给出「类型 × 挂载点 × 工具」的横表，02 给出每种类型的 helper 能力集与 verifier 约束 |
| [03-xdp-bpf.md](03-xdp-bpf.md) | XDP 程序能做什么、返回码语义 | 本篇只定位 XDP 在栈中最早的位置，03 展开 XDP 本身 |
| [04-cgroup-bpf.md](04-cgroup-bpf.md) | cgroup 系列程序的 attach 语义、继承与叠加 | 本篇指出 cgroup hook 的真实调用点（在 socket 入队 / IP finish 处），04 展开 attach 规则 |

**本篇的阅读顺序建议：** 先看第 2 节的 hook 全景图建立位置感 → 第 3 节逐点核对源码位置（本篇价值最高的部分，有 3 处会颠覆常见认知）→ 第 4 节按需求挑程序类型 → 第 5–7 节上手工具链与观测。

---

## 1. 先建立位置感：五个 hook 点的相对顺序

先看一张图（**RX 路径自上而下，TX 路径自下而上**）。图中每一行的行号都是 v6.6 源码实测位置，不是记忆：

```
                        ┌─────────────────────────────────────────┐
                        │  网卡硬件 / DMA ring                     │
                        └────────────────┬────────────────────────┘
                                         │
  ┌──────────────────────────────────────▼──────────────────────────────────┐
  │ ① XDP（驱动 NAPI poll 内，skb 之前）                                      │
  │    bpf_prog_run_xdp()          include/net/xdp.h:482                    │
  │    上下文：struct xdp_md（也就是内核侧的 struct xdp_buff）                │
  │    程序类型：BPF_PROG_TYPE_XDP                                          │
  │    挂载工具：xdp-loader / ip link set dev X xdp obj ...                  │
  └──────────────────────────────────────┬──────────────────────────────────┘
                                         │ XDP_PASS
                    ┌────────────────────▼────────────────────┐
                    │  build_skb() / napi_gro_receive()       │
                    │  → 分配 sk_buff                          │
                    └────────────────────┬────────────────────┘
                                         │
  ┌──────────────────────────────────────▼──────────────────────────────────┐
  │ ② tc ingress（__netif_receive_skb_core 内，L3 之前）                     │
  │    sch_handle_ingress()        net/core/dev.c:5412（函数体 4005）        │
  │    上下文：struct __sk_buff                                              │
  │    程序类型：BPF_PROG_TYPE_SCHED_CLS                                     │
  │    挂载工具：tc filter add ... ingress bpf da obj ...                    │
  │    注：v6.6 起入口变成「tcx 优先，legacy clsact 兜底」                    │
  └──────────────────────────────────────┬──────────────────────────────────┘
                                         │
            ip_rcv() → NF_HOOK(PRE_ROUTING) → 路由查找 → ip_local_deliver()
                                         │
  ┌──────────────────────────────────────▼──────────────────────────────────┐
  │ ③ cgroup ingress（socket 入队时， skb 已找到目标 socket）                │
  │    BPF_CGROUP_RUN_PROG_INET_INGRESS()   net/core/filter.c:138           │
  │    宿主函数：sk_filter_trim_cap()                                        │
  │    上下文：struct __sk_buff                                              │
  │    程序类型：BPF_PROG_TYPE_CGROUP_SKB                                    │
  │    挂载工具：bpftool cgroup attach <CG> ingress id <PROG>                │
  └──────────────────────────────────────┬──────────────────────────────────┘
                                         │
                       sk_filter（SO_ATTACH_BPF）→ 应用 recv()


  ───────────────────────── TX 方向（自下往上读）───────────────────────────

                       应用 send()/sendmsg()
                                         │
            tcp_sendmsg() → ip_queue_xmit() → 路由查找
                    → NF_HOOK(LOCAL_OUT) → ip_output()
                                         │
                    NF_HOOK(POST_ROUTING)  ─── okfn 回调 ───┐
                                                            │
  ┌─────────────────────────────────────────────────────────▼───────────────┐
  │ ④ cgroup egress（Netfilter POST_ROUTING **之后**）                       │
  │    单播：ip_finish_output()        net/ipv4/ip_output.c:314/318         │
  │    组播：ip_mc_finish_output()     net/ipv4/ip_output.c:330/337         │
  │    IPv6：ip6_finish_output()       net/ipv6/ip6_output.c:203            │
  │    程序类型：BPF_PROG_TYPE_CGROUP_SKB                                    │
  └──────────────────────────────────────┬──────────────────────────────────┘
                                         │
                    邻居解析 → __dev_queue_xmit()
                                         │
  ┌──────────────────────────────────────▼──────────────────────────────────┐
  │ ⑤ tc egress（__dev_queue_xmit 内，qdisc 之前）                           │
  │    sch_handle_egress()          net/core/dev.c:4311（函数体 4061）       │
  │    程序类型：BPF_PROG_TYPE_SCHED_CLS                                     │
  └──────────────────────────────────────┬──────────────────────────────────┘
                                         │
                        qdisc → ndo_start_xmit() → 网卡
```

**这张图里最容易被记错的三处**（下面第 3 节逐条用源码钉死）：

1. cgroup **ingress** 不在 L3 交付处，而是在 **socket 入队** 时；
2. cgroup **egress** 不在 POST_ROUTING 之前，而在 **POST_ROUTING 之后**；
3. tc ingress/egress 在 v6.6 已经分裂成 **tcx（link-based）** 与 **legacy clsact** 两套机制，且 tcx 优先。

---

## 2. 逐点核对：源码里的真实位置

### 2.1 XDP —— 最早点，且没有 sk_buff

```c
/* include/net/xdp.h:482 */
static __always_inline u32 bpf_prog_run_xdp(const struct bpf_prog *prog,
                                            struct xdp_buff *xdp)
```

调用者是各网卡驱动的 NAPI poll 循环（`driver → xdp_do_redirect` 之前的 `bpf_prog_run_xdp`）。此时：

- **还没有 `sk_buff`**，`xdp_buff` 只是 `{data, data_end, data_meta, data_hard_start, rxq}` 几个指针加一个长度。
- 返回码是 `enum xdp_action`（`include/uapi/linux/bpf.h:6283`）：

```c
enum xdp_action {
	XDP_ABORTED = 0,   /* 注意：是 0 */
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};
```

> **⚠️ `XDP_ABORTED == 0` 是本篇第一个必须记住的陷阱。** 任何「忘记 return」或「返回值来自一个出错的 helper 调用」的 XDP 程序，返回值默认就是 0 = `XDP_ABORTED` = 丢包并触发 tracepoint。详见 [03-xdp-bpf.md](03-xdp-bpf.md) 对 `bpf_redirect_map()` flags 低位的分析。

- **观测盲区**：XDP 在 `ptype_all`（AF_PACKET / tcpdump 的抓包点，`net/core/dev.c` 的 `dev_queue_xmit_nit` 与 `__netif_receive_skb_core` 的 `ptype_all` 遍历）**之前**，所以 **tcpdump 看不到被 XDP 丢弃的包**；它也不计入 `ethtool -S` 的 rx 计数（驱动统计在 XDP 判定之后才更新）。

### 2.2 tc ingress / egress —— v6.6 的 tcx 与 legacy 双机制

`sch_handle_ingress()`（`net/core/dev.c:4005`）的 v6.6 实现：

```c
sch_handle_ingress(struct sk_buff *skb, struct packet_type **pt_prev, int *ret,
		   struct net_device *orig_dev, bool *another)
{
	struct bpf_mprog_entry *entry = rcu_dereference_bh(skb->dev->tcx_ingress);
	int sch_ret;

	if (!entry)
		return skb;
	...
	if (static_branch_unlikely(&tcx_needed_key)) {
		sch_ret = tcx_run(entry, skb, true);
		if (sch_ret != TC_ACT_UNSPEC)
			goto ingress_verdict;
	}
	sch_ret = tc_run(tcx_entry(entry), skb);
ingress_verdict:
	switch (sch_ret) {
	case TC_ACT_REDIRECT:
		__skb_push(skb, skb->mac_len);
		if (skb_do_redirect(skb) == -EAGAIN) { ... }
		...
	case TC_ACT_SHOT:
		kfree_skb_reason(skb, SKB_DROP_REASON_TC_INGRESS);
		...
	}
	return skb;
}
```

从这段源码能读出四点：

1. **两份入口共存**：`skb->dev->tcx_ingress` 这个 `bpf_mprog_entry` 同时承载 tcx 程序和 legacy clsact 的 filter 链。`tcx_run()` 跑 tcx 程序，**返回 `TC_ACT_UNSPEC` 时继续跑 `tc_run()`（legacy）**。也就是说 legacy 不会被 tcx 取代掉，两者会串联执行。
2. **`static_branch_unlikely(&tcx_needed_key)`**：没有任何 tcx 程序时零开销，不会走到 `tcx_run()`。
3. **`tcx` 是 v6.6 新增的 attach 类型**：`BPF_TCX_INGRESS` / `BPF_TCX_EGRESS`（`include/uapi/linux/bpf.h:1040-1041`），通过 `bpf(BPF_LINK_CREATE)` 挂，走 bpf_link 语义（可以 pin、可以 `bpftool link show`、进程退出自动 detach）。legacy clsact 是 `netlink` + tc 命令挂的。
4. **丢包有 drop reason**：`SKB_DROP_REASON_TC_INGRESS`，可用 `dropreason` 工具或 `perf` 的 `skb:kfree_skb` tracepoint 看到，比 legacy 时代只能数 `tc -s filter show` 的 drop 计数要精确。

**egress 同理**：`sch_handle_egress()`（`dev.c:4061`）在 `__dev_queue_xmit()` 的 `dev.c:4311` 被调用，位置在 **qdisc enqueue 之前**。

> 这带来一个 HFT 相关的细节：tc egress 在 qdisc 之前，所以**绕过了 qdisc 排队**，但也意味着你在 tc egress 里看到的包还没经过任何流量整形；想做「出方向限速」必须在 tc egress 之后配合 qdisc，或者直接用 `TC_ACT_STOLEN` + 自己排。

### 2.3 cgroup ingress —— 在 socket 入队时，不在 L3

这是本篇**最反直觉**的一处。搜索 `BPF_CGROUP_RUN_PROG_INET_INGRESS` 在 v6.6 的调用点，只有一处：

```c
/* net/core/filter.c:124 */
int sk_filter_trim_cap(struct sock *sk, struct sk_buff *skb, unsigned int cap)
{
	int err;
	struct sk_filter *filter;

	if (skb_pfmemalloc(skb) && !sock_flag(sk, SOCK_MEMALLOC)) {
		NET_INC_STATS(sock_net(sk), LINUX_MIB_PFMEMALLOCDROP);
		return -ENOMEM;
	}
	err = BPF_CGROUP_RUN_PROG_INET_INGRESS(sk, skb);   /* ← net/core/filter.c:138 */
	if (err)
		return err;

	err = security_sock_rcv_skb(sk, skb);
	...
	filter = rcu_dereference(sk->sk_filter);   /* SO_ATTACH_BPF 的程序 */
```

**含义拆解：**

| 常见误解 | 实际情况（v6.6 源码） |
|---------|----------------------|
| cgroup ingress 在包进入协议栈时就跑 | ❌ 它在 `sk_filter_trim_cap()` 里，该函数由 `sock_queue_rcv_skb()` → `sk_filter()` 调用 |
| 它对设备上的所有包生效 | ❌ 只对**已经完成 socket 查找、确定要交给某个 socket 的包**生效；被路由转发、被丢、没找到 socket 的包根本不会经过 |
| 它在 `sk_filter`（`SO_ATTACH_BPF`）之后 | ❌ 它在 **`security_sock_rcv_skb()` 之前、`sk_filter` 之前**——比 socket 自己的 BPF 还早 |
| 它在 Netfilter 之前 | ✅ 对 RX 而言是的（PRE_ROUTING → 路由 → local_deliver → UDP/TCP → `sock_queue_rcv_skb` → 这里） |

**这直接决定了 cgroup ingress 的适用边界：**

- ✅ 适合：按进程/容器做「这个 cgroup 的进程能不能收到这类包」的准入控制、按 cgroup 记账。
- ❌ 不适合：**任何想在包刚进机器时就做的过滤**。例如丢弃某个 DDoS 源、丢弃非目标组播组——这些包到达 cgroup ingress 时已经走完了路由、UDP/TCP 栈、socket 查找，该付的 CPU 都付了。这类需求要用 XDP 或 tc ingress。
- ❌ 对**被转发的包、组播未加入组的包、没有 socket 的包完全不生效**。

### 2.4 cgroup egress —— 在 Netfilter POST_ROUTING 之后

`BPF_CGROUP_RUN_PROG_INET_EGRESS` 的调用点：

| 路径 | 函数 | 位置 |
|------|------|------|
| IPv4 单播 | `ip_finish_output()` | `net/ipv4/ip_output.c:314`（宏在 318） |
| IPv4 组播 / 广播 | `ip_mc_finish_output()` | `net/ipv4/ip_output.c:330`（宏在 337） |
| IPv6 | `ip6_finish_output()` | `net/ipv6/ip6_output.c:203` |

而这三个函数都是 **`NF_HOOK_COND(NFPROTO_IPV4, NF_INET_POST_ROUTING, ..., okfn)` 的 okfn**。以 `ip_output()` 为例（`net/ipv4/ip_output.c:424`）：

```c
int ip_output(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	struct net_device *dev = skb_dst(skb)->dev, *indev = skb->dev;

	skb->dev = dev;
	skb->protocol = htons(ETH_P_IP);

	return NF_HOOK_COND(NFPROTO_IPV4, NF_INET_POST_ROUTING,
			    net, sk, skb, indev, dev,
			    ip_finish_output,                      /* ← okfn */
			    !(IPCB(skb)->flags & IPSKB_REROUTED));
}
```

**结论：cgroup egress 在 Netfilter POST_ROUTING hook 链的「通过了所有 hook 之后」才执行**。所以：

- iptables/nft 的 POST_ROUTING（含 SNAT/MASQUERADE）**先跑**，cgroup egress 看到的是 SNAT **之后**的地址。
- 返回码语义也不是 XDP 那套，而是 `NET_XMIT_SUCCESS` / `NET_XMIT_CN`，其余一律丢：

```c
static int ip_finish_output(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	int ret;

	ret = BPF_CGROUP_RUN_PROG_INET_EGRESS(sk, skb);
	switch (ret) {
	case NET_XMIT_SUCCESS:
		return __ip_finish_output(net, sk, skb);
	case NET_XMIT_CN:
		return __ip_finish_output(net, sk, skb) ? : ret;
	default:
		kfree_skb_reason(skb, SKB_DROP_REASON_BPF_CGROUP_EGRESS);
		return ret;
	}
}
```

> **HFT 相关性**：组播走的是 `ip_mc_finish_output()` 这条**独立**路径（`net/ipv4/ip_output.c:337`），和单播不是一个函数。写 cgroup egress 做「限制交易进程出向带宽」时，如果只测单播没测组播，很容易漏掉组播路径上的行为差异（例如组播的 loopback 副本是通过 `ip_mc_output()` 里 `skb_clone()` + 独立 `NF_HOOK(..., ip_mc_finish_output)` 发出去的，见 `ip_output.c:396-400`）。

---

## 3. 程序类型 × 挂载点 × 工具 横表

v6.6 的 `enum bpf_prog_type` 共 **33 个成员**（`include/uapi/linux/bpf.h:958-990`，从 `BPF_PROG_TYPE_UNSPEC` 到 `BPF_PROG_TYPE_NETFILTER`）。其中与网络直接相关的主要如下：

| 程序类型 | 挂载点（attach type） | 上下文类型 | 挂载工具 | 返回码语义 |
|---------|---------------------|-----------|---------|-----------|
| `BPF_PROG_TYPE_XDP` | 设备 XDP hook（`ip link` / xdp-loader） | `struct xdp_md` | `ip link set dev X xdp obj`、`xdp-loader load` | `enum xdp_action` |
| `BPF_PROG_TYPE_SCHED_CLS` | `BPF_TCX_INGRESS`/`EGRESS`（v6.6+）、legacy clsact | `struct __sk_buff` | `bpftool prog attach`（tcx）、`tc filter add ... bpf da` | `TC_ACT_*` |
| `BPF_PROG_TYPE_SCHED_ACT` | tc action（已基本被 `da` 模式取代） | `struct __sk_buff` | `tc actions add bpf` | `TC_ACT_*` |
| `BPF_PROG_TYPE_SOCKET_FILTER` | `setsockopt(SO_ATTACH_BPF)` | `struct __sk_buff` | 应用代码 / `bpftool prog attach fd` | 接受的字节数（0 = 丢弃） |
| `BPF_PROG_TYPE_CGROUP_SKB` | `BPF_CGROUP_INET_INGRESS`/`EGRESS` | `struct __sk_buff` | `bpftool cgroup attach` | ingress: 0/1；egress: `NET_XMIT_*` |
| `BPF_PROG_TYPE_CGROUP_SOCK` | `BPF_CGROUP_INET_SOCK_CREATE`/`..._SOCK_RELEASE` | `struct bpf_sock` | `bpftool cgroup attach` | 0/1 |
| `BPF_PROG_TYPE_CGROUP_SOCK_ADDR` | `BPF_CGROUP_INET4/6_{BIND,CONNECT,POST_BIND}`, `UDP4/6_{SENDMSG,RECVMSG}`, `GET{SOCK,PEER}NAME` | `struct bpf_sock_addr` | `bpftool cgroup attach` | 0/1 |
| `BPF_PROG_TYPE_CGROUP_SOCKOPT` | `BPF_CGROUP_{GET,SET}SOCKOPT` | `struct bpf_sockopt` | `bpftool cgroup attach` | `0=忽略 / 1=接管 / 2=不再调后续` |
| `BPF_PROG_TYPE_CGROUP_SYSCTL` | `BPF_CGROUP_SYSCTL` | `struct bpf_sysctl` | `bpftool cgroup attach` | `0=拒绝 / 1=放行 / 2=继续调` |
| `BPF_PROG_TYPE_SOCK_OPS` | `BPF_CGROUP_SOCK_OPS` | `struct bpf_sock_ops` | `bpftool cgroup attach` | 依事件而定（见 [04](04-cgroup-bpf.md)） |
| `BPF_PROG_TYPE_SK_SKB` | `BPF_SK_SKB_STREAM_PARSER` / `..._STREAM_VERDICT` | `struct __sk_buff` | `bpf(BPF_PROG_ATTACH)` + sockmap | `SK_PASS` / `SK_DROP` |
| `BPF_PROG_TYPE_SK_MSG` | `BPF_SK_MSG_VERDICT` | `struct sk_msg_md` | `bpf(BPF_PROG_ATTACH)` + sockmap | `SK_PASS` / `SK_DROP` |
| `BPF_PROG_TYPE_SK_REUSEPORT` | `BPF_SK_REUSEPORT_SELECT` / `..._SELECT_OR_MIGRATE` | `struct sk_reuseport_md` | `setsockopt(SO_ATTACH_REUSEPORT_EBPF)` | 索引（选中的 socket 下标） |
| `BPF_PROG_TYPE_SK_LOOKUP` | `BPF_SK_LOOKUP` | `struct bpf_sk_lookup` | `bpftool prog attach`（netns） | `SK_DROP` / `SK_PASS` |
| `BPF_PROG_TYPE_FLOW_DISSECTOR` | netns flow dissector | `struct __sk_buff` | `bpf(BPF_PROG_ATTACH)` | `BPF_FLOW_DISSECTOR_CONT` / `..._RET` |
| `BPF_PROG_TYPE_LWT_{IN,OUT,XMIT}` | 路由的 light-weight tunnel | `struct __sk_buff` | `ip route add ... encap bpf` | `BPF_LWT_REROUTE` 等 |
| `BPF_PROG_TYPE_NETFILTER` | `BPF_NETFILTER`（v6.4+） | `struct bpf_nf_ctx` | `bpftool prog attach` | `NF_ACCEPT` / `NF_DROP` |

### 3.1 各类型可用 helper 数量（v6.6 实测）

helper 能力集决定了一个程序到底能干什么。以下是 `net/core/filter.c` 中 `get_func_proto` 各实现里 `case BPF_FUNC_*` 的分支数：

| 分发函数 | 位置 | `case` 分支数 | 兜底 |
|---------|------|--------------|------|
| `xdp_func_proto()` | `net/core/filter.c:8084` | **23** | `bpf_sk_base_func_proto()` |
| `tc_cls_act_func_proto()` | `net/core/filter.c:7968` | **81**（含 `#ifdef` 项，其中 `CONFIG_INET` 项最多） | `bpf_sk_base_func_proto()` |
| `sk_msg_func_proto()` | `net/core/filter.c:8226` | 14 | `bpf_sk_base_func_proto()` |
| `sock_ops_func_proto()` | `net/core/filter.c:8178` | 14 | `bpf_sk_base_func_proto()` |
| `cg_skb_func_proto()` | `net/core/filter.c:7919` | 15 + `cgroup_common_func_proto()` | `sk_filter_func_proto()` |
| `sk_filter_func_proto()` | `net/core/filter.c:7897` | **5** | `bpf_sk_base_func_proto()` |

**这张表说明的问题：**

1. **XDP 的 helper 集合（23）远小于 tc（81）**。XDP 拿不到 `bpf_skb_*` 系列（因为根本没有 skb），也拿不到 `bpf_redirect_neigh` / `bpf_clone_redirect` / `bpf_sk_assign` 这类依赖 skb 语义的东西。想用 `bpf_redirect_neigh()` 做「改邻居后转发」，只有 tc 能做到。
2. `sk_filter_func_proto()` 只有 5 个 helper（`skb_load_bytes`、`skb_load_bytes_relative`、`get_socket_cookie`、`get_socket_uid`、`perf_event_output`），全部程序类型的能力集最终都会兜底到 `bpf_sk_base_func_proto()`——后者提供 map 操作、`bpf_ktime_get_ns()`、`bpf_trace_printk()`、`bpf_probe_read_*` 等通用能力。
3. `cg_skb_func_proto()` 的兜底是 `sk_filter_func_proto()`，所以 **cgroup SKB 程序天然拥有 socket filter 的 5 个 helper**，再加 15 个专属的（`sk_fullsock`、`sk_storage_get/delete`、`skb_cgroup_id`、`sk_lookup_tcp/udp`、`tcp_sock` 等）。

---

## 4. 工具链：三条路线，各有适用场景

### 4.1 路线 A：libbpf + CO-RE（生产首选）

```bash
# 1. 生成 vmlinux.h（一次性，每个内核版本一份）
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 2. 编译（clang 需要 -g -O2 才能生成 BTF）
clang -g -O2 -target bpf -D__TARGET_ARCH_x86_64 \
      -I/path/to/libbpf/src -c prog.bpf.c -o prog.bpf.o

# 3. 生成 skeleton（把 load/attach/detach 封装成 C 结构体）
bpftool gen skeleton prog.bpf.o > prog.skel.h

# 4. 用户态程序里用 skeleton
#    prog_bpf__open() → prog_bpf__load() → prog_bpf__attach() → ... → prog_bpf__destroy()
```

**CO-RE（Compile Once – Run Everywhere）解决什么**：同一份 `.bpf.o` 在不同内核版本上跑，内核结构体的字段偏移不同。CO-RE 靠 BTF 在**加载时**重定位 `bpf_core_read()` 的偏移，不需要目标机器上有内核头文件，也不需要 clang 现场编译。

**常见报错**：`invalid bpf_context access` / `R1 invalid mem access 'scalar'`
→ 多半是 CO-RE 重定位失败，或访问的字段在该内核的 BTF 里不存在。用 `bpftool btf dump file /sys/kernel/btf/vmlinux format c` 检查目标内核是否真有该字段。

### 4.2 路线 B：xdp-tools / tc（运维最快）

```bash
# XDP
xdp-loader load -m native  eth0 xdp_prog.bpf.o    # native = 驱动层（零拷贝能力）
xdp-loader load -m skb     eth0 xdp_prog.bpf.o    # generic = 通用模式（性能降级）
xdp-loader status eth0                             # ⭐ 一定看 mode，确认没降级
xdp-loader unload -i <id> eth0

# tc（legacy clsact）
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj tc_prog.o sec ingress
tc filter show dev eth0 ingress                    # 带统计
tc -s filter show dev eth0 ingress                 # 带 packets/bytes/drops
tc qdisc del dev eth0 clsact

# v6.6+ tcx（link-based，推荐新项目用）
bpftool prog load tc_prog.bpf.o /sys/fs/bpf/tc_prog
bpftool prog attach id <PROG_ID> tcx_ingress dev eth0
bpftool link show dev eth0
```

> **`da`（direct-action）的含义**：legacy clsact 的 filter 命中后可以再跟一串 action。加 `da` 后，BPF 程序的返回值**直接**就是 filter 的 verdict（`TC_ACT_OK`/`TC_ACT_SHOT`/`TC_ACT_REDIRECT`…），不再走 action 链。省一次间接跳转，几乎所有现代 tc-BPF 都用 `da`。不加 `da` 时返回值会被解释成「要执行的 action 编号」——这是个隐蔽的坑。

### 4.3 路线 C：bpftool（观测 + 挂载通用入口）

```bash
bpftool prog show                  # 所有已加载程序（type / id / tag / jited）
bpftool prog show id <ID> --pretty # 详情
bpftool prog dump xlated id <ID>   # 反汇编（verifier 改写后的指令）
bpftool prog dump jited  id <ID>   # JIT 之后的机器码 ⭐ 性能分析用
bpftool map  show / dump id <ID>
bpftool link show
bpftool net  show                  # ⭐ 一眼看清 XDP / tc / flow_dissector 挂在哪
bpftool feature probe              # 内核支持哪些 prog/map 类型、哪些 helper
```

`bpftool net show` 的输出形如：

```
xdp:
eth0(2) driver id 42

tc:
eth0(2) clsact/ingress tc_prog.bpf.o:[ingress] id 43
eth0(2) clsact/egress  tc_prog.bbf.o:[egress]  id 44

flow_dissector:
```

**这就是「确认程序真的挂上了」的第一步，也是最容易被跳过的一步。**

---

## 5. 观测：四层递进，从粗到细

| 层次 | 手段 | 能看到什么 | 代价 |
|------|------|-----------|------|
| L1 是否挂载 | `bpftool net show`、`xdp-loader status`、`tc filter show` | 程序在不在、挂在哪个设备/方向 | 无 |
| L2 聚合计数 | `tc -s filter show`、`bpftool prog show`（`run_cnt`）、`bpftool map dump` | 处理了多少包、丢了多少、map 里存了什么 | 需要开 `kernel.bpf_stats_enabled` |
| L3 逐事件 | `bpf_printk()` + `cat /sys/kernel/debug/tracing/trace_pipe`；或 BPF ringbuf / perfbuf 自定义事件 | 单包级别的字段值 | 高（trace_pipe 全局共享、ringbuf 有每事件开销） |
| L4 内核 tracepoint | `bpftrace -e 'tracepoint:xdp:* { @[probe] = count(); }'`，`xdp_redirect` / `xdp_redirect_err` / `xdp_exception`、`skb:kfree_skb`（带 reason） | 内核侧的丢包原因 | 中 |

### 5.1 ⭐ `run_time_ns` / `run_cnt`：单次执行成本的免费测量

这是本篇认为**最有价值、最少被使用**的观测手段。

`struct bpf_prog_info`（`include/uapi/linux/bpf.h:6396`）里有这两个字段（`:6430-6431`）：

```c
	__u64 run_time_ns;
	__u64 run_cnt;
```

它们由 `bpftool prog show` 显示（`kernel/bpf/syscall.c:2233-2234`），取值为 `info.run_time_ns = stats.nsecs; info.run_cnt = stats.cnt;`（`kernel/bpf/syscall.c:4323-4324`）。

**但默认是关的**，因为埋点有成本。`include/linux/filter.h:592` 的 `__bpf_prog_run()`：

```c
static __always_inline u32 __bpf_prog_run(const struct bpf_prog *prog,
					  const void *ctx,
					  bpf_dispatcher_fn dfunc)
{
	u32 ret;

	cant_migrate();
	if (static_branch_unlikely(&bpf_stats_enabled_key)) {
		struct bpf_prog_stats *stats;
		u64 start = sched_clock();
		unsigned long flags;

		ret = dfunc(ctx, prog->insnsi, prog->bpf_func);
		stats = this_cpu_ptr(prog->stats);
		flags = u64_stats_update_begin_irqsave(&stats->syncp);
		u64_stats_inc(&stats->cnt);
		u64_stats_add(&stats->nsecs, sched_clock() - start);
		u64_stats_update_end_irqrestore(&stats->syncp, flags);
	} else {
		ret = dfunc(ctx, prog->insnsi, prog->bpf_func);
	}
	return ret;
}
```

**打开方式：**

```bash
sysctl -w kernel.bpf_stats_enabled=1      # 0=关，1=开（只有 BPF_STATS_RUN_TIME=0 这个值）
bpftool prog show id <ID>                 # 现在有 run_cnt / run_time_ns
sysctl -w kernel.bpf_stats_enabled=0      # 测完记得关
```

**三个必须知道的细节：**

1. **`kernel.bpf_stats_enabled` 是 sysctl（`kernel/bpf/syscall.c:5686`），不是 sysfs 文件**，mode `0644`，handler 是 `bpf_stats_handler`，合法取值 `SYSCTL_ZERO`～`SYSCTL_TWO`。通过 `bpf(BPF_ENABLE_STATS)` 系统调用开也行（`kernel/bpf/syscall.c:5208`，需要 `CAP_SYS_ADMIN`，只接受 `BPF_STATS_RUN_TIME`）。
2. **每次执行多两次 `sched_clock()` 调用 + 一次 per-CPU u64_stats 更新**。在 XDP 这种每包几十纳秒的场景下，这个开销可能比程序本身还大。**所以测量结果是「带埋点的成本」，不是真实成本；测完一定要关掉。**
3. **`stats` 是 per-CPU 的**（`this_cpu_ptr(prog->stats)`），bpftool 显示的是所有 CPU 的累加。如果你把程序只挂在一个队列上，看到的是那个队列所在 CPU 的统计——这反而是个好处，可以做 per-queue 的分解。

**用法示例：**

```bash
sysctl -w kernel.bpf_stats_enabled=1
# 记下 run_cnt0 / run_time_ns0
bpftool -j prog show id 42 | jq '.run_cnt, .run_time_ns'
sleep 10
# 再读一次，做差
bpftool -j prog show id 42 | jq '.run_cnt, .run_time_ns'
# 平均每包 ns = (ns1 - ns0) / (cnt1 - cnt0)
sysctl -w kernel.bpf_stats_enabled=0
```

> **HFT 视角**：这是唯一能在**不改动程序、不引入第二套时钟**的前提下拿到「内核里单次 BPF 执行耗时」的手段。做行情过滤程序的成本归因时，先跑这个拿到基线，再决定要不要上更重的测量（硬件时间戳 / 用户态打点）。

### 5.2 L4 tracepoint：XDP 丢包的唯一可见处

```bash
# XDP 相关的 tracepoint
perf list | grep xdp
bpftrace -l 'tracepoint:xdp:*'

# 实时看 XDP 异常（XDP_ABORTED 等）
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'

# 看 redirect 失败（★ bpf_redirect_map flags 写错时，这里才会报）
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'

# 看 skb 丢弃原因（cgroup BPF 丢包也能在这看到 reason）
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
```

`xdp_redirect_err` 的 `err` 值：`-EINVAL`（map/索引非法）、`-ENOSPC`（AF_XDP rx ring 满或包超长）、`-EOPNOTSUPP`（目标设备不支持）、`-EBUSY` 等。

---

## 6. 常见故障对照表

| 现象 | 根因 | 定位命令 |
|------|------|---------|
| 程序加载成功但完全没被调用 | 挂到了错误的设备 / 方向；或 XDP 挂在了 `offload` 模式但硬件不支持 | `bpftool net show` |
| XDP 程序「生效了」但性能没提升 | 实际上是 **generic 模式**（`-m skb`），每包额外一次 skb 分配 | `xdp-loader status eth0` 看 mode |
| 挂了 XDP 之后 SSH 断了 / ARP 不通 | `bpf_redirect_map()` 的 flags 低位写了 0 → 未命中时返回 `XDP_ABORTED`（=0）→ 静默丢弃 | `tracepoint:xdp:xdp_redirect_err`；tcpdump **看不到** |
| `libbpf: prog 'xdp': failed to load: -EINVAL` + `invalid bpf_context access` | CO-RE 重定位失败或访问了 BTF 中不存在的字段 | `bpftool btf dump file /sys/kernel/btf/vmlinux` 检查字段 |
| `R2 offset is outside of the packet` | 边界检查写法不对（verifier 要求先判 `data_end` 再解引用） | `bpftool prog dump xlated id <ID>` 看 verifier 的改写 |
| tc filter 加不上去：`Cannot find device "clsact"` | 忘了先 `tc qdisc add dev eth0 clsact` | `tc qdisc show dev eth0` |
| tc-BPF 返回值行为诡异 | 没加 `da`，返回值被当 action 编号解释 | 加 `da` |
| cgroup ingress 对某些包完全不生效 | 那些包没有对应 socket（转发 / 未加入的组播 / 无监听）→ 走不到 `sk_filter_trim_cap()` | 用 `tracepoint:skb:kfree_skb` 或改用 tc ingress |
| `bpftool prog show` 里 `run_cnt` 一直是 0 | 没开 `kernel.bpf_stats_enabled` | `sysctl -w kernel.bpf_stats_enabled=1` |
| 程序里 `bpf_printk()` 什么都没输出 | 需要 `cat /sys/kernel/debug/tracing/trace_pipe`（且它会被其他 tracer 抢） | 改用 ringbuf |

---

## 7. HFT 要点

1. **选 hook 就是选「能为这个包付多少 CPU」。** 想丢弃的包越早丢越划算：XDP（驱动层，无 skb）< tc ingress（有 skb，L3 之前）< cgroup ingress（走完整个协议栈 + socket 查找）。**「过滤非目标组播流」必须放在 XDP 或 tc ingress，放 cgroup ingress 等于什么都没省。**
2. **XDP 的观测盲区要先建立信任补偿。** tcpdump 与 `ethtool -S` 都看不到 XDP 之后的包，调试期必须靠 `tracepoint:xdp:*` + 程序自己的计数器 map。否则你会遇到「程序明明是对的，但看不到任何现象」的困境。
3. **`run_cnt`/`run_time_ns` 是成本归因的第一站**，但记住它自带 `sched_clock()` ×2 的埋点开销，测完要关。
4. **v6.6 起 tc 有 tcx 和 legacy 两条路**，新项目用 tcx（bpf_link 语义，生命周期可控，进程崩了自动摘）；legacy clsact 仍然有效且会与 tcx 串联执行。混用时要清楚执行顺序是 **tcx 先、legacy 后**。
5. **组播 egress 走独立代码路径**（`ip_mc_finish_output()`）。任何在 cgroup egress 上做的策略，单播和组播都要各测一遍。
6. **cgroup BPF 是「治理工具」不是「性能工具」。** 它慢（在 socket 入队处）、粒度是进程组，价值在于把交易进程和行情进程隔离开、按 cgroup 记账、用 `sock_ops` 采 RTT 而不用改应用代码——这些是运维收益，不是延迟收益。
7. **`bpf_redirect_map()` 的 flags 永远显式写 `XDP_PASS`**，不要写 0。`XDP_ABORTED == 0` 意味着写 0 的代价是静默丢掉所有未命中的包（ARP、ICMP、SSH 全中招），而且 tcpdump 和 ethtool 都发现不了。

---

## 8. 与 Rosen（《Linux Kernel Networking》，Rosen 3.x）的差异

| 维度 | Rosen 3.x 的世界 | 现在（v6.6） |
|------|-----------------|-------------|
| BPF 形态 | classic BPF（`struct sock_fprog`），只有 `SO_ATTACH_BPF` 和 tcpdump 过滤器 | eBPF，33 种程序类型、独立指令集、JIT |
| 程序能力 | 只能读包、返回接受长度 | 可改包头、可重定向、可写 map、可调用 20–80 个 helper |
| 挂载点 | 只有 socket filter | XDP / tc / cgroup / sockmap / sk_lookup / netfilter / lwt |
| 校验 | 无（classic BPF 只检查 jump 范围） | verifier 做全路径静态分析（DAG、无回边、类型追踪、边界检查） |
| 与 Netfilter 关系 | Netfilter 是唯一可编程点 | XDP/tc 在 Netfilter 之前；v6.4+ 甚至有 `BPF_PROG_TYPE_NETFILTER` 直接挂进 NF hook |
| 观测 | `iptables -L -nvx` | `bpftool` + `bpf_stats_enabled` + `tracepoint:xdp:*` + drop reason |

Rosen 那套知识仍然解释了协议栈的主体流程（路由、neighbour、qdisc、socket 层），**本篇的所有 hook 位置都是在它的骨架上「插桩」**。两本书不冲突，是叠加关系。

---

## 9. 代码自测

<details>
<summary>Q1：你把 XDP 程序挂在 eth0 上，它返回一个新的组播过滤逻辑。加载成功后，你用 <code>tcpdump -i eth0</code> 抓包，发现目标组播包仍然能看到——这说明 XDP 程序没生效吗？</summary>

**不一定，而且多半恰恰相反：程序生效了。**

原因在位置。`tcpdump` 用的是 AF_PACKET，它在 `__netif_receive_skb_core()` 里通过 `ptype_all` 链表抓包（`dev.c` 中 `__netif_receive_skb_core` 会先遍历 `ptype_all`）。而 XDP 在驱动 NAPI poll 里执行，**在 `build_skb()` 之前**，更在 `ptype_all` 之前。

所以：

- 返回 `XDP_PASS` 的包 → 走到 `ptype_all` → **tcpdump 能看到** ✅（这正是你观察到的）
- 返回 `XDP_DROP` / `XDP_REDIRECT` 的包 → 从来没变成 skb → **tcpdump 看不到** ❌

**正确的验证方式：**

```bash
# 1. 确认挂上了
bpftool net show
xdp-loader status eth0          # 顺带确认 mode 是 native 不是 skb

# 2. 程序里维护一个 map 计数器，用 bpftool 读
bpftool map dump name my_stats

# 3. 内核侧丢包原因
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'
```

**不要**用 tcpdump 的「看不看得见」来判断 XDP 是否生效——它只能证明「包没被 XDP 丢」。

</details>

<details>
<summary>Q2：你写了一个 cgroup ingress 程序，想丢弃「非目标组播组」的包来省 CPU。测试发现丢是丢了，但 CPU 占用几乎没降。为什么？</summary>

**因为 cgroup ingress 跑在 `sk_filter_trim_cap()`（<code>net/core/filter.c:138</code>），位置是 socket 入队时。** 到这一步，包已经完成了：

```
NAPI poll → build_skb → tc ingress → ip_rcv → PRE_ROUTING → 路由
→ ip_local_deliver → NF_LOCAL_IN → udp_rcv → __udp4_lib_mcast_deliver / 单播查找
→ sock_queue_rcv_skb → sk_filter() → sk_filter_trim_cap() → 【你的程序在这里】
```

真正贵的部分（GRO、路由查找、Netfilter、UDP 栈、组播 socket 组遍历、socket 查找）**全都在你的程序之前**。丢在这里只省掉了最后的拷贝到用户态，以及——

更糟的是：**如果你的进程没有加入那个组播组，那个包根本走不到你的 cgroup 程序。** 内核在 `__udp4_lib_mcast_deliver()` 里发现没有 socket 要它，就直接 `kfree_skb()` 了。所以你「成功丢弃」的那些包，只可能是**已经确定要交给某个 socket** 的包——也就是说程序实际上没起到「过滤无关流量」的作用。

**正确做法：把过滤下沉到 XDP 或 tc ingress。**

| 方案 | 位置 | 省掉什么 |
|------|------|---------|
| XDP | 驱动层 | skb 分配 + GRO + 路由 + Netfilter + UDP 栈 + socket 查找（**几乎全部**） |
| tc ingress | `__netif_receive_skb_core` 内 | 路由 + Netfilter + UDP 栈 + socket 查找（**仍然要付 skb 分配**） |
| cgroup ingress | `sk_filter_trim_cap` | 只有用户态拷贝 |

顺带一个验证技巧：对比 `cat /proc/net/softnet_stat` 第二列（`time_squeeze`）和 `ethtool -S eth0 | grep rx_`，下沉前后应有明显差异；cgroup 方案不会有任何变化。

</details>

<details>
<summary>Q3：你开了 <code>kernel.bpf_stats_enabled=1</code> 测 XDP 程序，得到「平均 340 ns/包」。关掉 sysctl 之后用外部打点测，只有 90 ns。这个差距合理吗？为什么？</summary>

**合理，而且 340 ns 里的大部分很可能就是埋点本身的成本。**

看 `include/linux/filter.h:592` 的 `__bpf_prog_run()`：开启统计后，每次执行都要额外做

```c
u64 start = sched_clock();                                  /* 1 */
ret = dfunc(ctx, prog->insnsi, prog->bpf_func);
stats = this_cpu_ptr(prog->stats);
flags = u64_stats_update_begin_irqsave(&stats->syncp);      /* 2：关中断 + seqcount */
u64_stats_inc(&stats->cnt);
u64_stats_add(&stats->nsecs, sched_clock() - start);        /* 3 */
u64_stats_update_end_irqrestore(&stats->syncp, flags);      /* 4 */
```

即：**2 次 `sched_clock()` + 1 次关中断的 seqcount 临界区 + 1 次 per-CPU 访问**。

- `sched_clock()` 在 x86 上通常是 TSC 读取（~20–30 ns/次），但在某些配置下会退化成 HPET / `ktime_get()` 路径，**单次可达 100 ns 以上**。两次就是 200 ns+。
- `u64_stats_update_begin_irqsave()` 涉及 `local_irq_save`，有额外的指令和可能的 pipeline 影响。
- 这些代码插入在你程序的前后，**污染了 I-cache 和分支预测**，间接拖慢程序本身。

**所以正确的用法是：**

1. **`run_time_ns`/`run_cnt` 用于「相对比较」和「找热点」，不用于「绝对数字」。** 比如：程序 A 埋点后 340 ns、程序 B 埋点后 180 ns，可以放心地说 A 比 B 慢一倍；但 340 ns 不等于 A 的真实成本。
2. **测完立刻关掉**：`sysctl -w kernel.bpf_stats_enabled=0`。它是**全局**开关，开着会拖慢机器上**所有** BPF 程序的每次执行——包括别人挂的 Cilium/XDP 程序。
3. **想要真实绝对数字**，用三层测量：
   - 硬件时间戳（网卡 PTP + `SO_TIMESTAMPING`）做端到端；
   - `bpf_ktime_get_ns()` 在程序首尾各打一次，把差值写进 map（只增加约 40 ns，且不依赖 sysctl）；
   - `bpftool prog dump jited id <ID>` 数指令条数做静态估算。

另外注意：`stats` 是 **per-CPU** 的（`this_cpu_ptr(prog->stats)`），bpftool 显示的是跨 CPU 累加值。如果你的 XDP 程序只挂在 1 个队列上，实际上测的是那一个 CPU 的数据——这恰好可以用来做 per-queue 的成本分解。

</details>

---

## 导航

- **上一篇：** [chapter-07-xdp-redirect-dpdk](../../chapter-07-xdp-redirect-dpdk/) — XDP_REDIRECT 的三步语义与 devmap/cpumap
- **本篇：**
  - [02-bpf.md](02-bpf.md) — 程序类型、map 类型、verifier 限制
  - [03-xdp-bpf.md](03-xdp-bpf.md) — XDP 程序的能力边界与返回码
  - [04-cgroup-bpf.md](04-cgroup-bpf.md) — cgroup attach 语义、继承与叠加
- **下一篇：** [chapter-09-tc-bpf](../../chapter-09-tc-bpf/) — tc-BPF 流量分类
- **相关：** [chapter-05-xdp-architecture](../../chapter-05-xdp-architecture/) XDP 架构 · [chapter-06-af-xdp](../../chapter-06-af-xdp/) AF_XDP · `06.7-bpf-observability/` eBPF 可观测性体系 · [chapter-01](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) 网络栈 hook 点总顺序
