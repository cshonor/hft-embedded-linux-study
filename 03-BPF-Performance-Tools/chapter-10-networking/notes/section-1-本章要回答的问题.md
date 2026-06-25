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

---
