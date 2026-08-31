# Chapter 14: TCP/UDP 内部机制

> 来源：LWN（TCP internals + UDP GRO）
> 对标：Rosen Ch4/6（TCP/UDP 3.x 实现）
> 本版基于 **v6.6 源码核验**重写（tcp_output.c / tcp_recovery.c / sch_fq.c / udp_offload.c / udp.c 锚点到行号）

## 核心结论（全章浓缩）

1. **TSO 不等凑包**：发送时刻由 `tcp_push`（NODELAY 即刻）决定，TSO 只影响"同一批排队数据打包成多大 skb"（≤64KB，`tcp_tso_autosize` 按 pacing rate 自适应）——"关 TSO 降延迟"对单条小消息流测不出差异。
2. **TSQ 才是发送闸门**：`tcp_small_queue_check()`（tcp_output.c:2568）限制单 socket 在飞内存（`2*truesize` 或 pacing 派生），超限扣住新数据（TSQ_THROTTLED），但重传队列空/单 skb 时永远放行——保底延迟。
3. **v6.6 pacing 是 EDT 模型**：TCP 把出发时刻写进 `skb->tstamp`（tcp_output.c:1407），fq qdisc 按时刻放行（sch_fq.c:470）——CC 算法与 qdisc 解耦，BBR 的精确 pacing 即插即用；horizon（10s）防时钟错乱卡死队列。
4. **RACK 默认且覆盖 tail loss**：任意新 ACK 推进 rack.mstamp，"早于它发出 + 超过 reo_wnd 未确认"即判丢（tcp_recovery.c:58）——尾丢从 RTO 级降到亚 RTT 级，是交易 TCP 连接免费的尾延迟改善。
5. **UDP GRO 的 HFT killer 是零 checksum**：`udp_gro_receive_segment()` 要求 csum 非零（udp_offload.c:464），交易所行情组播普遍关 checksum → GRO 直接不生效；先 `tcpdump -vv` 验证再谈收益。
6. **UDP GRO 交付协议是 cmsg 不是 MSG_EOR**：socket 开 UDP_GRO（=104，udp.c:2710）后合并包整条交付（单次 recvmsg ≤64KB），gso_size 经 `SOL_UDP/UDP_GRO` cmsg 上报（include/linux/udp.h:122）；不开则 `udp_rcv_segment()` 拆回逐条投递，向下兼容。
7. **TCP-AO 在 v6.6 尚不存在**（`tcp_ao.c` 为 6.7+），旧笔记"5.15+"说法已纠正。

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [tcp-internals](notes/01-tcp-internals.md) | TSO/TSQ/EDT pacing/RACK 源码级；TFO、repair、kTLS 事实核对 |
| 2 | [udp-gro](notes/02-udp-gro.md) | UDP GRO 准入（csum killer）、cmsg 交付协议、UDP_SEGMENT 发送 |
| 3 | [multicast-rx-path](notes/03-multicast-rx-path.md) | UDP 组播收包：L2 有损映射、组播复制开销、**旁路后 IGMP 失效**、gap 检测 |

## HFT 关联

- **TCP 拥塞控制**：HFT 交易连接用 `tcp_congestion_control=none`（或 BBR）避免 AIMD 降窗
- **Nagle 禁用**：`TCP_NODELAY=1` 是 HFT 必备，禁用 Nagle 算法避免小包等待
- **UDP GRO**：行情数据用 UDP 多播，GRO 聚合多个 UDP 包减少处理开销
- **TCP buffer 调优**：`tcp_rmem` / `tcp_wmem` 调大窗口，避免窗口缩放不足导致限速
- **行情走 UDP 组播不是 TCP**（§3）：组播路径多出 `ip_route_input_mc()` 与 per-socket skb 复制；
  **旁路后 IGMP 不再自动发出**，必须先与交换机确认静态组播，否则收不到任何流量
- **组播是 at-most-once**：无重传，必须做序列号 gap 检测 + 独立的 TCP 补单通道

## 交叉引用

- `11-tcpip-protocols/`：TCP/UDP 协议基础
- `12.5-modern-networking/chapter-02-napi-rx-path/`：GRO 与 busy polling 在收包路径（§3 的性能前提）
- `13-dpdk/01-Intro-Book/notes/chapter-05-组播行情接入.md`：§3 的内核路径 → DPDK 旁路对照
- `12-kernel-networking/note-组播IGMP.md`：组播协议基础（IGMP snooping / 组播路由）
