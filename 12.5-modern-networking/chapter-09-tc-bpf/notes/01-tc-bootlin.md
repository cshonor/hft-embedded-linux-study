# 01 — Traffic Control：qdisc 体系与 HFT 出向调度

> **Bootlin 课程模块：** Traffic Control
> **对应 Rosen:** Ch6（Traffic Control）
> **内核版本：** 全部位置与参数基于 **v6.6** 源码核对

## 文档概述

本篇是 **TC（Traffic Control）子系统** 的总览与实操篇，聚焦**出向（egress）调度**——这是 Rosen Ch6 讲的东西在现代内核里的样子。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [02-tc-bpf.md](02-tc-bpf.md) | **tc-BPF 程序**：clsact / tcx、返回码、`__sk_buff` 可写性、81 个 helper | 本篇讲 **qdisc（排队规则）**，02 讲 **filter（分类器 + BPF）**。两者是 tc 的两半，容易混淆 |

**一句话区分**：**qdisc 决定「什么时候发」，filter/BPF 决定「这个包属于哪一类、要不要丢、要不要改」。** 本篇讲前者，02 讲后者。

---

## 1. ⚠️ 先纠正一张流传很广的错误图

常见画法（也出现在本章的旧版笔记里）：

```
❌ 错误：
发送方向:  socket → qdisc → class → filter → driver
```

**问题在于：filter 不在 qdisc 之后，也不在它之前——filter 是「挂」在 qdisc（或 clsact）上的，不是一个独立的流水线阶段。**

正确的 v6.6 出向路径（`__dev_queue_xmit()`，`net/core/dev.c` 约 4278-4360）：

```
sendmsg()/sendto()
      │
      ▼
  ip_queue_xmit() → 路由查找 → NF_HOOK(LOCAL_OUT) → ip_output()
      │
      ▼  NF_HOOK(POST_ROUTING) 的 okfn
  【cgroup egress BPF】ip_finish_output()      net/ipv4/ip_output.c:314/318
      │
      ▼
  邻居解析 → dev_queue_xmit()
      │
      ▼
╔═══════════════════════════════════════════════════════════════════════╗
║  __dev_queue_xmit()  net/core/dev.c:4278 起                             ║
║                                                                        ║
║  ① skb_reset_mac_header(); skb_assert_len()                            ║
║  ② if (SKBTX_SCHED_TSTAMP) __skb_tstamp_tx(...)   ← SW SCHED 时间戳     ║
║  ③ rcu_read_lock_bh()                                                  ║
║  ④ skb_update_prio(skb)     ← 从 sk->sk_priority 更新 skb->priority     ║
║  ⑤ qdisc_pkt_len_init(skb)                                             ║
║  ⑥ tcx_set_ingress(skb, false)                                         ║
║                                                                        ║
║  ⑦ if (egress_needed_key) {            /* CONFIG_NET_EGRESS */          ║
║       if (nf_hook_egress_active())                                     ║
║           skb = nf_hook_egress(skb, &rc, dev);   ← ★ Netfilter egress   ║
║       netdev_xmit_skip_txqueue(false);                                 ║
║       nf_skip_egress(skb, true);                                       ║
║       skb = sch_handle_egress(skb, &rc, dev);    ← ★★ tc egress         ║
║                                          net/core/dev.c:4311            ║
║       if (netdev_xmit_txqueue_skipped())                               ║
║           txq = netdev_tx_queue_mapping(dev, skb);                     ║
║     }                                                                  ║
║                                                                        ║
║  ⑧ skb_dst_drop() / skb_dst_force()                                    ║
║  ⑨ if (!txq) txq = netdev_core_pick_tx(dev, skb, sb_dev);  ← 选 TX 队列 ║
║  ⑩ q->enqueue() ... q->dequeue()                                       ║
║       └── 这里才是 prio / fq / fq_codel / tbf / etf 起作用的地方          ║
║  ⑪ ndo_start_xmit() → 网卡                                             ║
╚═══════════════════════════════════════════════════════════════════════╝
```

**从源码读出的五个结论（这些都是能改变工程决策的）：**

| # | 结论 | 源码依据 |
|---|------|---------|
| 1 | **tc egress 在 qdisc 之前**，不是在之后 | `sch_handle_egress()` 在 `dev.c:4311`，队列选择在 `dev.c:4328` |
| 2 | **Netfilter egress（nftables netdev family）在 tc egress 之前** | `nf_hook_egress()` 在 `sch_handle_egress()` 之上 |
| 3 | **`skb->priority` 在 tc egress 之前就已经由 socket 优先级设定** | `skb_update_prio()` 在步骤 ④ |
| 4 | **软件 SCHED 时间戳在 tc egress 之前打** | `__skb_tstamp_tx()` 在步骤 ② |
| 5 | **tc egress 可以改变 TX 队列选择** | 步骤 ⑦ 末尾的 `netdev_xmit_txqueue_skipped()` 分支 |

