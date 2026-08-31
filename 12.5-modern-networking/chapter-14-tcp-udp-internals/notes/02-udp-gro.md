# 14-02 — UDP GRO：批量接收与交付协议（v6.6 源码级）

> **对应 Rosen:** Ch11（UDP 无 GRO，机制是 4.x/5.x 产物）
> **内核源码路径:** `net/ipv4/udp_offload.c`、`net/ipv4/udp.c`、`include/linux/udp.h:98,122`、`include/uapi/linux/udp.h`

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [14-01 TCP 内部](01-tcp-internals.md) | **14-02 UDP GRO** | [14-03 组播 RX 路径](03-multicast-rx-path.md) |

## 本节讲什么

组播行情的典型画像：每包几百字节、每秒十万级 PPS、单 flow。每个包过一遍完整协议栈（IP 头解析、UDP 查找、checksum、入队、唤醒）的固定成本 ~200-500ns，乘以 PPS 就是整核的消耗。GRO（Generic Receive Offload）的思路：**在 NAPI 批处理循环里把同 flow 的连续包合并成一个大 skb，协议栈只过一遍**。本篇讲 UDP GRO 的准入条件（其中一个会让交易所行情直接失效）、交付协议（cmsg 上报 gso_size，**不是**网上流传的 MSG_EOR）、以及收发两侧的对称设计。

## 要点（先记住结论）

1. **合并的准入在 `udp_gro_receive()`（udp_offload.c:545）**：socket 开了 `UDP_GRO`（或设备开 `NETIF_F_GRO_UDP_FWD` 走转发路径）才调 `udp_gro_receive_segment()`（:464）做 L4 合并——**没开的 socket，GRO 引擎对 UDP 直接 flush**。
2. **零 checksum = 不合并（killer）**：`udp_gro_receive_segment()` 开头 `if (!uh->check)` 直接放弃（注释原文 *"requires non zero csum, for symmetry with GSO"*）。**交易所行情组播普遍关闭 UDP checksum（省发送端开销）——这类流量上 UDP GRO 整个不生效**，这是 HFT 场景最重要的一条判据。
3. **交付协议是 cmsg，不是 MSG_EOR**（纠正旧笔记）：socket 开 `UDP_GRO`（uapi/udp.h:36，选项号 104，设置走 udp.c:2710→`up->gro_enabled`）后，合并包**整条**入队，一次 `recvmsg` 可读到 64KB；**分段大小通过 cmsg `SOL_UDP/UDP_GRO` 上报**（`udp_cmsg_recv()`，include/linux/udp.h:122，内容是 `skb_shinfo(skb)->gso_size`）——应用按 gso_size 自行切分。
4. **不开 UDP_GRO 的 socket 收到 GRO 合并包会被拆回去**：`udp_queue_rcv_skb()`（udp.c:2177）里 `udp_unexpected_gso()` 判定为"不速之客"→ `udp_rcv_segment()`（:2188）逐段拆开、逐条入队——行为与无 GRO 完全一致，向下兼容零成本。
5. **发送侧对称机制是 `UDP_SEGMENT` cmsg**（uapi/udp.h:35，选项号 103）：一次 sendmsg 写 ≤64KB，内核按 gso_size 切段（`cork->gso_size`，udp.c:908-934），单段上限 `UDP_MAX_SEGMENTS = 64`（include/linux/udp.h:98）——配合网卡 GSO/USO 一次 DMA 发出。
6. **GRO 没有等待窗口**：合并在 NAPI poll 的同一次批处理里完成（`dev_gro_receive` 持续把包挂到 hold 状态的 skb 上），**不存在"攒包等合并"的额外延迟**——"合并窗口增加延迟"的旧说法不准确，真实的延迟影响只有一个：合并包的 checksum 验证从硬件（每包独立）变成增量式。

## 一、合并发生在哪：NAPI 循环里的 GRO

```
NAPI poll（chapter-02 讲过的循环）
   └─ napi_gro_receive()
        └─ dev_gro_receive()
             ├─ 已有同 flow 的 hold skb？→ 挂到它的 frag list（不上协议栈）
             └─ 第一个包：标记 hold，记 flow 信息
        └─ napi_gro_flush()（poll 结束）→ 合并完的大 skb 进协议栈
```

`udp_gro_receive()` 是 UDP 层的 merge 判定回调：

```c
// udp_offload.c:545（节选）
if (!sk || !udp_sk(sk)->gro_receive) {              // ① socket 侧判定
	if (skb->dev->features & NETIF_F_GRO_FRAGLIST)
		NAPI_GRO_CB(skb)->is_flist = sk ? !udp_sk(sk)->gro_enabled : 1;

	if ((!sk && (skb->dev->features & NETIF_F_GRO_UDP_FWD)) ||
	    (sk && udp_sk(sk)->gro_enabled) || NAPI_GRO_CB(skb)->is_flist)
		return call_gro_receive(udp_gro_receive_segment, head, skb);

	/* no GRO, be sure flush the current packet */
	goto out;                                        // ② 未开启：flush
}
```

