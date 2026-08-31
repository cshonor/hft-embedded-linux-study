# 02 — Netfilter flowtable：连接级快路径

> **对应 Rosen:** Ch9（Netfilter——无此机制，flowtable 是 4.16+ 的产物）
> **内核源码路径：** `Documentation/networking/nf_flowtable.rst`
> **核对源码：** v6.6 `net/netfilter/nf_flow_table_core.c`、`nf_flow_table_ip.c`、
> `nf_flow_table_inet.c`、`nft_flow_offload.c`、`nf_tables_api.c`、
> `include/net/netfilter/nf_flow_table.h`

## 文档概述

flowtable 是 Netfilter 对「每包规则匹配太贵」的自我救赎：**首包走完整慢路径建立状态，后续包查一次哈希表直接转发**。本篇基于 v6.6 源码拆解它的位置、快路径具体做了什么、和 conntrack/NAT 的关系，以及 HFT 视角下它的适用边界。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [01-packet-filter.md](01-packet-filter.md) | 包级过滤器（BPF）的演进 | 01 是「包级」优化，本篇是「流级」缓存——同一动机的两种解法 |

---

## 1. 动机与位置：flowtable 挂在哪

**转发场景的痛点**：一个转发流量的网关，每个包要走 conntrack → 规则匹配 → NAT → 路由 → 邻居解析，其中规则匹配和 conntrack 查找都是每包开销。但流的首包已经把「该不该转、怎么转、改成什么地址」全决定了——后续包理论上只需要查一次缓存。

**v6.6 的 flowtable 架构（两半）**：

```
                          nftables 配置
                               │
        ┌──────────────────────┼──────────────────────┐
        ▼                      ▼                      ▼
  ① flowtable 对象       ② forward 链里的          （普通规则照常）
  hook ingress            「flow offload」表达式
  priority <n>            type filter hook forward
  devices = { eth0, eth1 }
        │                      │
        ▼                      ▼
  设备级 ingress hook      首包经过时把流
  （每个设备一个）          写入 flowtable
        │
        ▼
  后续包在「进入 IP 栈之前」被拦截 → 快路径直转
```

两个源码级事实（v6.6）：

1. **flowtable 只能挂 netdev ingress**：`nf_tables_api.c` 的 `nft_flowtable_parse_hook()` 里 `hooknum != NF_NETDEV_INGRESS` 直接 `-EOPNOTSUPP`。位置在 `__netif_receive_skb_core()` 的 `nf_ingress()`（dev.c:5420，即 nft netdev ingress 同一入口，按 flowtable 的 priority 排序）。
2. **`flow offload` 表达式只能出现在 FORWARD 链**：`nft_flow_offload.c:385`，`hook_mask = (1 << NF_INET_FORWARD)`，链校验强制。

**为什么拦截点在 ingress 而不是 forward？** 因为快路径的目标是**完全绕过 IP 栈**：包从 eth0 进来，flowtable 命中后直接从 eth1 发出去，中间的 `ip_rcv`、路由、conntrack、forward 链全都不走。挂在 ingress 才有机会在进栈前截住。

**文档强调的优先级规则**（`doc_nf_flowtable.rst:100-102`）：*flowtable 的 priority 必须**小于**同设备上 nftables ingress 链的 priority*——否则包先被 ingress 链处理（可能被丢弃/标记），flowtable 的语义就乱了。

---

## 2. 快路径逐行：`nf_flow_offload_ip_hook()`

这是每个命中包走的核心函数（`nf_flow_table_ip.c:409`，v6.6 已重构为 `nf_flow_offload_forward()` + 发送分支）：

