# 01 — 发包路径（sendmsg → tc egress → ndo_start_xmit → 完成中断）

> **Bootlin 课程模块：** TX Path
> **对应 Rosen:** Ch11
> **内核版本:** 以 v6.6 `net/core/dev.c` 为准，行号已核对源码

## 文档概述

本篇是 chapter-03 的**主线**：一次 `sendmsg()` 从用户态落到线缆上，中间经过哪些函数、哪些队列、哪些延迟源。

本篇与兄弟篇的分工：

| 篇 | 讲什么 | 不讲什么 |
|----|--------|----------|
| **01（本篇）** | 发包的**函数调用链**与延迟构成 | 驱动内部细节 |
| [02-txrx](02-txrx.md) | 驱动与内核的**契约**（`ndo_start_xmit` 返回值、DMA 映射、Tx ring 清理） | 协议栈上层 |
| [03-sock-sk-buff](03-sock-sk-buff.md) | `sk_buff` 的**分配/克隆/释放**生命周期 | 发包时序 |
| [04-sk-buff-xdp-buff](04-sk-buff-xdp-buff.md) | `xdp_buff` ↔ `sk_buff` 的**三种转换** | hook 顺序 |
| [chapter-01](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) | 收发 12 个 hook 点的**全局顺序** | 单条路径内部 |
| [chapter-09](../../chapter-09-tc-bpf/) | qdisc / tc-BPF 的**详细机制** | 发包路径全貌 |

**一句话结论**：发包延迟里，唯一能被你自己搞成毫秒级的，是 **qdisc 排队**。协议栈和驱动那部分通常总共 1–3 μs，你动不了多少；qdisc 一个 `fq_codel` 就能给你加 5 ms。

原笔记给了一张延迟分解表，数字大致没错，但有两个问题：一是**没说怎么测**（数字不可复现就没有意义），二是漏了**完成中断**这一段——而它恰恰是 `MSG_ZEROCOPY` 的真实代价所在。

---

## 一、完整链路（函数级）

```
用户态  sendmsg() / sendto() / write()
  │  系统调用 + 两次特权级切换（含 copy_from_user）
  ▼
socket 层
  udp_sendmsg()                           net/ipv4/udp.c
  tcp_sendmsg()                           net/ipv4/tcp.c
  ├─ 普通路径：copy_from_user() 拷进 skb 线性区
  ├─ MSG_ZEROCOPY：不拷数据，把用户 page 通过
  │   skb_frag 挂到 skb_shinfo(skb)->frags[]，标记 SKBTX_ZEROCOPY_FRAG
  └─ io_uring SEND_ZC（6.0+）：同上 + 不阻塞等完成
  ▼
传输层
  udp_send_skb() / tcp_write_xmit()
  ├─ 填 L4 头、校验和（CHECKSUM_PARTIAL → 交给网卡算）
  └─ TCP 额外：Nagle 判断、拥塞窗口、TSQ 限速、pacing
  ▼
IP 层
  ip_send_skb() → __ip_local_out() → ip_output()
  ├─ Netfilter OUTPUT → POST_ROUTING
  ├─ 路由查找（fib_lookup）
  └─ 分片 / GSO 分段
  ▼
邻居子系统（仅单播）
  neigh_output()                          net/core/neighbour.c
  ├─ ARP 命中 → neigh_hh_output()（走 hh_cache 缓存的 L2 头，快）
  └─ ARP 未命中 → neigh_resolve_output()，包被**暂存**，首包多一次 ARP 往返
  ▼
dev_queue_xmit() → __dev_queue_xmit()     net/core/dev.c:4277
  ├─ ① __skb_tstamp_tx(SCM_TSTAMP_SCHED)   dev.c:4289（ETF/SO_TXTIME 打时间戳）
  ├─ ② nf_hook_egress()                    Netfilter egress（6.x，在 tc 之前）
  ├─ ③ sch_handle_egress()                 dev.c:4311 ← tc egress / clsact-BPF
  ├─ ④ netdev_core_pick_tx()               选 txq（多队列网卡）
  └─ ⑤ q->enqueue → __dev_xmit_skb()      进 qdisc
      └─ qdisc 空且无人占用 → sch_direct_xmit() 直接发
         qdisc 有人占 → 入队，唤醒 net_tx_action softirq / 等 TX 完成中断
  ▼
qdisc 出队
  __qdisc_run() → qdisc_restart() → dequeue_skb()
      → sch_direct_xmit() → dev_hard_start_xmit()   dev.c:3579
  ▼
dev_hard_start_xmit()                      dev.c:3579
  while (skb) {
      xmit_one(skb, dev, txq, next != NULL);   ← 注意第 4 个参数
      ...
  }
  ▼
xmit_one()                                 dev.c:3562
  ├─ if (dev_nit_active(dev)) dev_queue_xmit_nit(skb, dev);  ← tcpdump 抓发包点
  └─ netdev_start_xmit(skb, dev, txq, more)  → ndo_start_xmit()
  ▼
驱动 ndo_start_xmit()
  ├─ dma_map_single() / dma_map_page()  流式 DMA 映射
  ├─ 填 Tx ring 描述符
  └─ if (!more) 写门铃寄存器（doorbell）告知网卡
  ▼
网卡 DMA 取数据 → 发送 → 线缆
  ▼
TX 完成中断（或 NAPI poll）
  驱动清理函数 → dma_unmap → napi_consume_skb() → 回收 skb / 释放 zero-copy page
  ▼
MSG_ZEROCOPY 场景：完成通知进 socket error queue（SO_EE_ORIGIN_ZEROCOPY）
```