**结论 1 的工程含义（最重要）**：

> **tc egress 无法做「出向限速」。** 限速是 qdisc（tbf / fq / htb）的职责，而 tc egress 在 qdisc **之前**。
> 在 tc-BPF egress 里丢包可以实现「超出速率就丢」，但**做不了整形（shaping）**——包不会被延迟发送，只会被直接丢掉。
> 想要整形必须配 qdisc；tc-BPF 能做的是**给包打标记/分类**，然后由 qdisc 的 class（prio / htb / cbq）去分流。

**结论 5 的工程含义**：多队列网卡上，TX 队列的选择（`netdev_core_pick_tx`）发生在 tc egress **之后**。所以 tc egress 程序可以通过设置 `skb->queue_mapping` 影响最终队列——前提是该路径触发了 `netdev_xmit_txqueue_skipped()`。

---

## 2. qdisc 是什么：三个角色

TC 的构件只有三个，理清它们的关系，所有 tc 命令都能看懂：

| 构件 | 作用 | 例子 |
|------|------|------|
| **qdisc**（排队规则） | 决定包**如何排队、何时发送** | `pfifo_fast`、`fq`、`fq_codel`、`tbf`、`etf`、`prio`、`htb` |
| **class**（类别） | qdisc 内部的**分支**，每个分支可以挂子 qdisc | `htb` 的 `1:10` / `1:20`；`prio` 的 `1:1`~`1:3` |
| **filter**（分类器） | 判断「这个包属于哪个 class」或「执行什么动作」 | `u32`、`flower`、`bpf`（`cls_bpf`）、`fw`、`route` |

**关键区分（旧笔记漏掉的）：**

- **有类（classful）qdisc**：`prio`、`htb`、`cbq`、`dr`、`mqprio`。它们内部有 class，可以挂 filter 做分流，class 下面还能再挂 qdisc。
- **无类（classless）qdisc**：`pfifo_fast`、`fq`、`fq_codel`、`tbf`、`sfq`、`cake`。没有 class，**filter 挂在 qdisc 自身上**。
- **无队列 qdisc（用于 filter 挂载）**：`ingress`、`clsact`。**没有 enqueue/dequeue**，纯粹是为了提供 filter 挂载点。

### 2.1 `clsact` 与 `ingress`：为什么 clsact 是现在唯一该用的

v6.6 源码（`net/sched/sch_ingress.c`）：

```c
static struct Qdisc_ops ingress_qdisc_ops __read_mostly = {
	.cl_ops			=	&ingress_class_ops,
	.id			=	"ingress",
	.priv_size		=	sizeof(struct ingress_sched_data),
	.static_flags		=	TCQ_F_INGRESS | TCQ_F_CPUSTATS,
	.init			=	ingress_init,
	.destroy		=	ingress_destroy,
	.dump			=	ingress_dump,
	.ingress_block_set	=	ingress_ingress_block_set,
	.ingress_block_get	=	ingress_ingress_block_get,
	.owner			=	THIS_MODULE,
};
                                        /* ↑ 只有 ingress_block_*，没有 egress */

static struct Qdisc_ops clsact_qdisc_ops __read_mostly = {
	.cl_ops			=	&clsact_class_ops,
	.id			=	"clsact",
	.priv_size		=	sizeof(struct clsact_sched_data),
	.static_flags		=	TCQ_F_INGRESS | TCQ_F_CPUSTATS,
	.init			=	clsact_init,
	.destroy		=	clsact_destroy,
	.dump			=	ingress_dump,
	.ingress_block_set	=	clsact_ingress_block_set,
	.egress_block_set	=	clsact_egress_block_set,   /* ← 有 egress */
	.ingress_block_get	=	clsact_ingress_block_get,
	.egress_block_get	=	clsact_egress_block_get,   /* ← 有 egress */
	.owner			=	THIS_MODULE,
};
```

三点：

1. **两者都没有 `.enqueue` / `.dequeue` / `.peek`** → **不排队，不引入排队延迟**。旧 README 里这句是对的。
2. **`ingress` qdisc 只提供 ingress block**，`clsact` 同时提供 ingress 和 egress block。所以想挂 egress filter 必须用 `clsact`。
3. `MODULE_ALIAS("sch_clsact")`——模块名。

