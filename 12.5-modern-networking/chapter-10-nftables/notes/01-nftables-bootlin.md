# 01 — Netfilter/nftables：hook 体系与 v6.6 架构

> **Bootlin 课程模块：** Netfilter/nftables
> **对应 Rosen:** Ch9（Netfilter/iptables）
> **内核版本：** 全部 hook 位置、优先级常量、行号基于 **v6.6** 源码核对
> （`include/uapi/linux/netfilter.h`、`include/uapi/linux/netfilter_ipv4.h`、
> `net/netfilter/core.c`、`net/ipv4/ip_input.c`、`net/ipv4/ip_output.c`、`net/core/dev.c`）

## 文档概述

本篇是 **nftables 的 hook 体系篇**：规则挂在哪、优先级怎么排、verdict 怎么解释、五个经典 hook 在收发路径的精确位置。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [02-nftables-lwn.md](02-nftables-lwn.md) | **内部机制**：nft VM 求值引擎、规则 blob、set 三后端、原子替换 | 本篇讲「挂哪」，02 讲「跑起来什么样」 |
| [03-nftables-vs-bpf.md](03-nftables-vs-bpf.md) | **选型对比**：nftables vs XDP/tc-BPF | 本篇讲机制，03 讲「什么时候用它、什么时候不用」 |

---

## 1. 先建立全景：Netfilter 是协议栈里的 hook 框架

Netfilter 本体只是一组**插入点**（hook）+ 一个**分发器**（`nf_hook_slow()`）。防火墙（nftables）、NAT、连接跟踪都是「注册到这些 hook 上的消费者」。

```c
/* include/uapi/linux/netfilter.h —— 5 个经典 hook + 1 个后加的 ingress */
enum nf_inet_hooks {
	NF_INET_PRE_ROUTING,   /* 0: 路由之前，刚完成 L2→L3 */
	NF_INET_LOCAL_IN,      /* 1: 路由判定「发给我」之后，交给 L4 之前 */
	NF_INET_FORWARD,       /* 2: 路由判定「转发」 */
	NF_INET_LOCAL_OUT,     /* 3: 本机发出的包，进入 IP 输出路径 */
	NF_INET_POST_ROUTING,  /* 4: 即将交给邻居子系统/驱动之前 */
	NF_INET_NUMHOOKS,      /* 5 */
	NF_INET_INGRESS = NF_INET_NUMHOOKS,  /* 5: netdev ingress（挂在 __netif_receive_skb_core） */
};

/* 另一组：netdev 家族专用（挂在 __dev_queue_xmit / __netif_receive_skb_core） */
enum nf_dev_hooks {
	NF_NETDEV_INGRESS,
	NF_NETDEV_EGRESS,
};
```

**关键认知**：v6.6 的 Netfilter 有**七个**插入位置，不止经典的五个。`NF_INET_INGRESS`（5.2+）和 `NF_NETDEV_EGRESS`（5.16+）是后补的——这让 nftables 也能在「协议栈之前」做事，不再只能等路由之后。

### 地址族（family）

```c
/* include/uapi/linux/netfilter.h */
enum {
	NFPROTO_UNSPEC =  0,
	NFPROTO_INET   =  1,   /* ⭐ 同时匹配 IPv4 和 IPv6 */
	NFPROTO_IPV4   =  2,
	NFPROTO_ARP    =  3,
	NFPROTO_NETDEV =  5,   /* 设备级（早于协议栈） */
	NFPROTO_BRIDGE =  7,
	NFPROTO_IPV6   =  10,
};
```

**`inet` 家族是 nftables 相对 iptables 最大的易用性改进**：一套规则同时管 v4/v6（iptables/ip6tables 要维护两份）。`netdev` 家族则是「每设备」的早过滤（详见 [03](03-nftables-vs-bpf.md)）。

---

## 2. hook 优先级：一张表排清所有消费者

同一个 hook 点可以有多个消费者，**按 priority 从小到大依次执行**（`nf_hook_slow()` 按注册顺序遍历，注册时已按优先级排好）。