### 三个容易被忽略的点

**① 不是"驱动 dequeue"，是"qdisc 决定什么时候发"**

原笔记第 5 步写成「驱动 dequeue → DMA 映射」，读起来像驱动自己来拉包。实际是：

- 进程上下文 `sendmsg()` 时，如果 qdisc 空且没别的 CPU 在跑它，`sch_direct_xmit()` 立刻把包发出去（**fast path，无排队延迟**）；
- 否则入队，由 `net_tx_action` softirq 或 TX 完成中断触发 `__qdisc_run()` 出队。

这个区别是实操的关键：**发包延迟是双峰的**。qdisc 空时 ~2 μs；一旦开始排队，延迟 = 队列深度 × 单包发送时间。只看 mean 完全看不到第二个峰。

**② `xmit_more` / doorbell 批量**

`xmit_one(skb, dev, txq, next != NULL)` 的第四个参数就是 `xmit_more`：告诉驱动"后面还有，先别写门铃"。驱动据此推迟 doorbell 寄存器写（一次 MMIO 写 ~100 ns 以上）。

对 HFT 的含义：**批量发多个包时，前面的包是被"攒"住的**，只有最后一个包触发门铃。所以不要拿"第 1 个包的延迟"当单包延迟——真要测单包延迟，必须一次只发一个、且 qdisc 为空。

**③ tcpdump 抓发包，抓的是 tc egress 之后**

`dev_queue_xmit_nit()` 在 `xmit_one()` 里，即在 `sch_handle_egress()` 之后、紧邻 `ndo_start_xmit()`。所以 **tc egress 丢掉的包，tcpdump 抓不到**。诊断 tc egress 丢包要用 `tc -s filter show dev eth0 egress` 看计数。

---

## 二、延迟构成与"谁该背锅"

| 阶段 | 典型延迟 | 你能改吗 | 怎么改 |
|------|---------|---------|--------|
| 系统调用 + `copy_from_user` | 500–2000 ns | 部分 | `MSG_ZEROCOPY` / `io_uring`；小包反而更慢 |
| 传输层（UDP） | 300–800 ns | 少 | 关校验和卸载前先想清楚 |
| IP 层 + 路由 | 200–800 ns | 少 | 路由表小、无 Netfilter 规则 |
| **qdisc 排队** | **0 ns ～ 数 ms** | **★ 主战场** | 换 `pfifo_fast` / 用 ETF 定时发送 |
| 驱动 xmit + DMA 映射 | 100–500 ns | 少 | 关 TSO/GSO 减少分段 |
| 网卡发送 + 线缆 | 100–500 ns + 传播延迟 | 否 | 换网卡 / 缩短光纤 |
| **完成中断** | **0 ～ 数 ms** | **★** | `ethtool -C eth0 tx-usecs 0` |