> **结论**：`tc qdisc add dev eth0 clsact` 是现在唯一正确的写法；`ingress` 是历史遗留（只支持 RX 方向）。

---

## 3. 常用 qdisc 全景（v6.6 实测存在）

核对方式：`net/sched/sch_*.c` 文件是否存在。

| qdisc | 源文件 | 类型 | 作用 | 排队延迟 | HFT 场景 |
|-------|--------|------|------|---------|---------|
| `pfifo_fast` | 内核默认 | 无类 | 3 个 band 的 FIFO（按 `skb->priority` 分） | 低（满队即丢） | 老默认，现代已被 fq_codel 取代 |
| `fq` | `sch_fq.c` (27 KB) | 无类 | **Flow Queue**：每流独立队列 + RR 调度 + pacing | **可控** | ⭐ **TCP 出向的标准选择** |
| `fq_codel` | `sch_fq_codel.c` (20 KB) | 无类 | fq + CoDel AQM（主动队列管理） | 低且自限 | 通用默认（多数发行版默认） |
| `tbf` | `sch_tbf.c` (15 KB) | 无类 | Token Bucket Filter，**硬限速 + 允许 burst** | 可控 | 限制非交易流量带宽 |
| `prio` | `sch_prio.c` (10 KB) | **有类**（3 band） | 严格优先级：band 0 空了才发 band 1 | 无（但有饥饿风险） | ⭐ 交易流高优先 |
| `etf` | `sch_etf.c` (12 KB) | 无类 | **Earliest TxTime First**：按 `SO_TXTIME` 截止时间发送 | 确定性 | ⭐⭐ **时间确定性发送（TSN）** |
| `mqprio` | `sch_mqprio.c` (21 KB) | 有类 | 把 traffic class 映射到硬件 TX 队列 | 取决于下层 | 多队列 + 优先级映射 |
| `cake` | `sch_cake.c` (81 KB) | 无类 | 综合 AQM + shaping + 流隔离 | 低 | 家用/边缘，非 HFT |
| `noqueue` | 内核内建 | 无类 | 直接发不排队 | 无 | loopback / 隧道设备默认 |

### 3.1 ⭐ `fq`：为什么它是 TCP 出向的默认推荐

`fq`（Fair Queue）的核心：每条流一个队列，轮转调度，并对每条流做 **pacing**（按 `flow_max_rate` 平滑发送）。

可调参数（`net/sched/sch_fq.c:786-800` 的 `fq_policy`）：

| 参数 | 含义 | 说明 |
|------|------|------|
| `plimit` | 总包数上限 | 默认 10000 |
| `flow_plimit` | **单流**包数上限 | 默认 100，防止单流占满队列 |
| `quantum` | 每轮给每条流补充的额度 | 默认 2×MTU（约 3028） |
| `initial_quantum` | 新建流的初始额度 | 默认 10×MTU |
| `rate_enable` | 是否启用 pacing | 需配合 `flow_max_rate` / `flow_default_rate` |
| `flow_default_rate` / `flow_max_rate` | 每流 pacing 速率 | — |
| `buckets_log` | 哈希表大小（log2） | 默认 10 左右 |
| `timer_slack` | timer 松弛 | — |

**HFT 用法**：

```bash
# TCP 出向用 fq，控制 burst 避免打爆交换机 buffer
tc qdisc replace dev eth0 root fq \
    flow_limit 100 \
    quantum 1514 \
    initial_quantum 15140
```

**为什么 burst 有害**：一次性把 100 个包丢给网卡，网卡/交换机的出端口 buffer 会积压，产生**排队延迟**。pacing 把它们摊平到时间轴上，单包延迟更稳定（代价是吞吐略有下降）。

### 3.2 ⭐⭐ `etf`：唯一能提供「确定性发送时刻」的 qdisc

`etf`（Earliest TxTime First）让你可以为**每个包**指定「不早于某个绝对时刻发送」，驱动按截止时间排序发送。这是 TSN（Time-Sensitive Networking）和硬件时间戳发射的基础。

**⚠️ 三条硬约束（`net/sched/sch_etf.c`）：**

```c
static inline int validate_input_params(struct tc_etf_qopt *qopt,
					struct netlink_ext_ack *extack)
{
	/* ...
	 *	* Dynamic clockids are not supported.
	 * ...
	 */
	if (qopt->clockid < 0) {
		NL_SET_ERR_MSG(extack, "Dynamic clockids are not supported");
		return -EINVAL;
	}
	if (qopt->clockid != CLOCK_TAI) {
		NL_SET_ERR_MSG(extack, "Invalid clockid. CLOCK_TAI must be used");
		return -EINVAL;
	}
```