```c
/* include/uapi/linux/netfilter_ipv4.h:31-45 */
enum {
	NF_IP_PRI_FIRST            = INT_MIN,
	NF_IP_PRI_RAW_BEFORE_DEFRAG = -450,  /* raw 表（defrag 前） */
	NF_IP_PRI_CONNTRACK_DEFRAG = -400,   /* 连接跟踪的分片重组 */
	NF_IP_PRI_RAW              = -300,   /* raw 表 */
	NF_IP_PRI_SELINUX_FIRST    = -225,
	NF_IP_PRI_CONNTRACK        = -200,   /* conntrack */
	NF_IP_PRI_MANGLE           = -150,   /* mangle 表 */
	NF_IP_PRI_NAT_DST          = -100,   /* DNAT（目的 NAT 必须在路由前） */
	NF_IP_PRI_FILTER           = 0,      /* filter 表 */
	NF_IP_PRI_SECURITY         = 50,
	NF_IP_PRI_NAT_SRC          = 100,    /* SNAT（源 NAT 必须在路由后） */
	NF_IP_PRI_SELINUX_LAST     = 225,
	NF_IP_PRI_CONNTRACK_HELPER = 300,
	NF_IP_PRI_CONNTRACK_CONFIRM = INT_MAX,
	NF_IP_PRI_LAST             = INT_MAX,
};
```

| 优先级 | 值 | iptables 对应 | 干什么 |
|---|---|---|---|
| -450 | raw-before-defrag | `raw` 表（NOTRACK 例外） | 重组前就查 |
| -400 | conntrack-defrag | — | 分片重组 |
| -300 | raw | `raw` 表 | 通常只设 NOTRACK |
| -200 | conntrack | — | 连接跟踪 |
| -150 | mangle | `mangle` 表 | 改 TTL/mark/dscp |
| -100 | nat-dst | `nat` 表 PREROUTING | **DNAT**（必须在路由查找前） |
| 0 | filter | `filter` 表 | 过滤主战场 |
| 100 | nat-src | `nat` 表 POSTROUTING | **SNAT/MASQUERADE**（必须在路由查找后） |

**为什么 DNAT 在 -100、SNAT 在 +100？** 因为路由查找发生在两者**之间**：目的地址还没改就路由 → 路由到错误接口；源地址在路由前就改 → 还不知道出接口，选错 SNAT 地址。**优先级不是装饰，是正确性约束。**

nftables 里 priority 是**任意整数**，不再限于这几个档位。但 NAT 链有硬校验（`nf_tables_api.c:2251`）：`type nat` 的链 priority ≤ `NF_IP_PRI_CONNTRACK(-200)` 直接返回 `-EOPNOTSUPP`——防止你在 conntrack 之前做 NAT 把跟踪状态搞乱。

---

## 3. 五个 hook 在 v6.6 路径上的精确位置

收包路径（IPv4，行号均为 v6.6）：

```
驱动收包 → napi
      │
      ▼
__netif_receive_skb_core()                dev.c
      ├─ ptype_all (tcpdump 抓包点)          :5394
      ├─ tc ingress (sch_handle_ingress)     :5412   ← tcx 先、legacy 后
      ├─ nft netdev ingress (nf_ingress)     :5420   ← netdev/inet 家族的 ingress 链
      ▼
ip_rcv()                                   ip_input.c:560
      └─ NF_HOOK(PRE_ROUTING)               :569    okfn=ip_rcv_finish
            ▼ （路由查找 ip_route_input_slow）
      ├── 目的是本机 → ip_local_deliver()
      │         └─ NF_HOOK(LOCAL_IN)        :254    okfn=ip_local_deliver_finish
      │                  └─ → TCP/UDP
      └── 需要转发 → ip_forward()
                └─ NF_HOOK(FORWARD)
                         └─ → ip_output 路径
```

发包路径：

```
sendmsg() → tcp_sendmsg → ip_queue_xmit()
      └─ __ip_local_out()                   ip_output.c:100
            └─ nf_hook(LOCAL_OUT)           :116    okfn=dst_output
                  ▼
ip_output() → ip_finish_output()
      └─ NF_HOOK_COND(POST_ROUTING)         :418    okfn=ip_finish_output
            （SNAT/MASQUERADE 在这里改源地址）
            ▼
__dev_queue_xmit()                         dev.c
      ├─ nft netdev egress (nf_hook_egress) :4303
      ├─ tc egress (sch_handle_egress)      :4311
      └─ qdisc 排队 → 驱动
```

**四个容易搞错的顺序（全部 v6.6 源码验证）：**

