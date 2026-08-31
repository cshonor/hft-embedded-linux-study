# 01 — Linux 网络栈架构与 hook 点顺序

> **Bootlin 课程模块：** Network Stack Architecture
> **对应 Rosen:** Ch1（概述）
> **内核源码路径：** `net/core/dev.c`、`net/socket.c`、`include/linux/netdevice.h`、`include/linux/skbuff.h`

## 文档概述

原笔记给了全景图和结构表，但**图里 XDP 的位置画错了** ——
它被画在协议栈旁边，好像是一个平行的旁路选项。实际上：

> **XDP 在 `sk_buff` 分配之前执行。** 那一刻还没有 skb，
> 所以 XDP 拿到的不是 skb 而是 `xdp_buff` —— 这是它能做到"零分配丢包"的根本原因。

本篇的核心产出是一张**hook 点顺序表**：包从网卡进来，依次经过哪些可干预点、
每个点能看到什么、适合做什么。所有优化手段都得挂到具体某一点上。

---

## 一、全景：收与发两条路径

```
═════════════════════════════ 收包 RX ═════════════════════════════

  NIC ──DMA──> Rx ring
    │
    ├─[H1] XDP（驱动层）        ← 最早点。**此时还没有 sk_buff**
    │        └─ DROP / TX / REDIRECT / PASS
    │                              （只是 page_pool 的一页 + 描述符）
    ↓ (XDP_PASS)
  napi_build_skb()  ← sk_buff 在这里才诞生
    │
  napi_gro_receive()   ── GRO 聚合（等合并窗口 = 延迟）
    │
  __netif_receive_skb_core()      net/core/dev.c:5330
    ├─[H2] ptype_all（AF_PACKET → tcpdump 在这里抓包）   dev.c:5394/5400
    ├─[H3] tc ingress（clsact/ingress qdisc）           dev.c:5412
    ├─    Netfilter ingress（nf_ingress）                dev.c:5420
    ├─    rx_handler（bridge / bonding）                 dev.c:5440
    └─[H4] ptype_base 分发：ETH_P_IP → ip_rcv()          dev.c:5504
    ↓
  ip_rcv() → ip_rcv_core()        net/ipv4/ip_input.c
    ├─[H5] Netfilter PRE_ROUTING（nftables 挂载点）
    └─ ip_route_input() 路由查找
         ├─ 本机  → ip_local_deliver()
         │            └─[H6] Netfilter LOCAL_IN
         │                 → udp_rcv() / tcp_v4_rcv()
         │                     → sock_queue_rcv_skb() → sk_data_ready()
         │                         → 唤醒进程 → copy_to_user
         │
         └─ 转发  → [H7] Netfilter FORWARD
                      → [H8] Netfilter POST_ROUTING
                          → [H9] tc egress → dev_queue_xmit()

═════════════════════════════ 发包 TX ═════════════════════════════

  应用 send()/sendmsg()
    ↓
  sock_sendmsg() → udp_sendmsg()        net/ipv4/udp.c
    ↓
  ip_send_skb() → ip_local_out()        net/ipv4/ip_output.c
    ├─[H10] Netfilter LOCAL_OUT
    ↓
  ip_output()
    ├─[H11] Netfilter POST_ROUTING
    ↓
  ip_finish_output() → ip_finish_output2() → neigh_resolve_output()
    ↓
  dev_queue_xmit() → __dev_queue_xmit()   net/core/dev.c
    ├─[H12] tc egress（qdisc：fq_codel / mqprio / ETF...）
    ↓
  dev_hard_header() → ndo_start_xmit()
    ↓
  NIC ──DMA──> Tx ring ──> 线上
```

---

## 二、hook 点顺序表（本篇核心）