1. **必须使用 `CLOCK_TAI`**（国际原子时），不是 `CLOCK_MONOTONIC` 也不是 `CLOCK_REALTIME`。这是 ETF 的硬校验，写错直接 `-EINVAL`。
2. **动态 clockid（`< 0`）不支持**。
3. **每个包的 socket 必须与 qdisc 配置匹配，否则包被丢弃**（`etf_enqueue()`，`:82-97`）：

```c
	if (q->skip_sock_check)
		return 0;                       /* 配置为跳过检查 */

	/* Drop if packet's clockid differs from qdisc's. */
	if (sk->sk_clockid != q->clockid)
		... drop ...
	if (sk->sk_txtime_deadline_mode != q->deadline_mode)
		... drop ...
```

**实操步骤：**

```bash
# 1. 挂 ETF（必须用 CLOCK_TAI，clockid 11）
tc qdisc replace dev eth0 root etf \
    clockid CLOCK_TAI \
    delta 200000 \          # 截止时间前 200us 开始准备（ns）
    deadline_mode on        # 可选

# 2. 应用侧必须设置 SO_TXTIME
#    struct sock_txtime sk_txt;
#    sk_txt.clockid = CLOCK_TAI;         /* 必须与 qdisc 一致 */
#    sk_txt.flags   = SK_TXTIME_DEADLINE_MODE;  /* 与 deadline_mode 一致 */
#    setsockopt(fd, SOL_SOCKET, SO_TXTIME, &sk_txt, sizeof(sk_txt));
#
#    然后每个包用 cmsg SCM_TXTIME 带上 uint64 的绝对 TAI 时刻（ns）

# 3. 硬件卸载（可选，需要网卡支持）
tc qdisc replace dev eth0 root etf clockid CLOCK_TAI delta 200000 offload
#   不支持时报错："Specified device does not support ETF offload"
```

**常见问题排查：**

| 现象 | 原因 |
|------|------|
| `Invalid clockid. CLOCK_TAI must be used` | 用了 `CLOCK_MONOTONIC`；**ETF 强制 CLOCK_TAI** |
| 配好 ETF 后发包全丢 | socket 没设 `SO_TXTIME`，或 `sk_clockid`/`deadline_mode` 与 qdisc 不一致 |
| 设了 `SO_TXTIME` 仍然丢 | 检查 `skip_sock_check` 是否开着（开着就不检查，但也就没有 txtime 语义） |
| `offload` 报错 | 网卡不支持 ETF 硬件卸载（需要 Intel i210/i225 这类 TSN 网卡） |

### 3.3 `prio`：最简单可靠的优先级隔离

```bash
# 3 个 band：band 0 最高，band 2 最低。只有高 band 空了才发低 band
tc qdisc add dev eth0 root handle 1: prio bands 3

# 默认 priomap：内核 skb->priority → band
#   0,1,2 → band 1 ; 3,4 → band 1 ; 5,6 → band 2 ; 7 → band 0 ...（可用 priomap 改）

# 用 u32 按端口分流
tc filter add dev eth0 parent 1: protocol ip prio 1 u32 \
    match ip dport 8001 0xffff flowid 1:1        # 交易流 → band 0
tc filter add dev eth0 parent 1: protocol ip prio 2 u32 \
    match ip dport 9001 0xffff flowid 1:3        # 行情流 → band 2
```

**⚠️ 饥饿风险**：`prio` 是严格优先级——只要 band 0 一直有包，band 2 一滴也发不出去。**必须确保高优先级流量有上限**（配合 tbf 或 fq 的下层 qdisc）。

### 3.4 `tbf`：限制非交易流量

```bash
# 在 prio 的 band 2（低优先）下挂 tbf 限速
tc qdisc add dev eth0 parent 1:3 handle 30: tbf \
    rate 10mbit burst 10kb latency 50ms
```
或
```bash
tc qdisc add dev eth0 parent 1:3 handle 30: tbf \
    rate 10mbit burst 10kb mtu 64kb peakrate 20mbit
```

**参数含义**：`rate` 长期平均速率；`burst` 允许的突发（必须 ≥ rate/HZ）；`latency` 包在队列里的最长停留时间（与 `limit` 二选一）。

---

## 4. HFT 出向配置模板

### 4.1 场景：交易流优先 + 行情流限速 + 其余默认

