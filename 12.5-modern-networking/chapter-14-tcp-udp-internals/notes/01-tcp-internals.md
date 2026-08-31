# 14-01 — TCP 内部优化：TSO / TSQ / Pacing(EDT) / RACK（v6.6 源码级）

> **对应 Rosen:** Ch11（TCP 基础实现）
> **内核源码路径:** `net/ipv4/tcp_output.c`、`net/ipv4/tcp_input.c`、`net/ipv4/tcp_recovery.c`、`net/sched/sch_fq.c`

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [chapter-13 零拷贝](../../chapter-13-zerocopy-highperf/README.md) | **14-01 TCP 内部** | [14-02 UDP GRO](02-udp-gro.md) |

## 本节讲什么

Rosen 时代（3.x 前后）的 TCP 发送路径是"cwnd 控制总量 + 中断驱动发包"。v6.6 的现实是四个相互咬合的机制：**TSO**（大段下推网卡分段）、**TSQ**（发送队列内存上限防 burst）、**EDT pacing**（把发送时刻写进每个 skb，由 fq qdisc 严格执行）、**RACK**（基于时间的丢包检测替代 3-dup-ACK）。本篇用源码把这四个机制各自的位置、相互作用和对延迟的真实影响讲清楚——其中有两个流传甚广的说法（"TSO 要等凑满 64KB"、"关 TSO 降延迟"）需要修正。

## 要点（先记住结论）

1. **TSO 不等待凑包**：`tcp_write_xmit()`（tcp_output.c:2670）发送的是**发送队列里现成的数据**——TCP_NODELAY 下 `tcp_push` 有多少发多少，TSO 只是把"同一时刻排队的多段"打包成一个 64KB 大 skb 交给网卡分段。小消息路径根本碰不到 TSO 阈值（`tcp_tso_autosize`，tcp_output.c:1976）。
2. **TSQ（TCP Small Queues）才是真正的"闸门"**：`tcp_small_queue_check()`（tcp_output.c:2568）用 `2*truesize` 或 `pacing_rate >> sk_pacing_shift` 限制 socket 在飞行中的字节数——**队列里积压超过 limit 时新数据被扣住**（`TSQ_THROTTLED`）。这是比 cwnd 更贴近发送机的流控。
3. **v6.6 的 pacing 是 EDT（Earliest Departure Time）模型**：TCP 在 `tcp_transmit_skb` 里把"最早出发时刻"写进 `skb->tstamp`（tcp_output.c:1407 注释原文 *"Leave earliest departure time in skb->tstamp"*），fq qdisc 按 tstamp 排队出队（sch_fq.c:470）——**不再有令牌桶，每个包自带时刻表**。
4. **RACK 已是默认丢包检测**（tcp_recovery.c，IETF draft-ietf-tcpm-rack-01）："收到更新的 ACK 时，发送时间早于它且超过 reo_wnd 没被确认的段，判丢"。比 3-dup-ACK 快一个 RTT 级别，且天然处理 tail loss（最后几个包丢了不会有 dup ACK）。
5. **reo_wnd 自适应**：DSACK 频发（网络在重排序，RACK 误判过）时窗口按 step 增长（tcp_recovery.c:5，`reo_wnd_steps`，上限逻辑在 tcp_input.c:2585 `TCP_RACK_RECOVERY_THRESH`）——用"误判历史"调节容差。
6. **TCP-AO 在 v6.6 还不存在**：`net/ipv4/tcp_ao.c` 是 6.7 才合入的；旧笔记的"TCP_AUTHOPT 5.15+"说法不准（5.15 合入的是 authopt 框架雏形，完整 RFC 5925 支持是 6.7+）。

## 一、TSO：下推分段，不是攒包

### 机制

```
tcp_sendmsg → 写入发送队列 → tcp_push → tcp_write_xmit()
                                        │ tcp_tso_segs() 算单次最大段数
                                        ▼
                        构造 ≤64KB 的大 skb（gso_size = mss）
                                        ▼
                        驱动 DMA 一次，网卡按 MSS 切段发出
```

`tcp_tso_autosize()`（tcp_output.c:1976）的"自动尺寸"逻辑：