```c
nf_flow_offload_ip_hook(void *priv, struct sk_buff *skb, ...)
{
	tuplehash = nf_flow_offload_lookup(&ctx, flow_table, skb);   /* ① rhashtable 查流 */
	if (!tuplehash)
		return NF_ACCEPT;                        /* 未命中 → 回慢路径 */

	ret = nf_flow_offload_forward(&ctx, flow_table, tuplehash, skb);
	if (ret < 0)  return NF_DROP;                   /* MTU 超限等硬错误 */
	else if (ret == 0)  return NF_ACCEPT;           /* 状态失效 → 回慢路径 */

	if (unlikely(tuplehash->tuple.xmit_type == FLOW_OFFLOAD_XMIT_XFRM)) {
		return nf_flow_xmit_xfrm(skb, state, &rt->dst);   /* ② IPsec 隧道 */
	}

	switch (tuplehash->tuple.xmit_type) {
	case FLOW_OFFLOAD_XMIT_NEIGH:                   /* ③ 普通邻居转发 */
		rt = (struct rtable *)tuplehash->tuple.dst_cache;
		skb->dev = outdev;
		nexthop = rt_nexthop(rt, ...);
		skb_dst_set_noref(skb, &rt->dst);
		neigh_xmit(NEIGH_ARP_TABLE, outdev, &nexthop, skb);
		ret = NF_STOLEN;                            /* ⭐ 包的所有权转移 */
		break;
	case FLOW_OFFLOAD_XMIT_DIRECT:                  /* ④ VLAN 直发 */
		ret = nf_flow_queue_xmit(state->net, skb, tuplehash, ETH_P_IP);
		break;
	}
	return ret;
}
```

### 2.1 `nf_flow_offload_forward()` 里做的「迷你 IP 转发」

```c
/* nf_flow_table_ip.c —— 顺序即要点 */
mtu = flow->tuplehash[dir].tuple.mtu + ctx->offset;
if (unlikely(nf_flow_exceeds_mtu(skb, mtu))) return 0;   /* MTU 检查 */

if (nf_flow_state_check(flow, iph->protocol, skb, thoff)) /* TCP 状态检查 */
	return 0;

if (!nf_flow_dst_check(&tuplehash->tuple)) {              /* 路由缓存还有效？ */
	flow_offload_teardown(flow);                      /* ⭐ 失效 → 拆流回慢路径 */
	return 0;
}

flow_offload_refresh(flow_table, flow, false);           /* 刷新 30s 超时 */

nf_flow_encap_pop(skb, tuplehash);                       /* 剥 VLAN 封装 */
nf_flow_nat_ip(flow, skb, thoff, dir, iph);              /* ⭐ 做 NAT（增量校验和） */
ip_decrease_ttl(iph);                                    /* TTL 减一 */
skb_clear_tstamp(skb);
```

**要点**：快路径不是「什么都不查直接发」，而是把慢路径里**有状态的检查**（MTU、TCP 连接状态、路由有效性）压缩成 O(1) 的缓存命中 + 增量更新。NAT 在这里做增量 checksum 修改（`nf_flow_nat_ip`），不重算整个校验和。

### 2.2 `NF_STOLEN`：快路径的返回语义

`neigh_xmit()` 直接把包交给邻居子系统发出，skb 所有权转移——对 Netfilter 框架返回 `NF_STOLEN`（=2），`nf_hook_slow()` 对它「return 0 装作成功」。**这就是快路径「绕过」的表达方式：不是跳过代码，而是把包偷走。**

### 2.3 双向 tuple：original/reply 各有一条

```c
/* include/net/netfilter/nf_flow_table.h */
enum flow_offload_tuple_dir {
	FLOW_OFFLOAD_DIR_ORIGINAL = IP_CT_DIR_ORIGINAL,   /* 去程 */
	FLOW_OFFLOAD_DIR_REPLY    = IP_CT_DIR_REPLY,      /* 回程 */
};

struct flow_offload {
	struct flow_offload_tuple_rhash tuplehash[FLOW_OFFLOAD_DIR_MAX];  /* 两条 */
	...
};
```

