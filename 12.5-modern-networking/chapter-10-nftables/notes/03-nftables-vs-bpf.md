# 03 — nftables vs eBPF：位置、成本与选型

> **对应 Rosen:** 无（3.x 时代没有可对比的第二条路）
> **内核版本：** 对比基于 **v6.6** 源码（两侧机制均已核对：nft 求值引擎见 [02](02-nftables-lwn.md)；
> XDP/tc-BPF 见 [chapter-05](../../chapter-05-xdp-architecture/)、[chapter-09](../../chapter-09-tc-bpf/)）
> **核心方法论：** 对比「包过滤机制」本质是对比**三件事：位置（决定了前置成本）、能力（能拿到什么、能改什么）、可维护性（谁来运维）**

## 文档概述

本篇回答一个工程问题：**同一件事（过滤/标记/丢弃一个包），什么时候用 nftables，什么时候用 XDP/tc-BPF？**

结论先行：

- **静态策略（IP/端口/协议的组合、NAT、限速）→ nftables**。声明式、原子热更新、set/vmap 的多维匹配足够快，且运维门槛低（安全团队就会 nft，不一定会写 BPF）
- **动态逻辑（按包内容计算、查运行时状态、需要 map 联动）→ XDP/tc-BPF**。图灵完备、位置更早、可观测性靠 bpftrace 一套生态
- **HFT 现实**：两者叠着用，各管一层

---

## 1. 一张图看清全部位置（v6.6 收包路径）

```
收包（RX）：
驱动收包 → NAPI
      │
      ▼
  ① XDP（native，驱动层）          ← 没有 skb，最快；tcpdump 看不到被它丢的包
      ▼
__netif_receive_skb_core()                    net/core/dev.c
  ② ptype_all（tcpdump 抓包点）                :5394
  ③ tc ingress（tcx 先、legacy 后）            :5412
  ④ nft netdev ingress（nf_ingress）          :5420   ← netdev/inet 家族的 ingress 链
      ▼
  ⑤ nft PRE_ROUTING（ip_rcv，含 conntrack/NAT） ip_input.c:569
      ▼ （路由查找）
  ⑥ nft LOCAL_IN（ip_local_deliver）            ip_input.c:254
      ▼
  ⑦ UDP/TCP 栈 → socket 查找 → cgroup ingress（sk_filter_trim_cap）
      ▼
  socket 接收队列 → 应用
```

```
发包（TX）：
sendmsg()
      ▼
  ① nft LOCAL_OUT（__ip_local_out）             ip_output.c:116
      ▼ （路由查找结果 dst_output）
  ② nft POST_ROUTING（ip_finish_output，SNAT）   ip_output.c:418
      ▼
__dev_queue_xmit()                             net/core/dev.c
  ③ nft netdev egress（nf_hook_egress）         :4303
  ④ tc egress（sch_handle_egress，tcx 先）       :4311
      ▼
  ⑤ qdisc（fq/prio/etf...）→ 驱动
```

**这张图直接给出第一个选型判据——「想挡住一个包，愿意付出多少前置成本」：**

| 挡在 | 该包已经付出了什么 | 典型场景 |
|---|---|---|
| XDP | 仅驱动收包 + DMA | 攻击流量清洗、行情早过滤 |
| nft netdev ingress | + skb 分配、ptype_all 遍历、tc 链 | 设备级白名单（比 IP 层早，比 XDP 晚一点） |
| nft PRE_ROUTING | + VLAN/PPPoE 剥层、ip_rcv 校验 | DNAT、按目的地址分流 |
| nft LOCAL_IN | + 路由查找、conntrack | 传统防火墙（INPUT 链） |
| cgroup ingress | + 完整 IP/UDP/TCP 栈 + socket 查找 | 「这个 socket 该不该收」的业务语义 |

**每往后一个位置，被丢弃的包都已经白白消耗了前面所有阶段的 CPU。** 对正常流量无所谓（反正要走到那），对攻击/垃圾流量（目标是丢弃）就是纯浪费。

---

## 2. 能力对比：拿得到什么，改得动什么