1. **nft netdev ingress 在 tc ingress 之后**（`dev.c:5412` → `:5420`）。两者在同一个 `CONFIG_NET_INGRESS` 块里，都由 `ingress_needed_key` 静态键控制。
2. **tcpdump（ptype_all）在一切过滤之前**——所以 tcpdump 能看到「被 XDP 之外的任何东西丢掉的包」。XDP 是唯一例外（它在驱动层，更早）。
3. **nft netdev egress 在 tc egress 之前**（`dev.c:4303` → `:4311`）。而经典的 POST_ROUTING（SNAT）比它们都早。
4. **LOCAL_OUT 在路由查找之后**（`dst_output` 是查找结果驱动的 okfn），POST_ROUTING 在分片判断前后都可能（GSO 场景走 `:397/:413` 的 NF_HOOK，非 GSO 走 `:418` 的 NF_HOOK_COND）。

### 跳过优化：nf_skip_egress

注意 `dev.c:5411/5419` 的 `nf_skip_egress(skb, true/false)`：在 tc ingress 处理期间**临时关闭** netdev egress hook（打标记），处理完恢复。这是防止 ingress 处理中重注入的包被 egress 链二次处理的互锁。看到这个词别困惑，它是**防递归**，不是性能开关。

---

## 4. verdict 语义

hook 函数的返回值（`include/uapi/linux/netfilter.h`）：

```c
#define NF_DROP   0    /* 丢包。kfree_skb_reason(SKB_DROP_REASON_NETFILTER_DROP) */
#define NF_ACCEPT 1    /* 放行，继续下一个 hook / okfn */
#define NF_STOLEN 2    /* 包的所有权被拿走（queued/重注入），后面什么都别做 */
#define NF_QUEUE  3    /* 送到用户态 nfqueue */
#define NF_REPEAT 4    /* 重新执行当前 hook（慎用） */
#define NF_STOP   5    /* 已废弃 */
```

**两个容易忽视的细节：**

1. **`NF_DROP` 可以携带 errno**：`NF_DROP_ERR(x) = (((-x) << 16) | NF_DROP)`，高 16 位编码错误码。`nf_hook_slow()`（`net/netfilter/core.c`）里 `ret = NF_DROP_GETERR(verdict)`，若 ret==0 则默认返回 `-EPERM`。**所以 sendmsg() 收到的错误不一定是 EPERM，取决于规则怎么编码。**
2. **`NF_STOLEN` 对调用方是「return 0」**（`nf_hook_slow` default 分支）：看起来像成功，但 skb 已不归你了。调试「包没到但没报错」时要想到它。

**丢包可见性**：`NF_DROP` 走 `kfree_skb_reason(skb, SKB_DROP_REASON_NETFILTER_DROP)`（v5.16+），配合 `tracepoint:skb:kfree_skb` 可以精确数出「netfilter 丢了多少、哪条链丢的」。这是 iptables 时代 `iptables -L -v` 计数之外的内核态证据链。

---

## 5. nftables 基本操作（对照 iptables 心智模型）

```
nftables 的层级：        iptables 的层级：
table（自定义）           table（内核预定义：filter/nat/mangle/raw）
 └─ chain（自定义或       chain（预定义：INPUT/OUTPUT/FORWARD...）
     base chain 挂 hook）  └─ rule（匹配 + target）
     └─ rule
        ├─ expression 链（匹配/改写/计数...）
        └─ verdict（accept/drop/jump/goto...）
```

```bash
# 创建表（inet 家族 = v4/v6 通吃）
nft add table inet filter

# 创建 base chain（挂到 input hook，priority 0 = filter 档位）
nft add chain inet filter input '{ type filter hook input priority 0 \; }'

# 添加规则（每条 = 若干 expression + verdict）
nft add rule inet filter input iif "lo" accept
nft add rule inet filter input tcp dport 22 accept
nft add rule inet filter input tcp dport 9090 ip saddr 10.0.0.0/24 accept
nft add rule inet filter input counter drop

# 查看规则集（counter 可见每条规则的命中数）
nft list ruleset

# 原子替换整个规则集（生产环境标准做法，见 02 篇「双代际」）
nft -f /etc/nftables.conf
```

**「无状态表」的含义**：table/chain 的存在不依赖内核模块。iptables 时代的 filter/nat/mangle 每个表是一个内核模块；nftables 里它们只是「用户态通过 netlink 定义的元数据」，内核只有一个统一的 nft 核心。这就是为什么 nftables 能按需创建任意数量的表，而 iptables 的表结构是编译期定死的。

---

## 6. HFT 防火墙规则示例（带源码级注释）