流表按**五元组 + 方向**索引（rhashtable，查一次 O(1)）。回程包查到的是 REPLY tuple，NAT 反向映射直接从 tuple 里取——**快路径自己完成了 de-NAT**，不查 conntrack。

### 2.4 XMIT 类型的完整清单

```c
enum flow_offload_xmit_type {
	FLOW_OFFLOAD_XMIT_UNSPEC = 0,
	FLOW_OFFLOAD_XMIT_NEIGH,     /* 普通下一跳（neigh_xmit + NF_STOLEN） */
	FLOW_OFFLOAD_XMIT_XFRM,      /* IPsec 策略路由 */
	FLOW_OFFLOAD_XMIT_DIRECT,    /* MAC 直发（同网段/VLAN 子接口） */
	FLOW_OFFLOAD_XMIT_TC,        /* ⭐ v6.6 新增：发回 TC 层 */
};
```

`XMIT_TC` 是 v6.6 的新能力：快路径命中后不直接发，而是**注入回 tc 层**（配合 flower 的 hw offload 场景）——说明 flowtable 和 tc 的边界在融合。

---

## 3. 生命周期：首包建立、超时回收

### 3.1 建表：`flow offload` 表达式

```
首包路径：ingress → ip_rcv → conntrack(建流) → forward 链 → 「flow offload」表达式
                                                                    │
                                    flow_offload_alloc(ct)          ▼
                                    （从 conntrack tuple 复制）→ flow_offload_add()
                                                                加入 rhashtable
```

`flow_offload_alloc()`（`nf_flow_table_core.c:52`）从 **conntrack 表项**复制五元组和 NAT 信息——flowtable 依赖 conntrack 存在（`flow offload` 表达式要求 ct 状态为 ESTABLISHED 才建条目，由 `nft_flow_offload_eval()` 检查）。

### 3.2 超时与 GC

```c
#define NF_FLOW_TIMEOUT (30 * HZ)     /* 30 秒 */

/* flow_offload_refresh()：每个命中包刷新 */
flow->timeout = nf_flowtable_time_stamp + flow_offload_get_timeout(flow);
```

- 每个**命中包**都刷新超时（`nf_flow_offload_forward()` 里的 `flow_offload_refresh`）
- 30 秒无流量 → GC work（`system_power_efficient_wq` 上的 `gc_work`，每 HZ 跑一轮）回收
- TCP 流的 teardown（FIN/RST）也会触发 `flow_offload_teardown()`

### 3.3 counter 同步（可选）

```
flowtable f { hook ingress priority 0; devices = { eth0, eth1 }; counter; }
```

加 `counter` 后，flowtable 快路径的包/字节计数**同步回 conntrack 表项**（`doc_nf_flowtable.rst:168-169`）——`conntrack -L` 看到的计数包含快路径流量，监控不瞎。`nft list ruleset` 里能看到 offload 了多少流（`[OFFLOAD]` 标记，`conntrack -L` 也能看）。

---

## 4. 硬件 offload：`flags offload`

```
flowtable f {
	hook ingress priority 0; devices = { eth0, eth1 };
	flags offload;      /* 请求网卡接管已建立的流 */
}
```

软件 flowtable 命中后仍要走 CPU（查表、改 TTL/checksum、neigh_xmit）；硬件 offload 把**整条流**下发到网卡（`nf_flow_table_offload.c`，通过 tc 的 flow block 基础设施 `FLOW_ACTION_` 指令），后续包**网卡直接转发，CPU 完全不参与**。

驱动支持：mlx5、ice 等企业卡。限制：硬件能表达的匹配/动作有限（五元组匹配 + 少量 action），复杂 NAT/策略仍回软件。

---

## 5. 完整配置示例（文档例子 + 注释）