| 维度 | nftables | tc-BPF | XDP |
|---|---|---|---|
| **操作对象** | `skb`（通过 payload/cmp 表达式间接读） | `__sk_buff`（81 个 helper，大部分字段可写） | `xdp_buff`（23 个 helper，线性区直接改） |
| **读包内容** | ✅ payload 表达式（固定偏移） | ✅ `data`/`data_end`（任意偏移，可循环） | ✅ `data`/`data_end` |
| **改包内容** | ⚠️ 有限（payload/mangle 类表达式） | ✅（`adjust_head` 等） | ✅（最灵活，可增删头部） |
| **逻辑复杂度** | 声明式规则 + set 查询 | **图灵完备**（C→BPF，verifier 审查） | **图灵完备** |
| **运行时状态** | set（可带 timeout/object） | **BPF map**（内核/用户态双向读写，联动应用） | **BPF map** |
| **重定向** | ⚠️ 有限 | ✅ `bpf_redirect`（devmap/cpumap） | ✅ `bpf_redirect_map`（最完整） |
| **NAT** | ✅ 成熟（conntrack 联动） | ⚠️ 要自己维护状态 | ❌ 无 conntrack，基本不用于 NAT |
| **限速** | ✅ limit 表达式（简单） | ✅（token bucket 自己写或 map） | ✅（自己写） |
| **原子热更新** | ✅ 双代际 blob（见 [02](02-nftables-lwn.md) §3） | ✅ tcx 的 bpf_link 原子替换 | ✅ bpf_link |
| **丢包可观测** | counter + `SKB_DROP_REASON_NETFILTER_DROP` | `SKB_DROP_REASON_TC_INGRESS/EGRESS` | 驱动统计更新在 XDP 之后，`tracepoint:xdp:xdp_exception` 是唯一可靠观测 |
| **tcpdump 可见性** | ✅ 被丢的包 tcpdump 能看到 | ✅ 能看到 | ❌ **看不到**（XDP 在 tap 之前） |

**三个容易误判的能力细节：**

1. **nft 的「读包」是模板化的**：payload 表达式声明「L3 头偏移 X 处取 Y 字节」，适合固定字段（IP/proto/port），**做不了「扫描 payload 找特征」**——那是 BPF 的活。
2. **XDP 没有 conntrack**：NAT 需要连接状态跟踪，XDP 拿不到（也刻不起）——所以 NAT 是 nftables/iptables 的保留地。
3. **tcpdump 可见性是排障分水岭**：`tcpdump 看得到但应用收不到` → 查 nft counter / tc / cgroup；`tcpdump 都看不到` → **一定是 XDP**（或驱动/链路问题）。

---

## 3. 成本对比：单次求值到底多贵

基于 v6.6 两侧源码的机制分析（量级估算，具体数字需实测）：

| 机制 | 求值成本结构 | 量级 |
|---|---|---|
| nft 单条简单规则 | fast path 直调（payload_fast + cmp_fast + verdict），无间接调用 | ~十几 cycle |
| nft 规则链 N 条 | 线性 × N（无 skipping） | N × 十几 cycle |
| nft set 查找 | hash O(1) / rbtree O(log n) / pipapo 字段词典 | 一次查找替代 N 条规则 |
| tc-BPF（tcx） | JIT 后原生代码，helper 调用有进出内核的固定开销 | 程序本身 ~几十-几百 cycle，取决于逻辑 |
| XDP | JIT 原生代码，无 skb，数据 pointer 直接操作 | 最低（省掉了 skb 分配本身 ~几百 ns） |

**关键不对称：**

- nft 的成本优势在「**规则简单**」——fast path 直调；劣势在「**规则多**」——线性扫描
- BPF 的成本优势在「**逻辑复杂**」——一次 JIT 程序能干 nft 几百条规则的活；劣势在「简单场景杀鸡用牛刀」以及**开发/验证/运维成本**（要过 verifier、要处理 CO-RE、要有人会改）

**HFT 的量化直觉**：收包路径上每省 100ns，在 10Gbps 线速（14.88Mpps）上相当于每秒省 1.5 秒 CPU。XDP 相对 nft INPUT 链省的是「skb 分配 + 协议栈前半段」（几百 ns 级），nft 相对 cgroup 省的是「协议栈后半段 + socket 查找」（μs 级）。

---

## 4. 选型决策表