```bash
DEV=eth0

# 根 qdisc：prio 三档
tc qdisc replace dev $DEV root handle 1: prio bands 3

# band 0（1:1）：交易下单流，用 fq 做 pacing
tc qdisc add dev $DEV parent 1:1 handle 10: fq flow_limit 100

# band 2（1:3）：行情/后台流，用 tbf 限速
tc qdisc add dev $DEV parent 1:3 handle 30: tbf rate 50mbit burst 32kb latency 20ms

# 分流规则（用 BPF 分类器，见 02）
tc filter add dev $DEV parent 1: protocol ip prio 1 bpf da obj class.bpf.o sec cls
```

### 4.2 场景：时间确定性发送（交易流）

```bash
DEV=eth0
tc qdisc replace dev $DEV root etf clockid CLOCK_TAI delta 200000 deadline_mode on
# 应用侧配合 SO_TXTIME + SCM_TXTIME
```

### 4.3 观测

```bash
tc -s qdisc show dev eth0        # 每个 qdisc 的 sent/dropped/overlimits/requeries
tc -s filter show dev eth0       # filter 命中统计
tc -s class show dev eth0        # class 级统计（有类 qdisc）

# 排队延迟的直接观测（fq_codel / cake 有）
tc -s qdisc show dev eth0 | grep -i delay
```

**`dropped` 与 `overlimits` 的区别：**

| 计数 | 含义 |
|------|------|
| `dropped` | 队列满 / AQM 主动丢的包 |
| `overlimits` | 超过 limit 被延迟的次数（**不一定丢**） |
| `requeues` | 重新入队次数（驱动 `NETDEV_TX_BUSY` 时） |

> **`requeues` 是非零值时最值得警惕的信号**：说明驱动把包退回来了，通常意味着 TX ring 满。在高 PPS 场景这是常态，但数值持续飙升说明出向被打爆。

---

## 5. HFT 要点

1. **tc egress 在 qdisc 之前**——这是本篇最重要的位置认知。BPF 能做的是**分类 + 丢包**，**整形必须靠 qdisc**。
2. **`clsact` 是不带队列的 filter 挂载点**，用它挂 tc-BPF 不会引入排队延迟。但**它本身也不能替你整形**。
3. **默认 qdisc 值得检查**：多数发行版默认是 `fq_codel`，它带有 AQM 和流隔离，对延迟友好但对 HFT 未必最优（CoDel 会主动丢包）。交易机上建议显式设置。
4. **`fq` 的 pacing 是降低出向 burst 的主要手段**。burst 打爆交换机 buffer 会造成几十微秒甚至毫秒级的排队抖动，这比 CPU 上的优化重要得多。
5. **`etf` 是唯一能给「确定性发送时刻」的 qdisc**，但硬约束多：必须 `CLOCK_TAI`、socket 必须 `SO_TXTIME`、硬件卸载需要 TSN 网卡。三条任一不匹配就是**静默丢包**。
6. **`prio` 会饥饿低优先级队列**。永远给高优先级流量配上限速（下层 tbf / fq）。
7. **多队列网卡上，TX 队列选择（`netdev_core_pick_tx`）发生在 tc egress 之后**，所以 tc egress 里设置 `skb->queue_mapping` 可以影响最终队列——但只在 `netdev_xmit_txqueue_skipped()` 为真时生效。

---

## 6. 与 Rosen Ch6 的差异

| 维度 | Rosen 3.x | v6.6 |
|------|-----------|------|
| 默认 qdisc | `pfifo_fast` | 多数发行版 `fq_codel` |
| 分类器 | `u32` / `fw` / `route`（classic BPF） | `flower` / **`cls_bpf`（eBPF）** 为主 |
| egress filter | 无（`ingress` qdisc 只有 RX） | **`clsact` 提供 ingress + egress 两个 block** |
| 时间确定性发送 | 无 | **`etf` + `SO_TXTIME`** |
| 硬件卸载 | 无 | `mqprio` / `etf offload`（TSN） |
| 出向 hook 顺序 | 只有 qdisc | **Netfilter egress → tc egress → qdisc → 驱动** |
| 配置工具 | `tc`（iproute2） | 同为 `tc`，但 `bpf da` 模式 + v6.6 的 tcx |

Rosen 讲的 qdisc/class/filter 三元模型**完全没变**，变化的是：分类器主流从 u32 换成了 BPF，多出了 egress 方向，多出了时间确定性发送。

---

## 7. 代码自测