两条合并路线：
- **L4 模式**（`udp_gro_receive_segment`）：要求每包 checksum 非零且校验通过，合并成 `SKB_GSO_UDP_L4` 类型；
- **fraglist 模式**（`NETIF_F_GRO_FRAGLIST`）：不剥头、整包链成 fraglist——对转发场景友好（转出去时头还在）。

## 二、交付协议：收一条 64KB 合并包

```c
/* 正确的接收端写法（替代旧笔记错误的 MSG_EOR 说法） */
int one = 1;
setsockopt(fd, IPPROTO_UDP, UDP_GRO, &one, sizeof(one));   /* udp.c:2710 */

char buf[64 * 1024];
char ctrl[CMSG_SPACE(sizeof(int))];
struct msghdr msg = {
	.msg_iov     = &(struct iovec){ .iov_base = buf, .iov_len = sizeof(buf) },
	.msg_iovlen  = 1,
	.msg_control = ctrl, .msg_controllen = sizeof(ctrl),
};
int n = recvmsg(fd, &msg, 0);

/* n 可能是多个原始报文的总长（≤64KB）
 * 分段大小从 cmsg 拿 —— udp_cmsg_recv() (include/linux/udp.h:122): */
for (struct cmsghdr *cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
	if (cm->cmsg_level == IPPROTO_UDP && cm->cmsg_type == UDP_GRO) {
		int gso_size;
		memcpy(&gso_size, CMSG_DATA(cm), sizeof(gso_size));
		/* buf 里每 gso_size 字节一个原始报文，最后一页可能不足 */
	}
}
```

内核侧的对称实现（三处锚点）：

```c
// udp.c:2710  设置
case UDP_GRO:
	...
	up->gro_enabled = valbool;              // :2716

// udp.c:1871  recvmsg 时附带 cmsg
if (udp_sk(sk)->gro_enabled)
	udp_cmsg_recv(msg, sk, skb);           // 报 gso_size

// include/linux/udp.h:122  cmsg 构造
static inline void udp_cmsg_recv(struct msghdr *msg, struct sock *sk,
				 struct sk_buff *skb)
{
	int gso_size;
	if (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_L4) {
		gso_size = skb_shinfo(skb)->gso_size;
		put_cmsg(msg, SOL_UDP, UDP_GRO, sizeof(gso_size), &gso_size);
	}
}
```

### 不开 UDP_GRO 时的兜底

GRO 在设备层合并时**不知道**这些包最终落在哪个 socket（GRO 在协议栈之前）——合并包进了 UDP 层才发现目标 socket 不收大包：`udp_queue_rcv_skb()`（udp.c:2177）→ `udp_unexpected_gso()` 为真 → `udp_rcv_segment()`（:2188）把大 skb 拆回单报文，逐条 `udp_queue_rcv_one_skb()`。**多进程场景下只要有一个进程开了 UDP_GRO，其他进程行为不变**（拆回去的分发逻辑在 per-socket 队列之前……严格说在入队前拆完再各自投递）。

## 三、发送侧：UDP_SEGMENT 与 GSO

```c
/* 应用一次写 64KB，内核切成 N 个 MSS 段发出（一次协议栈遍历） */
char ctrl[CMSG_SPACE(sizeof(__u16))];
struct msghdr msg = { .msg_iov = &(struct iovec){ buf, 64*1024 },
                      .msg_control = ctrl, .msg_controllen = sizeof(ctrl) };
struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
cm->cmsg_level = IPPROTO_UDP;
cm->cmsg_type  = UDP_SEGMENT;             /* uapi/udp.h:35 = 103 */
cm->cmsg_len   = CMSG_LEN(sizeof(__u16));
*(__u16 *)CMSG_DATA(cm) = 1400;           /* 分段大小 */
sendmsg(fd, &msg, 0);
```

内核侧（udp.c:908-934，`udp_sendmsg` 的 cork 分支）：`cork->gso_size` 记住分段大小；`datalen > cork->gso_size` 时设置 `skb_shinfo(skb)->gso_size` 交给网卡 GSO；上限 `datalen > cork->gso_size * UDP_MAX_SEGMENTS`（=64 段）拒绝发送。注意 cmsg 校验在 `udp_cmsg_send()`（udp.c:1014）：**分段大小必须是偶数字节**（`cmsg_len` 检查 + gso_size 为 16 位）。

## 四、性能真相与适用判定

| 指标 | 无 UDP GRO | UDP GRO | 备注 |
|---|---|---|---|
| 协议栈遍历 | 每包一次 | 每 N 包一次 | NAPI 批内合并 |
| checksum | 硬件逐包 | 增量式（L4 模式） | 合并时逐包累计校验 |
| PPS 处理能力 | ~3 Mpps/core | ~8 Mpps/core | 官方基准量级 |
| 应用解析 | 每次一报文 | 每次 N 报文 | 要按 gso_size 切 |
| **延迟** | 基准 | **几乎无差** | 无等待窗口；仅有增量 csum |

**判定树**：