**怎么测**：上表的每一行都不是拍脑袋来的，靠 `SO_TIMESTAMPING` 的分段时间戳。用 `TX_SCHED`（进 qdisc 前，dev.c:4289 那个 `SCM_TSTAMP_SCHED`）和 `TX_HARDWARE`（网卡真实上线缆时刻，需网卡支持）相减，得到的就是**纯队列 + 驱动延迟**。这个差值直接告诉你该不该换 qdisc。

最后一行是被忽略最多的：**发包还有个"尾巴"**。TX 完成中断的合并（`tx-usecs` / `tx-frames`）默认是开的，意味着你的 skb（以及 `MSG_ZEROCOPY` 的用户 buffer）要等合并窗口过去才被释放。

**`MSG_ZEROCOPY` 对 HFT 的真实代价就在这里**——不是系统调用省了拷贝，而是**你的发送缓冲区在完成通知到达前不能复用**。完成通知被合并延迟，等于把缓冲区池变小、把内存压力变成延迟。小包（< 10 KB）本来就省不了多少拷贝，反而多一次 error queue 通知，所以业界共识是：**小包别用 `MSG_ZEROCOPY`**。

---

## 三、qdisc：主战场

### 默认 qdisc 是 `fq_codel`，而 CoDel 的 target 是 5 ms

```
$ sysctl net.core.default_qdisc
net.core.default_qdisc = fq_codel
```

CoDel（Controlled Delay）的设计目标是"把队列延迟控制在 **5 ms**"（`target`），判定周期 `interval` 100 ms。它**主动**允许排队到 5 ms 才开始丢包。对 HFT 这是灾难性的：你什么都没做错，qdisc 自己就在制造毫秒级尾延迟。

| qdisc | 排队行为 | 适合 HFT 吗 |
|-------|---------|------------|
| `fq_codel` | 每流公平 + 主动延迟到 5 ms | ❌ 默认项，必须换 |
| `fq` | 每流公平 + pacing，**不做 CoDel 延迟** | ⚠️ 多流时可用，注意 pacing |
| `pfifo_fast` | 三个优先级 FIFO，无主动延迟 | ✅ 传统 HFT 选择 |
| `noqueue` / `mq` + bypass | 不排队，直接 xmit 或 `NETDEV_TX_BUSY` | ✅ 极致低延迟（丢包风险自负） |
| `etf` | **按纳秒时间戳定时发送** | ✅ 需要确定性发送时刻时 |

```bash
# 换成无主动延迟的 qdisc
tc qdisc replace dev eth0 root pfifo_fast
# 或直接绕过（配合 SO_PRIORITY / 单流）
tc qdisc replace dev eth0 root mq

# 确认当前用的是什么
tc qdisc show dev eth0
```

### 队列长度：`txqueuelen` 和 BQL

```
$ ip link show eth0 | grep qlen
    link/ether ... qlen 1000
```

`txqueuelen` 默认 1000 包。这是**包数**，不是字节数。10 Gbps 下 1000 个 64 B 小包 ≈ 67 μs 排队；1000 个 1500 B 包 ≈ 1.2 ms。HFT 场景常把它压到几十。

BQL（Byte Queue Limits）是另一层：它按**字节**动态限制硬件 Tx ring 的驻留量，防止 bufferbloat 发生在驱动队列里（那里 qdisc 管不到）。

```bash
# 看 BQL 当前限制与在途字节
cat /sys/class/net/eth0/queues/tx-0/byte_queue_limits/limit
cat /sys/class/net/eth0/queues/tx-0/byte_queue_limits/inflight
```

`inflight` 长期贴着 `limit`，说明硬件队列里堆着东西——尾延迟就在那儿。

### ETF + `SO_TXTIME`：纳秒级定时发送

如果想**精确控制包什么时候上线缆**（而不是"尽快发"），内核侧的方案是 ETF qdisc：

```bash
# offload 需要网卡支持（Intel i210/i225 等 TSN 网卡）
tc qdisc replace dev eth0 root etf clockid CLOCK_TAI delta 300000 offload
```

配合 socket 的 `SO_TXTIME` + `SCM_TXTIME` cmsg 指定每个包的发送时刻。