```c
static u32 tcp_tso_autosize(const struct sock *sk, unsigned int mss_now,
			    u32 min_tso_segs)
{
	unsigned long bytes;
	u32 r;

	bytes = sk->sk_pacing_rate >> READ_ONCE(sk->sk_pacing_shift);  // 每毫秒字节数

	r = tcp_skb_pcount(skb)-1; /* 段数越多步进越大 */
	if (r < BITS_PER_TYPE(sk->sk_gso_max_size))
		bytes += sk->sk_gso_max_size >> r;   // 逼近 gso_max_size(默认64KB)
	bytes = min_t(unsigned long, bytes, sk->sk_gso_max_size);
	return bytes  → 换算成段数;
}
```

设计意图：**pacing rate 高的流允许更大的 TSO 段**（一次排队摊薄更多协议栈成本）；rate 低则小段——避免低速率流在网卡里积压"已分段未发出"的数据。

### 修正两个流行说法

- **"TSO 增加发送延迟，HFT 应关闭"**——错。TSO 是**机会主义**的：发送时刻由 tcp_push 决定（TCP_NODELAY 即刻），TSO 只影响"这一批发多少打包方式"。1KB 的订单消息永远不会被扣住等 64KB。关 TSO 真正影响的是**吞吐路径**（每 MSS 一次协议栈遍历 vs 每 64KB 一次）和分段的**时间分布**（网卡分段出的连续帧背靠背上线，交换机瞬间 microburst——这才是 fq pacing 要解决的问题）。
- **"ethtool -K eth0 tso off 降延迟"**——对单条小消息流测不出差异；对满速大流量反而升 CPU。HFT 机器关不关 TSO 的正确判据是：交易口几乎只有小消息（TSO 无关紧要）+ 独立行情/回放口（开着有益）。

## 二、TSQ：发送机的内存闸门

```c
// tcp_output.c:2568 tcp_small_queue_check()（节选）
limit = max_t(unsigned long,
	      2 * skb->truesize,
	      sk->sk_pacing_rate >> READ_ONCE(sk->sk_pacing_shift));
if (sk->sk_pacing_status == SK_PACING_NONE)
	limit = min_t(unsigned long, limit,
		      READ_ONCE(sock_net(sk)->ipv4.sysctl_tcp_limit_output_bytes)); // 默认 1MB
...
if (refcount_read(&sk->sk_wmem_alloc) > limit) {
	if (tcp_rtx_queue_empty_or_single_skb(sk))
		return false;                      // 队列空/只剩一段：总是发
	set_bit(TSQ_THROTTLED, &sk->sk_tsq_flags);   // 否则扣住，等 TX 完成回调唤醒
	...
	return true;
}
```

TSQ 的意义：限制**单个 socket 在驱动/qdisc 里滞留的内存**——没有它，10G 线速下一个 cwnd 窗口的数据瞬间灌进 qdisc，饿死其他流且 swap 抖动。注意 `tcp_rtx_queue_empty_or_single_skb` 的例外：**重传队列为空或只剩一个 skb 时永远放行**——保底延迟，不让 TSQ 卡死最后一条消息。

## 三、Pacing：EDT 模型与 fq qdisc

### 传统问题

cwnd 只回答"能发多少"，不回答"什么时候发"。窗口打开瞬间内核把 N 个包背靠背喷出（burst）→ 交换机入口队列瞬时打满 → **同机房其他流的尾延迟被你制造出来**。

### EDT 解法（v4.20+ 落地）

```
tcp_transmit_skb()  ──►  skb->tstamp = now + 该流速率推算的出发时刻
        │                        （tcp_output.c:1407）
        ▼
   qdisc (sch_fq)
        │ fq_enqueue: fq_skb_cb(skb)->time_to_send = skb->tstamp  (sch_fq.c:470)
        │ fq_dequeue: 只发 tstamp ≤ now 的包（红黑树按时间排序）
        ▼
      网卡（必要时配合 TSO 分段）
```

- **fq 自动激活 pacing**：sch_fq.c:325/351，第一个带 `sk_pacing_rate` 的 skb 到达时 `smp_store_release(&sk->sk_pacing_status, SK_PACING_NEEDED)`——socket 侧从此知道"我的包会被按时放行"。
- **没有 fq 时**：`SK_PACING_NEEDED` 状态下 TCP 用内部 hrtimer 模拟 pacing（tcp_output.c:1209 一带，`tcp_pace_kick`）——精度略差但语义一致。
- **horizon 防御**：sch_fq.c:439，tstamp 离现在太远（超过 horizon，默认 10s）的包被夹回来——防止某 socket 的时钟错乱把整个 qdisc 卡住。
- **速率来源**：`tcp_update_pacing_rate()`（CC 模块在 cwnd 更新时调用）：reno/cubic = cwnd/RTT 的 2 倍左右的启发式；BBR 直接给精确带宽估计——**BBR 的高精度 pacing 是它低队列占用的根基**。