```bash
# ── 行情源白名单：集合 + 区间 ──
# flags interval → set 后端会选 rbtree 或 pipapo（支持 NFT_SET_INTERVAL）
nft add set inet filter md_sources '{ type ipv4_addr \; flags interval \; }'
nft add element inet filter md_sources '{ 10.0.1.0/24, 10.0.2.0/24 }'
nft add rule inet filter input udp dport 9090 ip saddr @md_sources accept

# ── 交易端口保护 ──
nft add set inet filter trade_clients '{ type ipv4_addr \; }'   # 无 interval → hash 后端
nft add element inet filter trade_clients '{ 10.0.0.5, 10.0.0.6 }'
nft add rule inet filter input tcp dport 8001 ip saddr @trade_clients accept

# ── 速率限制（非交易流量） ──
nft add rule inet filter input icmp limit 10/second accept

# ── 丢包审计：counter + 显式 drop ──
nft add rule inet filter input counter drop comment "default deny"
```

**HFT 上线前的自查清单：**

| 检查项 | 命令 | 期望 |
|---|---|---|
| filter 链里没有对行情端口的误杀 | `nft list chain inet filter input` | udp 9090 在 drop 之前 accept |
| conntrack 是否在跑（HFT 机通常不需要） | `conntrack -C` / `lsmod \| grep conntrack` | 模块未加载，或规则有 notrack |
| 丢包有计数 | `nft list ruleset` | default deny 带 counter |
| 内核态丢包证据 | `perf trace -e skb:kfree_skb` 或 bpftrace | NETFILTER_DROP 只出现在预期链 |

**conntrack 是 HFT 收包路径的隐形税**：`NF_IP_PRI_CONNTRACK(-200)` 在 filter(0) 之前，每个新连接的首包都要建表项。行情机上是百万级 pps 的单向上行流，连接表会被灌爆（`nf_conntrack: table full, dropping packet`）。解法：raw 表 priority -300 处 `notrack`（nftables 里是 `ct state set notrack`），或者干脆不加载 conntrack 模块。

---

## 7. iptables → nftables 迁移

```bash
# 方案一：iptables-nft 兼容层（iptables 命令翻译成 nft 规则，内核里只有一套）
update-alternatives --set iptables /usr/sbin/iptables-nft
# 验证当前用的是哪套：
iptables -V    # (nf_tables) 字样 = nft 后端

# 方案二：iptables-translate 逐条翻译
iptables-translate -A INPUT -p tcp --dport 9090 -j ACCEPT
# 输出: nft add rule ip filter INPUT tcp dport 9090 counter accept
```

**兼容层的代价**：iptables-nft 生成的规则走 iptables 兼容表达式（`xt_match`/`xt_target` 包装），**享受不到 nft 原生表达式的 fast path 直调**（见 02 篇 §2）。性能敏感的机器应该用原生 nft 语法重写，而不是长期跑兼容层。

---

## 8. HFT 要点

1. **七个 hook 各有代价，位置决定成本**：netdev ingress（协议栈前）< PRE_ROUTING/LOCAL_IN（IP 层）< cgroup（socket 层）。防火墙规则放得越晚，白白付出的协议栈 CPU 越多。
2. **conntrack 是行情机的隐形税**，百万 pps 单向流必须 notrack 或卸载模块。
3. **DNAT/SNAT 的优先级是正确性约束**，不是可调参数。
4. **inet 家族一套规则管 v4/v6**，减少「v6 忘了配」这种经典事故。
5. **`NF_DROP` 默认对 sendmsg 报 EPERM**，但可携带 errno；调试发送失败时先查 nft counter。
6. **丢包走 `SKB_DROP_REASON_NETFILTER_DROP`**，用 kfree_skb tracepoint 可以拿到「谁丢的」的内核态证据，比 nft counter 更细（counter 只到规则粒度，tracepoint 到调用栈）。

---

## 9. 与 Rosen Ch9 的差异

| 维度 | Rosen 3.x（iptables） | v6.6（nftables） |
|---|---|---|
| 前端 | iptables/ip6tables 两套 | nft 一套，inet 家族统一 v4/v6 |
| 表/链 | 内核预定义（每表一个模块） | 用户自定义，无状态表 |
| 规则执行 | match/target 链式回调 | nft VM 字节码（见 02 篇） |
| hook 数 | 5 个 | **7 个**（+netdev ingress/egress） |
| 集合 | ipset 外挂 | 原生 set（三后端，见 02 篇） |
| 优先级 | 隐含在表结构里 | 显式任意整数（NAT 链有下限校验） |
| 丢包可观测 | counter（规则粒度） | counter + SKB_DROP_REASON + tracepoint |