| # | hook | 位置 | 能看到 | 适合做什么 | 时机 |
|---|------|------|--------|-----------|------|
| H1 | **XDP** | 驱动 Rx，skb 之前 | `xdp_buff`（裸帧） | 丢包、采样、重定向 | **最早，最便宜** |
| H2 | **AF_PACKET（tcpdump）** | `ptype_all`，dev.c:5394 | skb | **抓包、观测** | 在 tc ingress **之前** |
| H3 | tc ingress | `sch_handle_ingress()`，dev.c:5412 | skb | 限速、过滤、镜像 | 在 tcpdump **之后** |
| H3b | Netfilter ingress | `nf_ingress()`，dev.c:5420 | skb | nftables ingress | 在 tc ingress 之后 |
| H4 | ptype 分发 | `ptype_base[]`，dev.c:5504 | skb | 协议注册 | 内核内部分发点 |
| H5 | NF PRE_ROUTING | `ip_rcv()` | skb | nftables 过滤/NAT | 路由前 |
| H6 | NF LOCAL_IN | `ip_local_deliver()` | skb | 本机入站过滤 | 路由后、L4 前 |
| H7 | NF FORWARD | `ip_forward()` | skb | 转发过滤 | 仅转发路径 |
| H8 | NF POST_ROUTING | `ip_output()` | skb | SNAT | 出去前最后一点 |
| H9/H12 | **tc egress** | `__dev_queue_xmit()` | skb | 排队、整形、ETF 定时发送 | 驱动之前 |
| H10 | NF LOCAL_OUT | `ip_local_out()` | skb | 本机出站过滤 | 路由后 |
| H11 | NF POST_ROUTING | `ip_output()` | skb | 同上 | — |

**三个容易记错的顺序：**

1. **XDP 在 skb 之前，tc ingress 在 skb 之后** —— 所以 XDP 能省掉 skb 分配（100–200ns），
   tc 不能。
2. **tcpdump（AF_PACKET / `ptype_all`）在 tc ingress 之*前*** —— ⚠️ 这条极易记反。
   内核源码 `net/core/dev.c` 的顺序是：generic XDP（dev.c:5373）→ `ptype_all` 分发
   （dev.c:5394，tcpdump 在这里拿到包）→ `skip_taps:` → `sch_handle_ingress()`
   （dev.c:5412，tc ingress 在这里）。所以：
   - 在 **tc ingress** 里丢的包，**tcpdump 能看到**（抓到之后才被丢）
   - `XDP_DROP` 的包 **tcpdump 看不到**（那时连 skb 都没有，AF_PACKET 无从挂钩）
   - 这个差别正好是排障利器：tcpdump 抓到但应用没收到 → 丢点在 tcpdump 之后
     （tc ingress / Netfilter / socket 队列）；tcpdump 根本没抓到 → 丢点在更前面
     （驱动 / XDP / 网卡）。
3. **tc egress 在驱动之前** —— 所以 ETF（Earliest TxTime First）qdisc 才能做到
   "精确到纳秒的发送时刻控制"，HFT 的发送时序整形靠它。而且 6.x 在 tc egress
   **之前**还多了一个 `nf_hook_egress()`（dev.c:4307），发包侧其实是
   「Netfilter egress → tc egress → qdisc → 驱动」三层。

---

## 三、核心数据结构

| 结构 | 定义 | 关键字段/语义 |
|------|------|--------------|
| `struct net_device` | `include/linux/netdevice.h` | `napi_list`、`rx_queue`、`features`（offload 开关位）、`xdp_prog` |
| `struct sk_buff` | `include/linux/skbuff.h` | `head/data/tail/end`、`len` vs `data_len`、`frags[]`、`napi_id`、`skb_iif` |
| `struct xdp_buff` | `include/linux/bpf.h` | 只有 `data`/`data_end`/`data_meta`/`rxq` —— **没有 skb 那几十个字段** |
| `struct sock` | `include/net/sock.h` | `sk_receive_queue`、`sk_data_ready`、`sk_napi_id`、`sk_reuseport_cb` |
| `struct net` | `include/net/net_namespace.h` | 网络命名空间，容器隔离的基础 |

### `sk_buff` 与 `xdp_buff` 的分工

```
        sk_buff                          xdp_buff
  ┌──────────────────┐            ┌──────────────────┐
  │ 几十个字段        │            │ data             │
  │ 协议头指针        │            │ data_end         │
  │ frags[] 非线性区  │            │ data_meta        │
  │ 引用计数、时间戳  │            │ rxq (queue_index)│
  │ dst/路由缓存      │            └──────────────────┘
  │ socket 关联       │              v6.6 共 8 个字段（4.8 引入时只有 4 个，
                               后来加了 txq / frame_sz / flags）
  └──────────────────┘              分配成本 ≈ 0
   分配成本 100-200ns
```

**这就是为什么 XDP_DROP 比"进协议栈再丢"快一个量级**：
它连 skb 都没建，直接把 page 还给 page_pool。

### `sk_buff` 的线性区陷阱

