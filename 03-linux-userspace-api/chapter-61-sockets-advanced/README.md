# TLPI 第 61 章 — Sockets: Advanced Topics

**优先级**：🔴（选项、msghdr、UDP connect、短读写）  
**前置**：[Ch60 Server Design](../chapter-60-server-design/notes.md)  
**后置**：[Ch62 Terminals](../chapter-62-terminals/notes.md)

---

## 小节目录

- [61.1 API](./notes/61.1-api.md)
- [61.2 SOL_SOCKET（必考）](./notes/61.2-solsocket.md)
- [61.3 TCP 选项](./notes/61.3-tcp.md)
- [61.4 OOB](./notes/61.4-oob.md)
- [61.5 `sendmsg` / `recvmsg`](./notes/61.5-sendmsg-recvmsg.md)
- [61.6 短读写](./notes/61.6-section-61-6.md)
- [61.7 –61.9 地址 · UDP connect · IPv6](./notes/61.7-udp-connect-ipv6.md)
- [61.10 `ioctl`](./notes/61.10-ioctl.md)

---

## 章节目标


`setsockopt`/`getsockopt`；SOL_SOCKET / TCP 选项；OOB；`sendmsg`/`recvmsg`；短读写；`getsockname`/`getpeername`；UDP connect；`IPV6_V6ONLY`。

---


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


---

## 参考


- Kerrisk · TLPI Ch61  
- `man 7 socket` · `tcp` · `man 2 sendmsg` · `getsockname`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