Rosen 的 hook 框架（五个位置、优先级、okfn 瀑布流）**到今天还是对的**——这套设计 25 年没变。变的是前端和执行引擎。

---

## 10. 代码自测

<details>
<summary>Q1：一个 UDP 行情包（dst port 9090），在到达 socket 之前会经过哪几个 Netfilter hook（假设没有 netdev 链）？</summary>

**两个**：

1. `NF_INET_PRE_ROUTING`（`ip_rcv()`，`ip_input.c:569`）——路由查找之前
2. `NF_INET_LOCAL_IN`（`ip_local_deliver()`，`ip_input.c:254`）——路由判定「发给我」之后、UDP 处理之前

如果还挂了 `netdev` 或 `inet` 家族的 ingress 链，则更早：`nf_ingress()`（`dev.c:5420`），在 tc ingress 之后、ip_rcv 之前。

注意：**没有 FORWARD**（那是转发路径），**没有 LOCAL_OUT/POST_ROUTING**（那是发包路径）。
</details>

<details>
<summary>Q2：为什么 tcpdump 能看到「被 filter 表 INPUT 链 drop 的包」？</summary>

tcpdump 挂在 `ptype_all`（AF_PACKET tap），位置 `dev.c:5394`，在 `__netif_receive_skb_core()` 的**最前面**——早于 tc ingress（:5412）、nft netdev ingress（:5420）、更早于 IP 层的 PRE_ROUTING/LOCAL_IN。

唯一早于 tcpdump 的过滤点是 **XDP**（驱动层）——这就是「tcpdump 看到了包但应用收不到 → 排查顺序：nft counter → tc → XDP」的判定逻辑依据。
</details>

<details>
<summary>Q3：应用调用 sendto() 返回 EPERM，但程序没检查任何权限。可能是谁干的？</summary>

大概率是 nftables/iptables 的 OUTPUT 链 `NF_DROP`。`nf_hook_slow()`（`net/netfilter/core.c`）里：

```c
case NF_DROP:
	kfree_skb_reason(skb, SKB_DROP_REASON_NETFILTER_DROP);
	ret = NF_DROP_GETERR(verdict);
	if (ret == 0)
		ret = -EPERM;      /* ⭐ 默认 errno 就是 EPERM */
	return ret;
```

验证：`nft list ruleset` 看 output 链的 counter 是否在涨；或 `bpftrace -e 't:skb:kfree_skb /args->reason == SKB_DROP_REASON_NETFILTER_DROP/ { @[kstack] = count(); }'`。
</details>

<details>
<summary>Q4：nftables 能把规则挂在 tc ingress 之前吗？</summary>

**不能**（v6.6）。收包路径上固定的顺序是：

```
tcpdump(ptype_all) → tc ingress(sch_handle_ingress, dev.c:5412) → nft netdev ingress(nf_ingress, dev.c:5420)
```

两者都在 `CONFIG_NET_INGRESS` 块里，由同一个静态键 `ingress_needed_key` 控制，顺序写死。nftables 最早就到 `nf_ingress()`。想要「比 tc 更早」只有 XDP（驱动层）。

发方向则相反：**nft egress（:4303）在 tc egress（:4311）之前**。
</details>

<details>
<summary>Q5：为什么 NAT 链的 priority 不能设成 -300？</summary>

`nf_tables_api.c:2251` 的硬校验：

```c
if (type->type == NFT_CHAIN_T_NAT &&
    hook->priority <= NF_IP_PRI_CONNTRACK)   /* -200 */
	return -EOPNOTSUPP;
```

NAT 依赖 conntrack 的连接状态（NAT 表项要挂在 conntrack 表项上）。如果 NAT 链排在 conntrack（-200）之前，改了地址却没有跟踪状态，后续包就匹配不上已有 NAT 映射，连接会烂掉。所以内核直接拒绝这种配置，报「Operation not supported」。
</details>

---

## 导航

- **下一篇：** [02-nftables-lwn.md](02-nftables-lwn.md) — nft VM 求值引擎、规则 blob 双代际、set 三后端与选择算法
- **相关：** [chapter-09-tc-bpf/](../../chapter-09-tc-bpf/) tc ingress/egress 与本篇 hook 的相对位置 · [chapter-11-packet-filter-flowtable/](../../chapter-11-packet-filter-flowtable/) flowtable（Netfilter 的快速路径） · [chapter-08-ebpf-cgroup-bpf/](../../chapter-08-ebpf-cgroup-bpf/) eBPF 框架
- **章节主页：** [README](../README.md)