```bash
nft add table inet t
nft add flowtable inet t f '{ hook ingress priority 0 \; devices = { eth0, eth1 } \; }'

# forward 链：policy accept（保证没被 offload 的包正常转发）
nft add chain inet t y '{ type filter hook forward priority 0 \; policy accept \; }'

# 计数器：观察多少包没走快路径（offload 生效后这个 counter 应该只在首包增长）
nft add rule inet t y counter

# 核心规则：已建立的连接 offload 到 flowtable
nft add rule inet t y ct state established flow offload

# 验证：
conntrack -L      # offload 的流带 [OFFLOAD] 标记
nft list ruleset  # flowtable 状态可见
```

**排障三板斧**：

1. `conntrack -L | grep OFFLOAD`——流有没有进表
2. forward 链的 counter——后续包还在过慢路径说明没命中（先查 `ct state established` 匹配和 devices 列表）
3. `perf trace -e net:netif_receive_skb` 或 dropwatch——快路径的包**不会**出现在 IP 层 tracepoint 里（这就是它快的原因，也是「包凭空消失」的正常表现）

---

## 6. HFT 要点

1. **flowtable 的目标是转发流量的吞吐，不是本机收包的延迟**。它对「包从 eth0 进、eth1 出」的网关场景是数量级优化；HFT 行情机是**本机终结**（包交给应用），flowtable 帮不上
2. **HFT 顺带受益的场景**：行情前置机/风控网关做 L3/L4 分流（同机多策略进程），forward 流量 offload 后 CPU 留给行情处理
3. **「绕过 IP 栈」是有代价的语义收缩**：快路径不做 conntrack 更新（除计数）、不做 per包规则、不做 QoS——**依赖 per 包逻辑的功能（如 nft limit）在 offload 后失效**。这是吞吐换灵活性，与 XDP 的取舍同构
4. **30 秒超时 + 每包刷新**：静默连接会被拆回慢路径重建。对长连接低频流量（管理通道）要注意首包延迟的偶发回归
5. **与 XDP 的关系是分工不是竞争**：XDP 逐包可编程（首包就能决策），flowtable 是「首包学习 + 后续包 O(1)」的固定模式。两者可叠加：XDP 做粗筛/丢弃，flowtable 加速剩余转发
6. **硬件 offload 是「CPU 零参与」的唯一路径**——比任何软件快路径（XDP 也包括在内）都快，但灵活性最低、且依赖网卡

---

## 7. 与 Rosen 的差异

Rosen 3.x 时代没有 flowtable（4.16 引入软件版，5.x 成熟硬件 offload）。对应它的知识空位：

| Rosen 的世界 | flowtable 的世界 |
|---|---|
| 每包走完整 Netfilter | 首包建流，后续包 ingress 截住直转 |
| 转发性能靠规则精简 | 转发性能与规则数解耦（查一次哈希） |
| NAT 每包查 conntrack | de-NAT 从流表 tuple 直接取 |
| 无硬件参与 | `flags offload` 下网卡整流接管 |

---

## 8. 代码自测

<details>
<summary>Q1：flowtable 的拦截点为什么在 ingress 而不是 FORWARD（规则挂的地方）？</summary>

因为快路径的语义是「**完全绕过 IP 栈**」。挂在 FORWARD 意味着包已经走完 `ip_rcv`（校验、conntrack 查找）和路由——这些正是要省掉的开销。

v6.6 的实现：flowtable 对象在**每个成员设备**上注册 netdev ingress hook（`nf_tables_api.c` 强制 `hooknum == NF_NETDEV_INGRESS`），拦截点就是 `nf_ingress()`（dev.c:5420）。命中的包从设备层直接 `neigh_xmit` 出去，IP 栈整个不经过。

而 FORWARD 链上只挂「flow offload」**表达式**——它的作用是在首包过慢路径时**建流**，不是拦截。
</details>

<details>
<summary>Q2：快路径对命中的包做了哪些「必须做」的检查？分别为什么？</summary>

`nf_flow_offload_forward()`（nf_flow_table_ip.c）逐项：

