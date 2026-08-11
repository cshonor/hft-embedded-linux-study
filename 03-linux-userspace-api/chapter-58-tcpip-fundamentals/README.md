# TLPI 第 58 章 — Sockets: Fundamentals of TCP/IP Networks

**优先级**：🟡（Ch59 实战地基）  
**前置**：[Ch57 UNIX 域](../chapter-57-sockets-unix-domain/notes.md)  
**后置**：[Ch59 Internet Domains](../chapter-59-internet-domains/notes.md)

---

## 小节目录

- [58.1 INET vs UNIX](./notes/58.1-inet.md)
- [58.2 –58.3 IPv4 · 端口](./notes/58.2-ipv4.md)
- [58.4 `sockaddr_in`](./notes/58.4-sockaddrin.md)
- [58.5 网络字节序（高频）](./notes/58.5-byte-order-network.md)
- [58.6 地址转换](./notes/58.6-conversion.md)
- [58.7 TCP vs UDP](./notes/58.7-tcp-udp.md)
- [58.8 –58.9 `sockaddr_storage` · `INADDR_ANY`](./notes/58.8-sockaddrstorage-inaddrany.md)

---

## 章节目标


IPv4/端口；`sockaddr_in`；字节序；pton/ntop；TCP vs UDP；`sockaddr_storage`；`INADDR_ANY`。

---


---

## 初始化模板


```c
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
/* or: addr.sin_addr.s_addr = htonl(INADDR_ANY); */
```

---


---

## 陷阱


1. 忘 htons/htonl  
2. 用 inet_ntoa  
3. 当 UDP 可靠  
4. TCP 一次 send≠一次 recv  
5. sin_zero 未清零  
6. 客户端 connect `INADDR_ANY`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 端点 = IP + 端口 |
| 2 | 网络大端；htons/htonl |
| 3 | pton/ntop；勿 ntoa |
| 4 | INADDR_ANY 仅服务端 bind |
| 5 | TCP 流 / UDP 报；UDP 可丢 |
| 6 | UNIX DGRAM ≠ UDP |

---


---

## 参考


- Kerrisk · TLPI Ch58  
- `man 3 htons` · `inet_pton` · `man 7 ip` · `tcp` · `udp`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