### HFT 视角

交易流（KB 级、偶发）受 pacing 影响可忽略——pacing rate 远高于消息速率，EDT 时刻就是 now。行情转发流（持续满速）则完全活在 EDT 模型里：**fq qdisc 的红黑树插拔在发送热路径上**，交换侧 microburst 被削平的代价是每包一次 rbtree 操作。

## 四、RACK：基于时间的丢包检测

### 核心数据（`tp->rack`）

| 字段 | 含义 | 更新点 |
|---|---|---|
| `mstamp` | 最近一次（新）数据被确认的时间 | `tcp_rack_advance()` tcp_recovery.c:118 |
| `rtt_us` | 该次确认测得的 RTT | 同上 |
| `advanced` | 有新进展待重估 | 同上置 1 |
| `reo_wnd_steps` | 重排序容差步数（自适应） | DSACK 误判时增长（tcp_input.c:2585） |

### 判丢逻辑

```c
// tcp_recovery.c:58 tcp_rack_detect_loss()（节选）
reo_wnd = tcp_rack_reo_wnd(sk);                       // 容差窗口
list_for_each_entry_safe(skb, ...) {                   // 遍历重传队列
	if (!tcp_skb_sent_after(tp->rack.mstamp,
				tp->rack.end_seq, scb->end_seq))
		continue;                               // 只看发送时间晚于 rack.mstamp 的段？
	// ↑ 实际是反向：发送时间【早于】最新确认对应时间的段才有资格被判丢
	remaining = tcp_rack_skb_timeout(tp, skb, reo_wnd); // rtt + reo_wnd - 已等待
	if (remaining > 0)
		*reo_timeout = max(...)                  // 还有耐心，定个重估定时器
	else
		tcp_skb_mark_lost_unsacked(sk, skb);     // 判丢
}
```

**一句话**："我最新的数据都确认了（用了 rtt_us），你比它早发却还没到，而且我给了你 reo_wnd 的容差——你八成是丢了。"

对比传统机制：

| | 3-dup-ACK + FACK | RACK |
|---|---|---|
| 信号 | 收到第 3 个重复 ACK | 任何携带新数据的 ACK |
| tail loss | 检测不到（无 dup）→ 等 RTO | **天然检测**（时间超限即判丢） |
| 重排序容错 | scoreboard/SACK 记账 | reo_wnd 时间窗，自适应 |
| 误判代价 | 过早重传（spurious） | reo_wnd 增大慢恢复 |
| 状态 | v6.6 里仍并存（`tcp_dupack` 路径） | 默认主路径，`tcp_rack_mark_lost()`（:95）在每个 ACK 后调用 |

`tcp_rack_reo_wnd()`（:5）的容差：`min(min_rtt/4 * steps, ...) `——**以 RTT 的四分之一为基数按步进放大**；DSACK 说明"你判丢的那个其实到了"时 steps++（上限 `TCP_RACK_RECOVERY_THRESH`），此后逐渐衰减回 1。

## 五、其他现代特性速览（v6.6 事实核对）

| 特性 | 版本 | v6.6 状态 |
|---|---|---|
| TCP Fast Open | 3.6+ cookie, `TCP_FASTOPEN_CONNECT` 4.15+ | 可用；`sendmsg` 无 sockopt 变体（`tcp_fastopen` flag）在 connect 前置数据 |
| TCP repair | 3.5+ | 可用（`TCP_REPAIR` 一族 sockopt），容器热迁移用 |
| kTLS（TLS offload） | 4.13+ tx / 5.x rx | 可用；`TLS_TX`/`TLS_RX` setsockopt 后数据路径走加密 offload |
| **TCP-AO（RFC 5925）** | **6.7+** | **v6.6 无 `tcp_ao.c`**（jsdelivr 404 实测）；旧笔记"5.15+"是 authopt 框架雏形 |
| SACK/FACK/RACK | RACK 4.9+，默认 | `sysctl_tcp_recovery`（RACK 丢失定时器微调）可用 |

## HFT 关联