**但要注意**：没有 `offload` 时，ETF 用 hrtimer 在软件里等到点再发，抖动在几十 μs 量级——比不排队还糟。**ETF 的价值完全依赖网卡硬件卸载**。这也是为什么真正做确定性发送的 HFT 系统通常直接用 Solarflare / Exablaze 的 `ef_vi` 定时发送，而不是内核 ETF。

时间戳打在 `__dev_queue_xmit()` 开头：skb 带 `SKBTX_SCHED_TSTAMP` 时记录 `SCM_TSTAMP_SCHED`（dev.c:4289）。因为它是**进 qdisc 之前**打的，所以能反映"实际发送时刻 vs 期望时刻"的偏差。

---

## 四、TCP 特有的坑（下单链路）

行情是 UDP 组播，但下单是 TCP。以下每一项都能给下单加几十到几百毫秒：

| 机制 | 默认值 | 症状 | 处置 |
|------|--------|------|------|
| **Nagle** | 开 | 小包被攒着等 ACK | `TCP_NODELAY` |
| **Delayed ACK** | 开（~40 ms） | 对端 ACK 慢 → RTT 测量虚高 | 对端设 `TCP_QUICKACK`，或避免"写-写-读"模式 |
| **Nagle + Delayed ACK 死锁** | 都开 | 请求-响应模式下直接卡 ~200 ms | 必须关 Nagle |
| **TSQ**（TCP Small Queues） | 限制每流在途字节 | 单流突发被截断成多批 | `sysctl net.ipv4.tcp_limit_output_bytes`（先查当前值） |
| **pacing** | `fq` 下默认开 | 按 `sk_pacing_rate` 摊平发送 | `SO_MAX_PACING_RATE` 设为极大 |
| **`SO_SNDBUF`** | 自动调优 | 大 buffer → 更多在途数据 → 尾延迟 | 小包低延迟：调小并显式 setsockopt |

最典型的现场：**下单用 TCP，忘了 `TCP_NODELAY`，延迟看起来"随机"地多出 40–200 ms**。这不是网络问题，是协议栈在按设计工作。

---

## 五、观测：延迟花在哪一步

```bash
# 1) qdisc 是否排队（最重要）
tc -s qdisc show dev eth0
#   看 backlog / requeues / dropped；backlog 长期 > 0 → 队列在堆

# 2) 硬件 Tx ring 里堆了多少（BQL）
cat /sys/class/net/eth0/queues/tx-0/byte_queue_limits/inflight

# 3) 驱动层统计：丢包、错误、Tx 超时
ethtool -S eth0 | grep -i "tx_"

# 4) 中断合并（决定完成通知的延迟）
ethtool -c eth0
ethtool -C eth0 tx-usecs 0 tx-frames 1     # 低延迟：关掉 TX 合并

# 5) TCP 层面：在途、重传、pacing
ss -tinp 'sport = :12345'                  # 看 rtt / cwnd / pacing_rate / retrans

# 6) 端到端：内核 + 硬件时间戳
ethtool -T eth0                            # 先看网卡支持哪些
#   setsockopt SO_TIMESTAMPING = SOF_TIMESTAMPING_TX_HARDWARE
```

---

## HFT 要点

- **qdisc 是发包延迟的主战场**：`fq_codel` 默认给你 5 ms，换成 `pfifo_fast` / `noqueue` 是收益最大的一步。
- **发包延迟是双峰的**：qdisc 空时 ~2 μs，一开始排队就跳到队列深度 × 单包时间。**只测 mean 会完全看不到第二个峰**。
- **`MSG_ZEROCOPY` 对小包是负优化**：省下的拷贝 < 完成通知 + buffer 锁定成本。只对大包（10 KB 以上）划算。
- **`xmit_more` 会"攒"住前几个包**：单包延迟测量必须一次只发一个，否则测到的是被攒住的那部分。
- **下单链路先查 `TCP_NODELAY`**：Nagle + Delayed ACK 能凭空造出 200 ms，比任何网络问题都常见。
- **TCP 发包没有 busy poll**：`SO_BUSY_POLL` 只管收包。发包的完成清理依赖中断 / softirq，所以 `ethtool -C` 的 TX 合并同样重要。
- **收发不对称**：收包可以忙轮询绕开中断延迟；发包只能"尽快走完 + 尽快被通知完成"。
- **ARP 未命中的首包**多一次往返。长期运行的行情进程无所谓，但冷启动的第一笔下单会踩到——预热一条连接。

