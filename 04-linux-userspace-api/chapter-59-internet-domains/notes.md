# TLPI 第 59 章 — Sockets: Internet Domains

> 对应目录：`chapter-59-internet-domains/`  
> 书名原文：**Sockets: Internet Domains**  
> ⚠️ **新项目用 `getaddrinfo`/`getnameinfo`，禁用 `gethostbyname`/`inet_ntoa`。** TCP 写已断连接 → **SIGPIPE**（忽略或 `MSG_NOSIGNAL`）。勿裸发 C 结构体。UDP **不可靠**（≠ UNIX DGRAM）。后置按地图是 [Ch60 Server Design](../chapter-60-server-design/notes.md)（非「Socket Options」；选项等多在 Ch61 Advanced）。

**优先级**：🔴（INET TCP/UDP 实战）  
**前置**：[Ch58 TCP/IP 基础](../chapter-58-tcpip-fundamentals/notes.md)  
**后置**：[Ch60 Server Design](../chapter-60-server-design/notes.md)

---

## 章节目标

`sockaddr_in(6)`；pton/ntop；`getaddrinfo`；UDP/TCP 迭代 C/S；SIGPIPE；序列化；vs UNIX。

---

## 59.1–59.3 回顾 · 序列化

端点 = IP+端口；TCP 流可靠；UDP 报不可靠。  
字节序：`htons`/`htonl` 永远调用。  
跨机：**禁止直接 send 原生结构体** → 文本 / JSON/PB/TLV / 统一网络序打包。

---

## 59.4–59.6 地址与转换

`sockaddr_in` / `sockaddr_in6` / `sockaddr_storage`。  
`inet_pton`/`inet_ntop`；缓冲 `INET_ADDRSTRLEN` / `INET6_ADDRSTRLEN`。  
淘汰：`gethostbyname`、`getservbyname`、`inet_ntoa`（非线程安全 / 仅 v4）。

---

## 59.7 `getaddrinfo` / `getnameinfo`（核心）

```c
getaddrinfo(host, service, &hints, &res);  /* 链表；用完 freeaddrinfo */
getnameinfo(addr, len, host, ..., serv, ..., flags);
```

| hints | |
|-------|--|
| `AI_PASSIVE` | 服务端；host=NULL → ANY |
| `AI_NUMERICHOST` / `NUMERICSERV` | 禁 DNS / 禁服务名查表 |
| `AF_UNSPEC` | 双栈友好 |

`ai_addr` 可直接 bind/connect。

---

## 59.8–59.9 UDP / TCP 模型

UDP：`sendto`/`recvfrom`；可选 `connect` 固定对端后用 send/recv。  
TCP 迭代服务：`socket→bind→listen→循环 accept→处理→close(conn)`；listener 常留。  
DNS / `/etc/services`：由 getaddrinfo 经 nsswitch 使用。

Demo：[`code/`](./code/)

---

## 59.13 SIGPIPE

对端已关仍 `write`/`send` → SIGPIPE → 默认同亡。  
处理：`signal(SIGPIPE, SIG_IGN)` 或 `send(..., MSG_NOSIGNAL)`。

---

## vs UNIX（汇总）

| | AF_UNIX | AF_INET(6) |
|--|---------|------------|
| 范围 | 本机 | 跨机 |
| DGRAM | 可靠 | UDP 可丢 |
| fd/凭证传递 | 可 | 否 |
| 字节序 | 不必 | 必须 |

---

## 陷阱

1. 忘 htons / memset  
2. 裸发结构体  
3. TCP 当消息边界  
4. SIGPIPE  
5. 旧解析 API  
6. 忘 `freeaddrinfo`  
7. UDP recv 缓冲短 → 余部丢弃  
8. connect 用 INADDR_ANY  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | getaddrinfo + freeaddrinfo |
| 2 | AI_PASSIVE 服务端 |
| 3 | TCP 迭代：accept 新 fd |
| 4 | SIGPIPE：IGN 或 MSG_NOSIGNAL |
| 5 | UDP 边界有、可靠无 |
| 6 | 跨机须序列化 |

---

## 参考

- Kerrisk · TLPI Ch59  
- `man 3 getaddrinfo` · `getnameinfo` · `man 7 tcp` · `udp`
