# 11. 小结（10.6 Summary）

> 底本：《BPF之巅》第 10 章 网络，10.6 节（印刷 p530）

## 本章要点回顾

1. **网络栈分层观测**（图 10-1 / 表 10-1）：syscall→套接字→TCP/UDP→IP→qdisc→驱动，每层有对应跟踪点或 kprobe；31 个 BPF 工具按层就位。
2. **低开销策略**：跟踪**事件**而非每包——重传、丢包、状态迁移、缓冲区超限，天然低频高价值（tcpretrans 开销 ≈1% 采样）。
3. **上下文归属三陷阱**（10.1.4）：软中断上下文无 PID（sock 指针缓存法）、快慢路径、无效字段——本章工具反复演示规避手法。
4. **延迟七指标 + 对应工具**：DNS（gethostlatency）/Ping（superping）/建连（soconnlat）/TTFB（solstbyte）/RTT（tcpwin）/连接时长（tcplife）/丢包定位（tcpdrop+skbdrop）。
5. **kprobe → 跟踪点迁移趋势**：tcp:tcp_retransmit_skb（4.15）、tcp:tcpprobe（4.16）、sock:inet_sock_set_state 等逐步替代 kprobe 版本，更稳定高效。
6. **与传统工具配合**：nstat/sar 定面 → ss 定连接 → BPF 定事件与根因；tcpdump 仅留给低频取证。

## 网络三板斧（生产默认动作）

```bash
tcpretrans      # 重传/丢包事件（跨网问题信号）
tcpconnect -t   # 主动连接异常与延迟
tcplife         # 连接画像（时长/吞吐/归属）
```

## 与其他章的衔接

- **下钻链**：tcptop 发现异常流量 → softirq/hardirq（ch6）看中断热点 → profile 看 CPU → skblife/skbdrop 钻内核。
- **向磁盘延伸**：收到的数据落盘 → biolatency/biotop（ch9）。
- **向前延伸**：容器/安全视角 → ch11（安全）用同套 sock 跟踪点做策略审计。
- XDP/tc BPF 编程本身 → 仓库 `note-XDP与tc-BPF` 与 13-DPDK 对照。

## HFT 一句话

交易链路每一跳都能被 BPF 量化：**建连→TTFB→RTT→重传→丢包栈**，五件套（soconnlat/solstbyte/tcpwin/tcpretrans/skbdrop）构成延迟预算表的现场测量手段。

<details>
<summary>自测题</summary>

1. "跟踪事件而非每包"为什么是网络 BPF 的核心原则？
   <details><summary>答案</summary>包率以 10 万 pps 计且与负载线性相关——逐包跟踪的开销随流量增长（iptraf-ng/tcpdump 的 90% CPU 就是下场）；而高价值事件（重传/丢包/状态迁移/缓冲超限）天然稀少（重传 ≈1%），跟踪开销与**事件率**而非**包率**成正比。聚合放内核态、只把低频事件送出来，是低开销的结构性来源。</details>

2. 内核 4.15/4.16 新增的 tcp 跟踪点解决了什么问题？
   <details><summary>答案</summary>tcp:tcp_retransmit_skb（4.15）与 tcp:tcpprobe（4.16）把原来必须 kprobe 内核函数（tcp_retransmit_skb/tcp_rcv_established）的观测迁移到稳定跟踪点——kprobe 受函数改名、内联、参数漂移影响（ch05/ch06 反复出现的脆弱性），跟踪点是稳定 ABI，跨内核可移植。本章"tp 版工具"（tcpconnect-tp、tcpaccept-tp 等）都是这波迁移的产物。</details>

3. 说出网络三板斧及各自回答的问题。
   <details><summary>答案</summary>tcpretrans——哪里在丢包/重传（跨网质量信号）；tcpconnect -t——谁在建连、建得快不快（建连异常与延迟）；tcplife——每条连接的时长/吞吐/归属画像（连接行为审计）。三件都是事件型低开销，可常驻。</details>
</details>