| 机制 | HFT 判定 |
|---|---|
| TSO | 交易口无所谓（碰不到阈值）；行情/回放口保持开启；**不存在"关 TSO 降延迟"的收益** |
| TSQ | 长肥管道满速回放时它是防 microburst 的闸门；默认参数即可 |
| EDT pacing | 交易流无感；行情转发流要意识到 fq 的 rbtree 在热路径——规模大时考虑 `fq` 换 `noqueue`+独立口 |
| RACK | 保持默认——对交易 TCP 连接，RACK 把丢包恢复从 RTO 级（百 ms）拉到亚 RTT 级，是**免费的尾部延迟改善** |
| TFO | 交易网关与前置间长连接 + 断线重连场景，TFO 省一个 RTT；首次连接仍需 cookie 协商 |
| kTLS | 加密回放流可用；交易流极少有人愿意把 TLS 放进关键路径 |

## 衔接

TCP 的发送/检测机制讲完。下一篇 [14-02 UDP GRO](02-udp-gro.md) 讲接收侧的批量合并——组播行情每秒几十万包的 PPS 压力，GRO 用"一次协议栈遍历处理 N 个包"来摊薄成本，以及为什么交易所行情常用的零 checksum 会让它整个失效。

## 代码自测

<details>
<summary>Q1：TCP_NODELAY 开着，为什么 TSO 仍然不会让小消息变慢？</summary>

发送时刻由 `tcp_push`→`tcp_write_xmit` 的调用决定，NODELAY 意味着数据写入即触发。TSO 的影响只在 `tcp_tso_segs()` 算出的"单次最大段数"——它决定这一批数据打成多大的 skb，但**不决定什么时候发**。队列里只有 1KB 时，发出去的就是 1KB 的普通 skb（连 gso_size 都不设）。TSO 的收益场景（大流量多段排队）与代价场景（网卡分段 microburst）都不在"单条小消息"上。
</details>

<details>
<summary>Q2：TSQ 的 limit 为什么用 truesize（含开销）而不是 len（纯数据）？</summary>

TSQ 管的是**内存占用**不是数据量：一个 skb 在驱动/qdisc 里占的内存是 truesize（数据 + skb 结构 + 预留），网卡环形描述符、qdisc 队列的内存压力都跟 truesize 走。用 len 会导致小包流（truesize/len 比值大）实际内存占用远超预期。`2 * truesize` 下限还保证了"至少能发一个包"——极限情况下不饿死。
</details>

<details>
<summary>Q3：EDT 模型里 TCP 和 fq 的分工边界在哪？为什么把时刻写进 skb 而不是 fq 自己算？</summary>

TCP 知道流语义（速率、RTT、cwnd 变化），由它算"这个包最早该几点走"（tcp_output.c:1407 写 skb->tstamp）；fq 只负责"到点放行"（sch_fq.c:470）——单调时钟比较 + 红黑树。这样 qdisc 无需理解每流的 CC 状态，CC 换算法零改动 qdisc；反过来 fq 给 TCP 的反馈只有 `sk_pacing_status`（"我在按时放行，你可以按 EDT 计划"）。解耦后 BBR 的精确 pacing 才能即插即用。
</details>

<details>
<summary>Q4：尾丢（tail loss，窗口最后两个包丢了）为什么 3-dup-ACK 救不了而 RACK 能？</summary>

dup ACK 由"乱序到达"触发：后续包到达才产生 dup。尾部丢了之后**没有后续包**，收不到任何 dup——传统路径只能等 RTO（百 ms 级）。RACK 是时间驱动：任意新 ACK 都推进 rack.mstamp，尾部段"发送时间早于 mstamp + 容差内未确认"即可判丢——不需要额外的包到达信号，重估定时器（reo_timeout）兜底触发恢复。
</details>

<details>
<summary>Q5：DSACK 不断增加 reo_wnd_steps，会不会让丢包检测退化到不可用？</summary>

不会无限涨：`reo_wnd_steps` 有上限（tcp_input.c:2585，`TCP_RACK_RECOVERY_THRESH`，达到即进入保守模式），且窗口基数是 min_rtt/4——steps 涨的是倍数，绝对值仍受 RTT 约束。设计上的权衡：误判重传（spurious retransmit）烧带宽且可能触发不必要的 CC 回退，比晚一点判丢更贵；网络真的长期深度重排序时，宁可慢一点。另外 steps 会随时间/无 DSACK 阶段回落，不是单调增长。
</details>