```c
unsigned int len;       /* 数据总长（含非线性区） */
unsigned int data_len;  /* 非线性区长度 */
/* 线性区长度 = len - data_len */
```

`skb->data` 只有在线性区才有意义。**别假设包是连续的**：

```c
/* 错误：GRO 合并 / 分片 / 非线性 skb 下会读飞 */
memcpy(buf, skb->data, skb->len);

/* 正确 */
skb_copy_bits(skb, 0, buf, skb->len);
/* 或者只读取头部时用（会自动处理非线性） */
iph = skb_header_pointer(skb, 0, sizeof(*iph), &_iph);
```

> 这条与 13-dpdk `mcast-minimal/src/main.c` 里 `m->nb_segs != 1` 的检查
> 是同一个道理：**DPDK 的 mbuf 和内核的 skb 都可能不是连续的一块内存**。

---

## 四、子系统与源码位置

| 子系统 | 源码 | 对应 Rosen | 本章后续 |
|--------|------|-----------|---------|
| socket layer | `net/socket.c`、`net/core/sock.c` | Ch11 | 03.5-unix-network-api |
| L4 (TCP/UDP) | `net/ipv4/tcp*.c`、`udp.c` | Ch4/Ch11 | [ch14](../../chapter-14-tcp-udp-internals/) |
| L3 (IP/路由) | `net/ipv4/ip_input.c`、`route.c`、`fib_*.c` | Ch5/Ch6 | — |
| Netfilter/nftables | `net/netfilter/`、`net/netfilter/nf_tables_*.c` | Ch9 | [ch10](../../chapter-10-nftables/) |
| Traffic Control | `net/sched/` | Ch6 | [ch09](../../chapter-09-tc-bpf/) |
| XDP | `net/core/filter.c`、`kernel/bpf/devmap.c` | 无 | [ch05](../../chapter-05-xdp-architecture/) |
| NAPI | `net/core/dev.c` | Ch1/Ch14 | [ch02](../../chapter-02-napi-rx-path/) |
| page_pool | `net/core/page_pool.c` | 无 | [ch04](../../chapter-04-page-pool/) |

---

## 五、四种"少走几层"的旁路

| 方案 | 跳过什么 | 保留什么 | 代价 |
|------|---------|---------|------|
| **XDP** | 部分（skb 分配之前丢包） | 网卡仍属内核 | 要写 BPF |
| **AF_XDP** | skb + 协议栈（按队列） | 网卡仍属内核，其它队列正常 | 独占队列 |
| **DPDK** | 全部 | 无（内核看不到这张网卡） | **失去 TCP/路由/SSH** |
| **nf_flowtable** | Netfilter 规则遍历 | 内核栈其余部分 | 仅对已建立的流生效 |

```
                    协议栈参与度
  完整内核栈  ████████████████  功能全，延迟 10-50μs
  nf_flowtable█████████████    快速路径，跳过规则遍历
  XDP         ████████          skb 之前可丢/可转
  AF_XDP      ████              skb+协议栈都不过，但网卡还是内核的
  DPDK        ░                 内核完全不管这张卡
```

选择决策树 → [../../chapter-07-xdp-redirect-dpdk/](../../chapter-07-xdp-redirect-dpdk/)

---

## 六、观测：确认包走了哪条路

```bash
# XDP 是否真的挂上了
ip link show dev eth0 | grep -i xdp
bpftool net show dev eth0

# tc 过滤规则命中计数（hwt/sent/drop）
tc -s filter show dev eth0 ingress
tc -s qdisc show dev eth0

# nftables 规则计数（确认走了 fastpath 还是 slowpath）
nft list ruleset -a

# 丢包在哪一层？（kfree_skb 的 reason，5.15+）
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'
#   或 4.x/5.x 老内核用：
bpftrace -e 'kprobe:kfree_skb { @[kstack()] = count(); }'
```

`kfree_skb` 的 `reason` 是定位"包在哪一层被丢"最直接的信号，
5.15+ 才有；老内核要用栈回溯替代。

---

## HFT 要点

- **hook 越早，能省的成本越多，但能看到的信息越少**：
  XDP 看不到 socket、看不到连接状态，只有裸帧
- **"最早"不等于"最合适"**：需要连接状态、需要重组的逻辑，只能往后放
- **XDP_DROP 的包 tcpdump 抓不到** —— 排障时先把 XDP 摘掉，或用
  `XDP_ABORTED` + tracepoint 观察