<details>
<summary>Q1：你想在 tc-BPF egress 程序里实现「限制交易进程出向带宽到 100 Mbps」。你写好了令牌桶逻辑，超过速率就返回 <code>TC_ACT_SHOT</code>。上线后发现：带宽确实被限住了，但对端看到的是「大量的重传和吞吐抖动」，而不是平滑的 100 Mbps。为什么？</summary>

**因为你做的是 policing（ policing = 超了就丢），不是 shaping（整形 = 超了就排队延迟）。**

看 `__dev_queue_xmit()` 的顺序（`net/core/dev.c`）：

```
⑦ sch_handle_egress(skb, &rc, dev);     /* ← 你的程序在这里，dev.c:4311 */
⑨ txq = netdev_core_pick_tx(dev, skb, sb_dev);
⑩ q->enqueue() → q->dequeue()           /* ← tbf/fq 在这里 */
⑪ ndo_start_xmit() → 网卡
```

**tc egress 在 qdisc 之前。** 你的程序执行时，包还没进任何队列。你返回 `TC_ACT_SHOT` 时：

```c
	case TC_ACT_SHOT:
		kfree_skb_reason(skb, SKB_DROP_REASON_TC_EGRESS);
		*ret = NET_XMIT_DROP;
		return NULL;
```

——包**直接被 free**，根本没有「等一会儿再发」这个选项。

**对 TCP 的后果**：突发 200 Mbps → 你丢掉一半 → TCP 看到的是**随机丢包**（不是 ECN、不是主动队列管理），触发超时重传或快速重传，拥塞窗口剧烈收缩 → 吞吐抖动、延迟尖刺。这正是你看到的现象。

**正确做法：把限速交给 qdisc。**

```bash
# tbf 做整形：超出 rate 的包会被延迟（排队），而不是丢弃
tc qdisc add dev eth0 root handle 1: prio
tc qdisc add dev eth0 parent 1:1 handle 10: tbf \
    rate 100mbit burst 32kb latency 20ms
```

tbf 的 `latency` 参数就是「包最多在队列里待多久」——这是 shaping 的核心：**用延迟换平滑**。

**那 tc-BPF 在出向该干什么？**

| 你想做的事 | 该用什么 |
|-----------|---------|
| 分类（这个包属于哪个 class/band） | **tc-BPF**：设 `skb->mark` / `skb->priority`，返回 `TC_ACT_OK`，交给 `prio` qdisc 分流 |
| 丢包（确定不要的包） | **tc-BPF**：`TC_ACT_SHOT` |
| 改包内容 / 重定向 | **tc-BPF** |
| **限速 / 整形 / pacing** | **qdisc**：`tbf` / `fq` / `fq_codel` |
| **确定性发送时刻** | **qdisc**：`etf` + `SO_TXTIME` |

**组合用法**（标准做法）：

```bash
tc qdisc add dev eth0 root handle 1: prio
tc qdisc add dev eth0 parent 1:1 handle 10: tbf rate 100mbit burst 32kb latency 20ms
# tc-BPF 只负责打标记，prio 按 skb->priority 选 band
tc filter add dev eth0 parent 1: protocol all prio 1 bpf da obj cls.bpf.o sec cls
```

BPF 里：

```c
SEC("cls")
int cls_prog(struct __sk_buff *skb)
{
	if (is_trading_flow(skb)) {
		skb->priority = 1;      /* → prio 的 band 0 */
		return TC_ACT_OK;
	}
	skb->priority = 6;              /* → prio 的 band 2（被 tbf 限速） */
	return TC_ACT_OK;
}
```

**注意**：`skb_update_prio(skb)` 在 tc egress **之前**（`dev.c:4293`）已经把 socket 的 `SO_PRIORITY` 写进 `skb->priority` 了。你在 tc egress 里覆盖它会生效，但要考虑是否会破坏上层已经设定的优先级。

</details>

<details>
<summary>Q2：<code>tc qdisc add dev eth0 ingress</code> 和 <code>tc qdisc add dev eth0 clsact</code> 有什么区别？为什么现在都用 clsact？</summary>

**源码级区别**（`net/sched/sch_ingress.c`）：

```c
static struct Qdisc_ops ingress_qdisc_ops __read_mostly = {
	.id			=	"ingress",
	.static_flags		=	TCQ_F_INGRESS | TCQ_F_CPUSTATS,
	...
	.ingress_block_set	=	ingress_ingress_block_set,
	.ingress_block_get	=	ingress_ingress_block_get,
	/* ← 只有 ingress block */
};

static struct Qdisc_ops clsact_qdisc_ops __read_mostly = {
	.id			=	"clsact",
	.static_flags		=	TCQ_F_INGRESS | TCQ_F_CPUSTATS,
	...
	.ingress_block_set	=	clsact_ingress_block_set,
	.egress_block_set	=	clsact_egress_block_set,   /* ← 多了这个 */
	.ingress_block_get	=	clsact_ingress_block_get,
	.egress_block_get	=	clsact_egress_block_get,   /* ← 多了这个 */
};
```