| 检查 | 原因 |
|---|---|
| MTU 超限 | 出接口路径 MTU 变了（隧道增删、路由切换），超限包必须回慢路径走分片/ICMP |
| TCP 状态（`nf_flow_state_check`） | 收到 FIN/RST 后连接要结束，不能继续无状态直转 |
| 路由缓存有效（`nf_flow_dst_check`） | 路由表变了，缓存的 `dst_cache` 指向的设备可能已下线；失效则 `flow_offload_teardown` 拆流 |
| TTL 减一 + 增量 NAT | 转发的协议义务，不做就是黑洞/环路 |

设计哲学：**缓存加速的是「查询」，不缓存「不变量」**——每个可能失效的条件每包都要验，验的成本必须是 O(1)。
</details>

<details>
<summary>Q3：为什么 `flow offload` 表达式要求 `ct state established`？首包（SYN）能 offload 吗？</summary>

不能。flowtable 的快路径**不维护连接状态机**（除了 TCP 状态的粗检查），它只是 conntrack 结论的缓存。首包（SYN）时：

1. conntrack 表项刚建立，还没有经过完整的规则链验证（forward 链的过滤规则还没跑完）
2. NAT 映射可能尚未确定（DNAT 在 PRE_ROUTING 已做，但依赖路由结果的 SNAT 还没到）

所以表达式设计成只在 `ct state established` 时建流——首包完整走慢路径正是**正确性的一部分**，不是浪费。

（技术细节：`nft_flow_offload_eval()` 会检查 ct；v6.6 里也允许 `flow add at ...` 之类更精细的时点控制，但 established 是默认门槛。）
</details>

<details>
<summary>Q4：一个被 flowtable 快路径转发的包，`tracepoint:net:netif_receive_skb` 和 nft forward counter 分别能看到吗？</summary>

**都看不到**：

- 快路径发生在 `nf_ingress()`（设备层），**早于** `__netif_receive_skb_core` 的协议处理和 `ip_rcv`——IP 层 tracepoint 不触发（skb 被 `neigh_xmit` 直接从设备层发出去了，`NF_STOLEN`）
- forward 链只在首包和未命中包上执行——offload 后 counter 不再增长（这正是文档例子里放 counter 的目的：**验证 offload 生效**）

这也是 flowtable 排障「包凭空消失」错觉的来源：所有基于 IP 层以上的观测（tcpdump 在 ptype_all 倒是能看到、nft counter、conntrack 计数）里，快路径流量只反映在 flowtable 的 counter（若配置）和 conntrack 的同步计数里。
</details>

<details>
<summary>Q5：HFT 行情网关，XDP 过滤 + flowtable 转发剩余流量，顺序对吗？</summary>

对，且顺序固定（由位置决定）：

```
包到达 → XDP（驱动层，丢弃垃圾流量）→ tc/nft ingress → flowtable 拦截（设备 ingress hook，
按 priority 与 nft ingress 链排序）→ [命中 → 直转发] / [未命中 → IP 栈 → ...]
```

XDP 在最前（skb 分配前），flowtable 的 ingress hook 在其后。两者语义互补：XDP 是**无状态的逐包决策**（首包就要丢的流量直接丢），flowtable 是**有状态的流缓存**（首包放行后加速后续）。注意 flowtable 的 priority 要小于同设备 nft ingress 链的 priority（文档要求），否则 ingress 链的 drop/标记逻辑会先跑，语义就乱了。
</details>

---

## 导航

- **上一篇：** [01-packet-filter.md](01-packet-filter.md) — 包级过滤器（cBPF→eBPF）
- **相关：** [chapter-10-nftables/](../../chapter-10-nftables/) flowtable 的宿主体系（hook/优先级/conntrack） · [chapter-05-xdp-architecture/](../../chapter-05-xdp-architecture/) XDP（逐包可编程的对照解法） · [chapter-07-xdp-redirect-dpdk/](../../chapter-07-xdp-redirect-dpdk/) 高性能转发的另一条路
- **章节主页：** [README](../README.md)
