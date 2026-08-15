# 6. UDP、DNS 与其他

### `udpconnect`

追踪 **UDP** “连接”/首次 sendto 目标。

```bash
sudo udpconnect-bpfcc
```

**HFT：** 组播/UDP 行情 — 与 [15-DPDK 组播笔记](../14-dpdk/01-Intro-Book/notes/chapter-05-组播行情接入.md) 对照。

### `gethostlatency`

追踪 **`getaddrinfo` 等 DNS 解析** 延迟。

```bash
sudo gethostlatency-bpfcc
```

**一针见血：** 慢在 **DNS** 还是 **网络 RTT** — 共置机偶发 `connect` 卡顿常见根因。

### `superping`

内核路径 **ICMP echo** 延迟 — 减少用户态 `ping` 的调度 jitter。

### `ipecn`

追踪 IPv4 **ECN (Explicit Congestion Notification)** 入站事件 — 拥塞信号是否到达。


### 常见陷阱

1. **忽视 DNS 解析延迟对 HFT 的影响** — HFT 启动时 DNS 解析如果走网络可能引入毫秒级延迟；应在本地缓存 /etc/hosts 或使用 IP 直连
2. **混淆 UDP 和 TCP 的观测方式** — UDP 无连接状态（无 connect/accept），用不同的 tracepoint 追踪；UDP 丢包不会重传，需靠应用层检测
3. **忽视 UDP socket buffer 大小** — UDP 不可靠，buffer 溢出直接丢包无重传；`ss -u -m` 查看 buffer 使用，`sysctl net.core.rmem_max` 调大

<details>
<summary>📝 自测题（点击展开）</summary>

1. **UDP 和 TCP 在 BPF 追踪上有什么区别？**

   <details>
   <summary>参考答案</summary>

   TCP 有连接状态机（connect/accept/retransmit 等 tracepoint 丰富）。UDP 无连接：(1) 无 connect/accept——用 sendto/recvfrom 追踪；(2) 无重传——丢包不可恢复，需追踪 udp_send_skb 和 udp_rcv 的丢包点；(3) 无 RTT——无法用 tcprtt 类工具。UDP 追踪重点：send/recv 频率、buffer 溢出丢包、DNS 解析延迟。

   </details>

2. **DNS 解析对 HFT 有什么影响？如何优化？**

   <details>
   <summary>参考答案</summary>

   DNS 解析走网络可能引入 1-50ms 延迟（取决于 DNS 服务器和网络）。HFT 启动时如果每次连接都做 DNS 解析，会引入不可控延迟。优化：(1) /etc/hosts 预填关键地址（零 DNS 延迟）；(2) 应用层缓存 DNS 结果（首次解析后复用）；(3) 使用本地 DNS 缓存（systemd-resolved/nscd）；(4) 直接用 IP 连接（生产环境推荐）。

   </details>

3. **如何用 BPF 追踪 UDP 丢包？**

   <details>
   <summary>参考答案</summary>

   UDP 丢包发生在：(1) socket buffer 满——`tracepoint:udp:udp_fail_queue_rcv_skb { @[reason] = count() }`；(2) 校验和错误——`kprobe:__udp4_lib_rcv /ret == 0/` 检查返回值；(3) 无 socket 匹配——`tracepoint:udp:udp_rcv { @drop++ }`。`netstat -su` 给聚合计数，BPF 能定位丢包时刻和原因。

   </details>

</details>

---