| 场景 | 推荐 | 原因（源码/机制依据） |
|---|---|---|
| 端口/IP 静态白名单（几十~几千条） | **nftables set** | hash O(1)，fast path，运维门槛低 |
| 多维组合（IP 段 × 端口段 × 协议） | **nftables concat set（pipapo）** | 一次词典查找完成多维区间匹配 |
| NAT（SNAT/DNAT/MASQUERADE） | **nftables** | conntrack 联动是刚需，BPF 侧没有等价物 |
| 攻击流量清洗（百万 pps 级丢弃） | **XDP** | 位置最早，被丢的包不消耗 skb 分配 |
| 行情早过滤（组播组+端口粗筛） | **XDP** | 行情 pps 高但策略简单；省下的 skb 分配是纯赚 |
| 按包内容做复杂计算（如解析到某字段后查 map 决策） | **tc-BPF 或 XDP** | nft payload 表达式做不了任意逻辑 |
| 给包打标记/改 dscp 供 qdisc 用 | **nftables 或 tc-BPF** | nft 的 mangle 够用；tc-BPF 适合「标记值需要运行时计算」的场景 |
| 限速（简单固定阈值） | **nftables limit** | 一行规则 |
| 限速（动态/按对端/联动应用状态） | **tc-BPF + map** | BPF map 可以被应用实时调阈值 |
| 「防火墙规则每周改、由安全团队维护」 | **nftables** | 声明式 + 原子替换 + trace，不需要程序员 |
| 「逻辑跟交易策略联动、由量化开发维护」 | **BPF** | 和应用共享 map，一个代码库 |
| 需要丢包有「为什么」的完整证据链 | **两者都要** | `SKB_DROP_REASON_*` 按 hook 区分 reason，tracepoint:skb:kfree_skb 全局可见 |

---

## 5. HFT 实践：叠加而非二选一

典型行情机的完整防线（v6.6 位置顺序）：

```
RX: ① XDP：行情组播粗筛（非订阅组直接丢）+ 记 per-queue pstats
    ④ nft netdev ingress（可选）：设备级白名单兜底
    ⑤ nft PRE_ROUTING：notrack（行情流不进 conntrack）
    ⑥ nft LOCAL_IN：INPUT 防火墙（管理面 SSH/监控端口 + default deny）

TX: ① nft LOCAL_OUT：OUTPUT 防火墙
    ④ tc egress：tc-BPF 给交易流打 skb->priority
    ⑤ qdisc：prio（交易流高优先级）+ tbf（行情重传/日志限速）
```

**分工原则：**

- **XDP 管「量」**：百万 pps 级的丢弃/分流，位置就是一切
- **nft 管「策略」**：IP/端口维度的安全基线，NAT，conntrack 相关的一切
- **tc-BPF 管「动态」**：需要和应用联动（map）、需要复杂计算的分类
- **qdisc 管「发送时机」**（见 [chapter-09](../../chapter-09-tc-bpf/)）：BPF 只能分类+丢包，整形必须 qdisc

**一个真实的坑**：三层防线（XDP + nft + cgroup）都可能丢包，排障时先看 tcpdump——

```
tcpdump 看得到 → 不是 XDP 丢的
  → nft counter 在涨？→ nft trace 定位到规则
  → tc -s 统计在涨？→ tc 侧丢的
  → 都没有？→ cgroup ingress 或 socket buffer 满（dropwatch / ss -s）
tcpdump 看不到 → XDP（tracepoint:xdp:xdp_exception）或链路层
```

---

## 6. 常见误区纠正

| 误区 | 事实 |
|---|---|
| "BPF 一定比 nftables 快" | 简单规则 nft fast path 直调可能更快；BPF 的优势在复杂逻辑和更早的位置，不在「执行引擎本身」 |
| "nftables 规则多了也不怕，内核会优化" | **没有**规则索引/skipping——线性扫描是设计选择，「快」的路径是 set/vmap（见 [02](02-nftables-lwn.md) §4） |
| "XDP 能替代防火墙" | XDP 无 conntrack、无 NAT、无 fragment 处理——它是「最前面的包处理器」，不是防火墙 |
| "iptables-nft = nftables" | 兼容层走 xt 包装表达式，**没有 fast path 直调**，也用不上 set/vmap（见 [02](02-nftables-lwn.md) §5） |
| "丢包都能在 tcpdump 里看到" | XDP 丢的看不到（在 tap 之前）；这也是 XDP 的特性不是缺陷（省掉了 tap 本身的成本） |
| "tc 和 nftables 的 egress 谁先谁后无所谓" | v6.6 固定 **nft egress（dev.c:4303）在 tc egress（:4311）之前**——改源地址/标记要在正确的层做 |

---

## 7. 代码自测