| | `ingress` | `clsact` |
|---|-----------|----------|
| ingress filter | ✅ | ✅ |
| **egress filter** | ❌ **不支持** | ✅ |
| 排队（enqueue/dequeue） | ❌ 无 | ❌ 无 |
| `TCQ_F_INGRESS` 标记 | ✅ | ✅ |
| 引入排队延迟 | 无 | 无 |

**三个要点：**

1. **只有 `clsact` 能挂 egress filter。** 这是选它的首要原因。想做出向的 BPF 处理（打标记、丢包、重定向、改包）必须用 `clsact`。

2. **两者都不排队。** Qdisc_ops 里**没有 `.enqueue` / `.dequeue` / `.peek` 成员**，所以它们纯粹是 filter 的挂载点，不引入任何排队延迟。旧 README 里「clsact qdisc：无队列分类器，ingress/egress hook 不引入排队延迟」这句话是对的，但**要理解成「它不排队」，不是「它替你整形」**。

3. **`TCQ_F_INGRESS` 这个 flag 容易让人误解**：`clsact` 也带这个 flag（因为它是从 ingress qdisc 演化来的），但它同时支持 egress 方向。flag 名字是历史遗留。

**实操：**

```bash
# 挂 clsact（一个 qdisc 同时提供 ingress + egress 两个 block）
tc qdisc add dev eth0 clsact

tc filter add dev eth0 ingress bpf da obj p.bpf.o sec rx
tc filter add dev eth0 egress  bpf da obj p.bpf.o sec tx

tc filter show dev eth0 ingress
tc filter show dev eth0 egress

# 删除（会同时摘掉 ingress 和 egress 上的所有 filter）
tc qdisc del dev eth0 clsact
```

**⚠️ 一个常见错误**：`tc qdisc del dev eth0 clsact` 会**一次性**摘掉两个方向的所有 filter。生产环境做变更时要意识到这个操作的爆炸半径。

**另外，v6.6 起还有第三条路——tcx**（`BPF_TCX_INGRESS` / `BPF_TCX_EGRESS`，`include/uapi/linux/bpf.h:1040-1041`）：

```bash
bpftool prog load p.bpf.o /sys/fs/bpf/p
bpftool prog attach id <PROG_ID> tcx_ingress dev eth0
bpftool link show dev eth0
```

tcx 走 **bpf_link 语义**（可 pin、进程退出自动 detach、`bpftool link show` 可见），且**不需要先创建 clsact qdisc**。详见 [02-tc-bpf.md](02-tc-bpf.md) 第 2 节。

</details>

<details>
<summary>Q3：你配好了 <code>etf</code> qdisc 和应用的 <code>SO_TXTIME</code>，但一个包都发不出去，<code>tc -s qdisc show</code> 里 dropped 一直在涨。列出至少 3 个可能原因。</summary>

**`etf` 的失败模式几乎全是「静默丢包」**——包进 `etf_enqueue()` 后因为不匹配被直接丢，没有任何 errno 反馈给应用。

**原因 1：用了错误的 clockid（最常见的坑）。**

ETF **强制 `CLOCK_TAI`**，硬校验在 `net/sched/sch_etf.c`：

```c
	if (qopt->clockid < 0) {
		NL_SET_ERR_MSG(extack, "Dynamic clockids are not supported");
		return -EINVAL;
	}
	if (qopt->clockid != CLOCK_TAI) {
		NL_SET_ERR_MSG(extack, "Invalid clockid. CLOCK_TAI must be used");
		return -EINVAL;
	}
```

注意：**如果你用 `CLOCK_MONOTONIC`，qdisc 创建就会直接失败**（不会到丢包阶段）。所以如果 qdisc 创建成功了但还丢包，clockid 错的可能可以排除——但要确认**应用侧的 `sk_clockid` 也是 `CLOCK_TAI`**。

**原因 2：socket 的 clockid / deadline_mode 与 qdisc 不一致。**

`etf_enqueue()`（`sch_etf.c:82-97`）：

```c
	if (q->skip_sock_check)
		return 0;

	/* Drop if packet's clockid differs from qdisc's. */
	if (sk->sk_clockid != q->clockid)
		... drop ...
	if (sk->sk_txtime_deadline_mode != q->deadline_mode)
		... drop ...
```

