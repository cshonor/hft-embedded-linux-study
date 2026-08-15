# 1. 本章要回答的问题

| tcpdump / ss 的盲区 | BPF 补什么 |
|---------------------|------------|
| 看到包，不知 **哪个进程** 发的 | `tcpconnect`、`soconnect` + **PID/comm** |
| 只有线路统计 | **内核状态**：重传、SYN 队列、socket 缓冲 |
| 抓包 **高开销** | `tcplife`、`tcpretrans` **内核聚合** |
| 连接慢不知卡在哪 | `soconnlat`、`so1stbyte` + 栈 |

```
应用 syscall (socket/connect/send/recv)
        ↓
套接字层（sockstat / soconnect / socketio / sormem）
        ↓
TCP/UDP（tcpconnect / tcplife / tcpretrans / tcpsynbl）
        ↓
IP / DNS（gethostlatency / ipecn / superping）
        ↓
qdisc / skb / 驱动（qdisc-* / skbdrop / nettxlat / netsize）
        ↓
NIC  ·  旁路：DPDK / XDP → note-XDP
```


### 常见陷阱

1. **只看带宽忽视延迟** — HFT 网络问题主要是延迟和抖动而非带宽；带宽利用率低不代表网络没有问题
2. **混淆吞吐量和延迟的观测工具** — 吞吐量用计数器（ifconfig/ip -s link），延迟用追踪工具（tcpretrans/tcpconnlat）；选错工具类型会漏掉关键信息
3. **忽视网络栈各层的观测分工** — 网络问题可能在套接字层、TCP 协议层、IP 层、网卡驱动层；不同层用不同工具，跨层分析才能定位根因

<details>
<summary>📝 自测题（点击展开）</summary>

1. **Ch10 网络章节要回答的核心问题是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 网络延迟在哪里产生？（哪个层、哪个函数）；(2) 吞吐量瓶颈在哪？（带宽、丢包、重传）；(3) 连接建立是否正常？（TCP 握手延迟、失败率）。HFT 最关心延迟——从应用 send/recv 到网卡发包的每一跳都可能引入抖动。

   </details>

2. **网络分析的层次划分是什么？每层用什么工具？**

   <details>
   <summary>参考答案</summary>

   (1) 套接字层：sockstat、BCC socketsnoop——看 connect/accept/send/recv；(2) TCP 协议层：tcpretrans、tcpconnlat、tcptop——看重传、连接延迟、吞吐；(3) IP/qdisc 层：qdisc stats、BCC tcplife——看排队和丢弃；(4) 网卡驱动层：ethtool、BCC netqos——看硬件统计和中断。

   </details>

3. **HFT 网络排障与普通网络排障有什么区别？**

   <details>
   <summary>参考答案</summary>

   普通排障关注：带宽利用率、丢包率、连接成功率（毫秒级）。HFT 排障关注：微秒级延迟尖刺、抖动来源、尾部分布。区别：(1) 普通工具（ping/netstat）粒度太粗，HFT 需要 BPF 追踪每个包的内核路径时间；(2) HFT 更关心尾部延迟（P99.9）而非平均值；(3) HFT 关注内核网络栈的调度和中断影响。

   </details>

</details>

---