<details>
<summary>Q1：一个被 nft INPUT 链 drop 的包，和一个被 XDP drop 的包，tcpdump 分别看得到吗？</summary>

- **nft INPUT drop：看得到**。tcpdump 在 `ptype_all`（dev.c:5394），早于 IP 层的 LOCAL_IN。
- **XDP drop：看不到**。XDP 在驱动层，早于 `__netif_receive_skb_core()` 全部内容（包括 ptype_all）。

这是排障的第一分水岭。XDP 丢包的观测只有 `tracepoint:xdp:xdp_exception`（针对 `XDP_ABORTED`/`XDP_DROP` 的异常返回）——注意 `XDP_PASS` 之外的静默丢弃驱动统计可能都不更新。
</details>

<details>
<summary>Q2：为什么 NAT 至今留在 nftables，XDP 做不了？</summary>

NAT 的语义需要**连接状态**：同一 flow 的后续包要复用首包建立的地址映射，连接结束要回收映射。这套状态机就是 conntrack（`NF_IP_PRI_CONNTRACK = -200`，跑在所有 NAT 之前，且 NAT 链 priority 有「必须 > -200」的硬校验，`nf_tables_api.c:2251`）。

XDP 在 conntrack 之前（驱动层），**既读不到 conntrack 状态，也不该自己去建**（那是分布式状态，绕过 conntrack 会和其它 hook 打架）。技术上可以用 BPF map 自己维护 NAT 表，但等于重写半个 conntrack——没有工程理由。
</details>

<details>
<summary>Q3：白名单有 3000 个 CIDR 网段，方案 A 写 3000 条 nft 规则，方案 B 一个 interval set。差多少？</summary>

**方案 A**：每包线性扫 3000 条规则（不匹配也要逐条 `NFT_BREAK`），~3000 × 十几 cycle ≈ **4 万+ cycle**。而且规则插入越晚，平均扫描越长。

**方案 B**：一条规则 + `@whitelist`（interval set → rbtree，O(log n)≈11 次比较 ≈ **几十 cycle**）。集合内容更新不触碰规则链（`nft add element` 独立事务）。

差三个数量级。**规则数超过几十条就该 set 化**——这是 nftables 自己的设计哲学（见 [02](02-nftables-lwn.md) Q5）。
</details>

<details>
<summary>Q4：tc-BPF 能做 NAT 吗？位置上它在 POST_ROUTING 之后啊？</summary>

**位置上确实晚**（TX 路径：nft LOCAL_OUT → POST_ROUTING(SNAT) → nft netdev egress → tc egress → qdisc），tc-BPF 在 SNAT **之后**跑，看到的是改完源地址的包。

技术上有 `bpf_skb_store_bytes` 等能改包头，但没有 conntrack 联动：内核的连接跟踪表里记录的还是 SNAT 之前的映射，回包的 de-SNAT 不会按你 tc-BPF 的改法走。**改了就是自造黑洞**。结论同 Q2：NAT 是 nftables 的保留地。
</details>

<details>
<summary>Q5：行情机想给「非订阅组播组」的包早丢弃，但策略每周由 Python 服务下发。选什么？</summary>

**XDP + BPF map**：

- 位置：XDP 最早（省 skb 分配）——组播垃圾流量 pps 可能很高
- 策略下发：Python 服务通过 `bpftool map update` 或 libbpf 更新 devmap/hash map——**改 map 不需要重载程序**，原子生效
- nftables 也能做到类似位置（netdev ingress 链 + interval set 动态增删），但比 XDP 晚一个 skb 分配；如果策略就是「组播组地址列表」这种纯数据，nft netdev ingress 链 + set 也是完全合理的选择（运维更简单）

判断点：如果丢弃逻辑包含任何计算（如「同组但需要按内容分流」），只能 BPF；纯地址/端口列表则两者皆可，按运维能力选。
</details>

---

## 导航

- **上一篇：** [02-nftables-lwn.md](02-nftables-lwn.md) — nft 求值引擎与 set 后端（本篇成本对比的依据）
- **相关：** [chapter-05-xdp-architecture/](../../chapter-05-xdp-architecture/) XDP 架构 · [chapter-09-tc-bpf/](../../chapter-09-tc-bpf/) tc-BPF 双机制 · [chapter-11-packet-filter-flowtable/](../../chapter-11-packet-filter-flowtable/) flowtable（Netfilter 的快速路径——另一个「拿回性能」的答案）
- **章节主页：** [README](../README.md)