## 与 Rosen 3.x 的差异

Rosen 第 11 章描述的是 2.6.x 时代的发包路径，有几处已经不成立了：

| Rosen 3.x | 现在（5.x/6.x） |
|-----------|----------------|
| qdisc 默认 `pfifo_fast` | 默认 **`fq_codel`**（主动引入 5 ms 队列延迟） |
| 发包路径无 BPF 介入 | **tc egress BPF**（clsact）在 qdisc 之前 |
| 无 `MSG_ZEROCOPY` | 4.14+ `MSG_ZEROCOPY`，6.0+ `io_uring SEND_ZC` |
| 无硬件定时发送 | 5.x+ **ETF qdisc + `SO_TXTIME`** |
| 单队列为主 | 多队列 + `netdev_core_pick_tx()`，队列选择影响锁竞争 |
| 无 egress Netfilter hook | 6.x 加 **`nf_hook_egress()`**，且在 tc egress **之前** |
| GSO/TSO 是优化 | 对 HFT 常常是**负优化**（大包分段 → 单包延迟上升） |
| 驱动主动 dequeue | 出队由 qdisc 层驱动（`__qdisc_run` / `sch_direct_xmit`），驱动只提供 `ndo_start_xmit` 回调 |

---

## 代码自测

<details>
<summary>Q1：你把 <code>fq_codel</code> 换成了 <code>pfifo_fast</code>，p99 发包延迟从 4.8 ms 降到 120 μs，但 mean 几乎没变。为什么？</summary>

<b>答：</b>因为 mean 主要落在"qdisc 为空"的 fast path 上（~2 μs），本来就没受影响。CoDel 的伤害集中在<b>有排队时的尾部</b>——它主动把队列攒到 5 ms 才丢包。

这说明：低延迟优化几乎全部体现在 p99 / p99.9 上，<b>mean 是几乎无信息的指标</b>。如果一个"优化"只改了 mean 没改 p99，那它优化的是吞吐，不是延迟。
</details>

<details>
<summary>Q2：你用 <code>MSG_ZEROCOPY</code> 发 200 字节的下单报文，为什么延迟反而变差了？</summary>

<b>答：</b>三个原因叠加：

1. 200 字节的 `copy_from_user()` 只有几十 ns，本来就不是瓶颈——你省了个不存在的东西。
2. zero-copy 需要把用户页 pin 住并挂到 `skb_shinfo->frags[]`，涉及 `get_user_pages()`，这比拷贝<b>贵得多</b>。
3. 完成通知要等到 TX 完成中断（还被 `tx-usecs` 合并），在此之前那块 buffer 不能用。小包高频下等于把发送缓冲池缩小，退化成隐式的流控阻塞。

业界经验阈值：大约 <b>10 KB</b> 以下用普通 send，以上才考虑 `MSG_ZEROCOPY`。
</details>

<details>
<summary>Q3：你测出"第一个包延迟 8 μs，后续每个 1.2 μs"，怀疑是 <code>xmit_more</code> 导致的。怎么验证？</summary>

<b>答：</b>做对照实验——<b>一次只发一个包，两次 send 之间等一个 RTT</b>（确保 qdisc 排空），再测。

- 若单发测得 ~1.5 μs，说明 8 μs 确实来自批量：第 1 个包被 `xmit_more=1` 标记、门铃没写，一直等到最后一个包才触发 doorbell。
- 若单发仍是 8 μs，问题在别处：ARP 未命中首包、qdisc 非空、TSQ 限速、NUMA 跨节点。

顺带验证队列是否真的空：`tc -s qdisc show dev eth0` 看 `backlog`。非 0 说明你测的是排队延迟，不是单包延迟。
</details>

---

→ 前一篇：[chapter-01 网络栈架构与 hook 点顺序](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md)
→ 后一篇：[02-txrx：驱动与内核的契约](02-txrx.md)
→ 相关：[chapter-09 tc/BPF](../../chapter-09-tc-bpf/) · [chapter-13 零拷贝](../../chapter-13-zerocopy-highperf/)
