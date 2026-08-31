# 5. BPF 工具：TCP 流量与重传（10.3.14–10.3.19）

> 底本：《BPF之巅》第 10 章 网络，10.3 节（印刷 p485–500）

覆盖 6 个工具：tcptop、tcpsnoop、tcpretrans、tcpsynbl、tcpwin、tcpnagle。

## 5.1 tcptop —— 按连接聚合吞吐

```
PID    COMM         LADDR  RADDR  LPORT RPORT TX_KB RX_KB
```

- 周期输出 top 会话表，回答"哪条连接最费带宽"。

## 5.2 tcpsnoop —— 逐包事件流

- 每个收/发包一行（时间戳+PID+地址端口+字节数+方向），tcpdump 的事件版但有 PID。

## 5.3 tcpretrans —— 重传（生产明星工具）

```
TIME   PID    LADDR  RADDR  LPORT RPORT STATE
```

- **开销可忽略**：重传率天然低（案例：10 万包/秒仅 1000 重传/秒 ≈ 1% 采样）。
- 跟踪点 `tcp:tcp_retransmit_skb`（Linux 4.15+）或 kprobe `tcp_retransmit_skb`。
- `-c` 统计模式；含 **send loss probe**（TCPLostRetransmit/PTO 探测）事件。
- **dport 是大端序**，需翻转后显示。
- 案例：
  - **Netflix**：云上流量超限排查（重传=运营商丢包信号）。
  - **Shopify**：tcpdump 丢包率过高不可用 → 改 tcpretrans + tcpdrop 定位**防火墙静默丢包**。

## 5.4 tcpsynbl —— SYN 积压队列长度

```
backlog_queue_len: 按最大长度直方图（<=0, 1, 2-3, ... 4096+）
```

- 读 `sk_max_ack_backlog`（当前）与 listen backlog（上限）。
- 超限时内核打印 "possible SYN flooding, sending cookies" / dropping SYN —— 应用还没感知就丢客户。
- 调优：`listen(2)` 第二参数 + `net.core.somaxconn`。

## 5.5 tcpwin —— 拥塞窗口时间序列

- CSV 输出：`rcv,sock,timeus,snd_cwnd,snd_ssthresh,sk_sndbuf,sk_wmem_queued`。
- kprobe `tcp_rcv_established` 采样每个 ACK 后的窗口；R 绘图呈**锯齿状**（每次丢包/拥塞事件 cwnd 跌落）。
- 跟踪点版：`tcp:tcpprobe`（Linux 4.16+）。

## 5.6 tcpnagle —— Nagle 算法状态统计

```
@flags: 0-5 → ON / OFF / CORK / PUSH 映射
```

- kprobe `tcp_write_xmit` 的 arg2（nonagle 参数）：OFF 226697 次但真正引入延迟仅 5 次——**Nagle 常被冤枉**，先测再关。

## HFT 关联

- 交易链路**必须确认 TCP_NODELAY**：tcpnagle 先验证 Nagle 是否真在捣乱（避免盲改）。
- tcpretrans + tcpdrop = 跨网络丢包定位双件套（配套 `ss -i` 看 rto/backoff）；对柜台/行情网关每分钟巡检。
- tcpsynbl 用于开盘前压测：确认 somaxconn/backlog 与预期峰值匹配。

<details>
<summary>自测题</summary>

1. 为什么 tcpretrans 开销可忽略？给出数量级估算。
   <details><summary>答案</summary>开销与**重传事件率**（而非包率）成正比，重传率天然低：书例 10 万 pps 中约 1000 重传/s——相当于只有 1% 的包触发跟踪，每次 ~1µs 的 BPF 处理摊到全部流量上 ≈ 0.001% 量级。这是"跟踪事件而非每包"原则的教科书案例。</details>

2. tcpsynbl 读取哪个字段？两个相关内核参数是什么？
   <details><summary>答案</summary>读 `sk_max_ack_backlog`（当前 SYN 积压长度，与 listen backlog 上限对比）。参数：`listen(2)` 第二参数（应用侧 backlog）+ `net.core.somaxconn`（系统级上限——应用设再大也会被它截断）。</details>

3. tcpwin 输出中 snd_cwnd 锯齿说明什么？
   <details><summary>答案</summary>典型 AIMD 拥塞控制节律：丢包/拥塞事件后 cwnd 减半（锯齿跌落），拥塞避免阶段线性爬回（缓坡）——每个锯齿周期对应一次丢包事件。锯齿密度=拥塞频率，跌幅=拥塞严重度，是"这链路丢包长什么样"的时间序列答案。</details>

4. tcpnagle 的输出如何解读才算"Nagle 确实造成延迟"？
   <details><summary>答案</summary>Nagle OFF 计数大≠有延迟（书例 OFF 226697 次但真正引入延迟仅 5 次——大部分 OFF 只是"这个包本来就有 PUSH/满段，Nagle 不适用"）。判据是"因 Nagle 而**等待凑包**的实际延迟事件数"那一栏显著非零。先测再关 TCP_NODELAY，避免盲改。</details>
</details>