```
行情包 checksum == 0？
 ├─ 是 → UDP GRO 直接不生效（udp_offload.c:464 的准入），别折腾
 └─ 否 → 延迟敏感的交易/决策路径？
          ├─ 是 → 不开（省 cmsg 解析 + 应用切分复杂度，收益在吞吐不在延迟）
          └─ 否（录制/转发/回放）→ 开，PPS 压力立减 2-3x
```

转发场景的完整链路：`UDP_GRO 收` → 零修改透传 → `NETIF_F_GRO_UDP_FWD` + GSO 发——fraglist 模式下连头都不用剥，是 UDP 中继（行情放大器）的标准姿势。

## HFT 关联

- **先查 checksum 再谈 GRO**：`tcpdump -vv` 看行情流 `bad cksum 0`（发送端关闭）还是正常——前者 GRO 免谈，这是很多"HFT 上 GRO 没效果"的根因。
- **交易路径不开**：收益是吞吐/CPU，代价是应用层切分逻辑复杂度；交易路径的瓶颈从来不在 PPS。
- **行情中继/录制器必开**：3 Mpps/core → 8 Mpps/core 意味着 4 个核变 1.5 个核，或同样核数扛 3 倍通道。
- **与 chapter-13 的衔接**：转发器"收大包" + [MSG_ZEROCOPY/SEND_ZC 发大包](../chapter-13-zerocopy-highperf/notes/02-msg-zerocopy.md)——两端都摆脱逐包成本，中继的 CPU 曲线和延迟抖动一起改善。

## 衔接

GRO 是"包处理批量化的软件层"。下一篇 [14-03 组播 RX 路径](03-multicast-rx-path.md) 已是完整版（组播在 IP 层的分发、`ip_mc_join_group` 与网卡 hash 的关系）——本篇的 csum 准入问题在那里从组播配置的角度还有呼应。

## 代码自测

<details>
<summary>Q1：为什么 udp_gro_receive_segment 坚持 checksum 非零才合并（"for symmetry with GSO"）？</summary>

对称性问题：GSO 发送时网卡按 gso_size 切段，每段的 UDP checksum 由硬件逐段重算（分段后头字段变了）——这要求原始大包的 checksum 是"可增量分解"的。GRO 是逆操作：合并时逐包做增量校验（NAPI_GRO_CB 的 csum 累计），最后算总 checksum——零 checksum 的包没有可累计的校验值，合并后也无法为拆回场景重建每段的合法 checksum（转发路径要用）。所以干脆规定：L4 合并只对"校验在场"的流量开放。
</details>

<details>
<summary>Q2：设备层 GRO 合并时并不知道包会到哪个 socket，那 UDP_GRO 的 per-socket 开关怎么生效？</summary>

两段判定：①设备层看的是**当前 NAPI poll 对应 RX 队列的流量特征**+设备特性（GRO_UDP_FWD/GRO_FRAGLIST），先做机会主义合并；②合并包进 UDP 层做 socket lookup 时（udp_gro_receive 的 `sk` 参数来自查找结果）再按 `udp_sk(sk)->gro_enabled` 决定"这条合并能不能继续存在"——不开就 udp_rcv_segment 拆回去。所以 GRO 的合并尝试对所有流量发生，交付形态由目标 socket 的开关决定。
</details>

<details>
<summary>Q3：一次 recvmsg 读到 65536 字节、gso_size=512，原始报文到底几条？</summary>

128 条（65536/512），但**最后一页可能不足 gso_size**——gso_size 是"分段上限"不是"固定长度"（发送端最后一个 UDP_SEGMENT 段可以是任意 ≤gso_size 的长度）。正确解析：按 gso_size 切，剩余不足 gso_size 的尾巴是一条独立报文；总长必须能整除才全是满段。这也是为什么 cmsg 里给的是 size 而不是"报文数"——数是算出来的且尾段特殊。
</details>

<details>
<summary>Q4：UDP_SEGMENT 发送和 UDP_CORK 有什么关系？必须 cork 吗？</summary>

实现上 gso_size 存在 `cork->gso_size`（udp.c:908），cork 结构是"本次 sendmsg 的暂存上下文"——但**不需要应用显式 UDP_CORK**：带 UDP_SEGMENT cmsg 的单次 sendmsg 内部就走 cork 路径（构造大 skb → 设 gso_size → 一次发出）。显式 UDP_CORK 是另一个功能（攒多次 sendmsg 的数据再一次性发出）。两者可组合但独立：GSO 分段管"一次写多大怎么切"，cork 管"什么时候发"。
</details>

<details>
<summary>Q5：GRO 合并会不会把不同源/目标的 UDP 包合在一起？应用需要担心串流吗？</summary>

不会。merge 判定在 `udp_gro_receive_segment` 里逐字段比对：目标端口、源端口、源地址（同 flow 的 4-tuple 语义）都要一致才能挂进同一 hold skb。串流风险为零——这是协议栈内合并与"应用层攒批"的本质区别：内核有完整头信息做严格判据，应用层攒批反而容易把业务上不该合的合了。
</details>