**三个必须完全对齐的设置：**

| 位置 | 设置 |
|------|------|
| qdisc | `tc qdisc ... etf clockid CLOCK_TAI deadline_mode on` |
| socket | `setsockopt(fd, SOL_SOCKET, SO_TXTIME, &sk_txt, sizeof)` 里 `sk_txt.clockid = CLOCK_TAI` |
| socket | `sk_txt.flags = SK_TXTIME_DEADLINE_MODE`（**必须与 qdisc 的 `deadline_mode on/off` 一致**） |

三者任一不匹配 → 包被丢。

**应用侧正确写法：**

```c
struct sock_txtime sk_txt = {
	.clockid = CLOCK_TAI,                  /* ← 必须 TAI */
	.flags   = SK_TXTIME_DEADLINE_MODE,    /* ← 与 qdisc 的 deadline_mode 对齐 */
};
setsockopt(fd, SOL_SOCKET, SO_TXTIME, &sk_txt, sizeof(sk_txt));

/* 每个包：用 cmsg 带上绝对截止时间（ns，CLOCK_TAI 时基） */
char cbuf[CMSG_SPACE(sizeof(uint64_t))];
struct cmsghdr *cm = (struct cmsghdr *)cbuf;
cm->cmsg_level = SOL_SOCKET;
cm->cmsg_type  = SCM_TXTIME;
cm->cmsg_len   = CMSG_LEN(sizeof(uint64_t));
uint64_t txtime = tai_now_ns() + 50000;    /* 50us 之后发 */
memcpy(CMSG_DATA(cm), &txtime, sizeof(txtime));

struct msghdr msg = { .msg_control = cbuf, .msg_controllen = CMSG_SPACE(sizeof(uint64_t)) };
sendmsg(fd, &msg, 0);
```

**原因 3：截止时间已经过去了（或用错了时基）。**

即使 clockid 都是 `CLOCK_TAI`，如果你算出来的 `txtime` 是 `CLOCK_MONOTONIC` 或 `CLOCK_REALTIME` 的时基，数值会差出几十亿纳秒（TAI 与 UTC 差 37 秒的闰秒偏移，与 MONOTONIC 差整个 boot 时间）。结果是要么「截止时间远在未来」（包永远不发），要么「已过期」（立即丢/立即发）。

**验证时基：**

```bash
# 读当前 CLOCK_TAI（需要 CLOCK_TAI 支持）
# 用 clock_gettime(CLOCK_TAI, &ts) 而不是 gettimeofday / CLOCK_MONOTONIC
```

**原因 4：`offload` 配了但网卡不支持。**

```c
static int etf_enable_offload(struct net_device *dev, struct etf_sched_data *q,
			      struct netlink_ext_ack *extack)
{
	...
	if (!ops->ndo_setup_tc) {
		NL_SET_ERR_MSG(extack, "Specified device does not support ETF offload");
		...
```

这会直接报错（不静默），但错误信息容易被忽略。硬件卸载需要 TSN 网卡（Intel i210 / i225 等）。

**原因 5：`delta` 设置不合理。**

`delta` 是「截止时间前多久开始准备」。设置过小（< 驱动准备时间）会导致错过截止时间；某些驱动实现会因此丢包。通常从 100–200 µs 起调。

**排查清单：**

```bash
# 1. 确认 qdisc 参数
tc qdisc show dev eth0
#   应看到：etf ... clockid CLOCK_TAI delta ... deadline_mode

# 2. 看丢包计数
tc -s qdisc show dev eth0
#   dropped 持续增长 = enqueue 阶段被丢

# 3. 内核日志
dmesg | grep -i etf

# 4. 拿掉 skip_sock_check 再试（排除配置不匹配）
tc qdisc replace dev eth0 root etf clockid CLOCK_TAI delta 200000
#   然后确认应用侧的 SO_TXTIME 设置
```

</details>

---

## 导航

- **下一篇：** [02-tc-bpf.md](02-tc-bpf.md) — tc-BPF 程序：v6.6 tcx 机制、返回码、`__sk_buff` 可写性、81 个 helper
- **相关：** [chapter-03-tx-path-skbbuff/](../../chapter-03-tx-path-skbbuff/) 发包路径与 skb · [chapter-08-ebpf-cgroup-bpf/](../../chapter-08-ebpf-cgroup-bpf/) eBPF 框架 · [chapter-15-debugging-perf-tuning/](../../chapter-15-debugging-perf-tuning/) 延迟测量
- **章节主页：** [README](../README.md)
