# TLPI 第 61 章 — Sockets: Advanced Topics

> 对应目录：`chapter-61-sockets-advanced/`  
> 书名原文：**Sockets: Advanced Topics**（Socket API 收尾）  
> ⚠️ **`SO_REUSEADDR` 须在 bind 前设。** REUSEADDR≠REUSEPORT。TCP **短读/短写**须循环。传 fd / 凭证用 **`sendmsg`/`recvmsg` 辅助数据**。UDP `connect` 无握手、仍不可靠。OOB 仅 1 字节，少用。

**优先级**：🔴（选项、msghdr、UDP connect、短读写）  
**前置**：[Ch60 Server Design](../chapter-60-server-design/notes.md)  
**后置**：[Ch62 Terminals](../chapter-62-terminals/notes.md)

---

## 章节目标

`setsockopt`/`getsockopt`；SOL_SOCKET / TCP 选项；OOB；`sendmsg`/`recvmsg`；短读写；`getsockname`/`getpeername`；UDP connect；`IPV6_V6ONLY`。

---

## 61.1 API

```c
getsockopt(fd, level, optname, optval, &optlen);
setsockopt(fd, level, optname, optval, optlen);
```

`level`：`SOL_SOCKET` · `IPPROTO_TCP` · `IPPROTO_IP`/`IPV6`。  
部分选项须 **bind/connect 前**设置。

---

## 61.2 SOL_SOCKET（必考）

| 选项 | 要点 |
|------|------|
| `SO_REUSEADDR` | TIME_WAIT 端口可再 bind；**listen 前开** |
| `SO_REUSEPORT` | 多进程同 IP:port，内核负载分担 |
| `SO_RCVBUF`/`SNDBUF` | 建议值，内核对齐；影响窗口 |
| `SO_LINGER` | close 行为；`linger=0`→RST，慎用 |
| `SO_KEEPALIVE` | 死连接探测；业务心跳更可控 |
| `SO_ERROR` | 取异步错（如非阻塞 connect）后清零 |
| `SO_RCVTIMEO`/`SNDTIMEO` | 超时；主要作用于 recv/send/…（对 `read`/`write` 因实现而异） |
| `SO_BROADCAST` | UDP 广播 |

---

## 61.3 TCP 选项

| `TCP_NODELAY` | 关 Nagle → 小包立刻发（交互/低延迟） |
| `TCP_KEEPIDLE` 等 | 配 SO_KEEPALIVE |

---

## 61.4 OOB

`MSG_OOB`；TCP **仅 1 字节**紧急数据；`SIGURG` 或 `recv(..., MSG_OOB)`。工程少用，自定协议更好。

---

## 61.5 `sendmsg` / `recvmsg`

最强收发：`msg_iov` 分散聚集；`msg_control` **辅助数据** → UNIX **传 fd / 凭证**。普通 send 做不到。

---

## 61.6 短读写

| TCP | 可部分读写；须循环发满；对端关 → recv=0；RST → `ECONNRESET` |
| UDP | **整报**；缓冲过小 → **余部丢弃**，不分段 |

---

## 61.7–61.9 地址 · UDP connect · IPv6

`getsockname` / `getpeername`：本端 / 对端（UDP 未 connect 无 peer）。  

UDP `connect`：**无网络握手**；记下默认对端 → 可用 send/recv；非对端报文丢弃；可收异步 ICMP；**仍可丢包**。`AF_UNSPEC` 可“断开”。  

`IPV6_V6ONLY`：开=纯 v6；关（Linux 默认可双栈映射）。

Demo：[`code/`](./code/)

---

## 61.10 `ioctl`

如 `FIONREAD`：待读字节；TCP 不含 OOB；UDP=下一条报大小。

---

## 陷阱

1. REUSEADDR 设晚于 bind  
2. REUSEADDR vs REUSEPORT  
3. TCP 不循环写满  
4. TCP 当包边界  
5. send 传 fd  
6. linger=0 粗暴 RST  
7. UDP connect≠可靠  
8. OOB 当大数据通道  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | REUSEADDR 在 bind 前；≠ REUSEPORT |
| 2 | TCP_NODELAY 关 Nagle |
| 3 | 短读写循环；UDP 整报 |
| 4 | fd 传递 → sendmsg 控制信息 |
| 5 | UDP connect 无握手仍可丢 |
| 6 | linger=0 → RST |

---

## 参考

- Kerrisk · TLPI Ch61  
- `man 7 socket` · `tcp` · `man 2 sendmsg` · `getsockname`