- **tc egress 是发送时序的控制点**（ETF qdisc），报单的发送时刻整形靠它
- **别把所有过滤都堆在 Netfilter** —— 那是成本最高的位置之一，
  能在 XDP 做的就别往后放

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| 最早干预点 | 无（只能进协议栈） | **XDP**（skb 之前） |
| 包结构 | 只有 `sk_buff` | `sk_buff` + **`xdp_buff`** 双轨 |
| 过滤框架 | iptables | **nftables** + bpf + flowtable |
| 旁路 | 无 | XDP_REDIRECT / **AF_XDP** / DPDK |
| 命名空间 | 有 | 更完善，容器网络基础 |
| 丢包诊断 | 靠计数 | `kfree_skb` 带 reason（5.15+） |

## 代码自测

<details>
<summary>Q1：XDP 到底在 sk_buff 之前还是之后？为什么说它"零拷贝"？</summary>

**在之前。** 驱动 poll 从 Rx ring 拿到的是 DMA 页和描述符，
此时可以直接构造 `xdp_buff`（**栈上的**临时结构，v6.6 是 8 个字段，**无任何分配**）交给 BPF 程序。
只有返回 `XDP_PASS` 时，内核才调用 `napi_build_skb()` 把这页包装成 `sk_buff`。

至于"零拷贝"这个说法要小心：XDP 并没有省掉 DMA（DMA 本来就不经 CPU），
它省掉的是 **sk_buff 分配 + 后续的协议栈遍历和拷贝**。
内核栈版也有 DMA，所以"零拷贝"指的是**相对内核栈少几次拷贝**，不是绝对零次。
→ [13-dpdk/chapter-04-零拷贝与用户态旁路](../../../13-dpdk/01-Intro-Book/notes/chapter-04-零拷贝与用户态旁路.md)
</details>

<details>
<summary>Q2：我在 tc ingress 里 DROP 了一个包，tcpdump 还能看到它吗？在 XDP 里 DROP 呢？</summary>

- **tc ingress DROP**：**能看到**。tcpdump 的底层是 AF_PACKET，挂在 `ptype_all`；
  它在 `net/core/dev.c:5394` 分发，而 tc ingress 的 `sch_handle_ingress()`
  在 `dev.c:5412`——**tcpdump 先拿到包，tc 才丢**。
  （这条极易记反，我第一次也写错了，后来查 v6.6 源码才改正。）
- **XDP DROP**：**看不到**。那时连 `sk_buff` 都还没分配，AF_PACKET 无从挂钩。

**排障结论**：正好可以用这个差别做二分定位。

| 现象 | 丢点在哪 |
|------|---------|
| tcpdump **能**抓到，应用没收到 | tcpdump 之后：tc ingress / Netfilter / 路由 / socket 队列 |
| tcpdump **抓不到**，但驱动计数在涨 | tcpdump 之前：驱动 / XDP / 硬件 GRO |

抓不到包不等于没收到。要确认"包到底进没进网卡"，看**驱动层计数**和 **XDP 自己的计数**：

```bash
ethtool -S eth0 | grep -E 'rx_packets|missed'
bpftool prog show id <id>   # 看 XDP 程序的 run_time / 计数
```

⚠️ 另有一个会破坏这套推理的陷阱：**开了 `rx-gro-hw` 之后，包在进内核之前就被网卡合并了**，
tcpdump 看到的是合并后的"假包"，上面这张表就不成立了。排障期间先关掉硬件 GRO。
→ 详见 [chapter-02/04-gro-gso](../../chapter-02-napi-rx-path/notes/04-gro-gso.md)
</details>

<details>
<summary>Q3：`skb->len` 和 `skb->data_len` 有什么区别？什么时候必须注意？</summary>

- `len`：整个 skb 的数据总长（线性区 + 非线性区）
- `data_len`：**非线性区**长度（存在 `frags[]` 页引用里，不在 `skb->data` 后面）
- 线性区 = `len - data_len`

必须在意的场景：

1. **GRO 合并后** —— 大概率是非线性的
2. **IP 分片重组** —— 一定是非线性的
3. **大于一页的包** —— 放不下就进 frags

在这些包上直接 `memcpy(dst, skb->data, skb->len)` 会读到 `skb->data` 后面的
**未映射内存**（不是"读到错数据"，是可能直接崩）。
正确做法用 `skb_copy_bits()` 或 `skb_header_pointer()`。
</details>
