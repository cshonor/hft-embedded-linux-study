# Chapter 10: nftables

> 来源：Bootlin（nftables 概述）+ LWN（nftables 设计 + vs BPF）+ **v6.6 源码逐条核对**
> 对标：Rosen Ch9（iptables → nftables）
> 内核版本：以 **v6.6** 为准，机制、常量、行号均取自源码
> （`net/netfilter/nf_tables_core.c`、`net/netfilter/nf_tables_api.c`、
> `net/netfilter/nft_set_{hash,rbtree,pipapo}.c`、`net/netfilter/core.c`、
> `net/core/dev.c`、`net/ipv4/ip_{input,output}.c`、`include/uapi/linux/netfilter*.h`）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [nftables-bootlin](notes/01-nftables-bootlin.md) | **hook 体系**：7 个插入位置（含 netdev ingress/egress）、优先级全表（DNAT -100 / SNAT +100 的正确性约束）、五 hook 在收发路径的精确行号、verdict 语义与 errno 编码、conntrack 是行情机隐形税 |
| 2 | [nftables-lwn](notes/02-nftables-lwn.md) | **内部机制**：`nft_do_chain()` 逐行解析（blob 连续内存、fast path 直调旁路、16 层 jump 栈）、双代际 blob 原子替换、set 三后端能力表 + `nft_select_set_ops()` 选择算法、pipapo 多维区间匹配、iptables-nft 兼容层的代价 |
| 3 | [nftables-vs-bpf](notes/03-nftables-vs-bpf.md) | **选型对比**：全路径位置图（XDP→taps→tc→nft→…）、三机制能力/成本/可维护性对比、决策表、HFT 三层防线叠加实践、tcpdump 可见性排障分水岭 |

## 本篇的核心结论

1. **⭐ v6.6 的 Netfilter 有 7 个 hook，不止经典 5 个。**
   `NF_INET_INGRESS`（=5）和 `NF_NETDEV_EGRESS` 让 nftables 也能「协议栈之前」过滤。
   收包顺序：tcpdump(:5394) → tc ingress(:5412) → **nft netdev ingress(:5420)** → IP 层。

2. **⭐ DNAT(-100) / SNAT(+100) 的优先级是正确性约束。**
   路由查找发生在两者之间；且 NAT 链 priority ≤ -200（conntrack）直接 `-EOPNOTSUPP`。

3. **⭐ nft 规则是预编译的连续内存 blob，不是链表。**
   热表达式（cmp/cmp16/bitwise/payload）有**直调旁路**（绕过 retpoline 的间接调用），
   简单规则单条成本 ~十几 cycle；但**规则数 = 成本**（线性扫，无 skipping）——快查靠 set/vmap。

4. **⭐ 原子热更新靠双代际 blob**（`blob_gen_0/1` + gencursor 翻转），
   `nft -f` 全量替换无中间态窗口。生产环境必须用全量替换，不要逐条 add。

5. **⭐ set 三后端各司其职**：等值 → hash O(1)；单字段区间 → rbtree O(log n)；
   **拼接+区间 → pipapo**（其 estimate 只接受 interval + 字段数≥2，x86_64 有 AVX2 加速版）。

6. **⭐ tcpdump 可见性是排障分水岭**：看得到 → nft/tc/cgroup 丢的；
   看不到 → **一定是 XDP**（它在 tap 之前）。nft 丢包有 `SKB_DROP_REASON_NETFILTER_DROP`
   + tracepoint 证据链。

## HFT 关联

- **conntrack 是行情机隐形税**：百万 pps 单向流会灌爆连接表，必须 notrack 或不加载模块
- **防火墙位置决定被丢包的前置成本**：netdev ingress（早）< PRE_ROUTING < LOCAL_IN < cgroup
- **白名单用 interval set（rbtree），多维用 concat+pipapo**——3000 条 CIDR 规则链 vs 一次
  O(log n) 查找差三个数量级
- **NAT 永远留给 nftables**：XDP/tc-BPF 无 conntrack，自己做等于重写半个 conntrack
- **三层防线叠加**：XDP 管量（早丢弃）、nft 管策略（安全基线/NAT）、tc-BPF 管动态
  （与应用联动的分类）、qdisc 管发送时机
- **iptables-nft 兼容层没有 fast path 直调**，性能敏感机器应重写为原生 nft 语法

## 交叉引用

- `12.5-modern-networking/chapter-09-tc-bpf/`：tc ingress/egress 与 nft hook 的相对位置
- `12.5-modern-networking/chapter-08-ebpf-cgroup-bpf/`：BPF 框架（对比的另一侧）
- `12.5-modern-networking/chapter-11-packet-filter-flowtable/`：flowtable（Netfilter 的快速路径）
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP（tcpdump 看不到的那个 hook）
