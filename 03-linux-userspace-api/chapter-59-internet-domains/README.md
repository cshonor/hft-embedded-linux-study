# TLPI 第 59 章 — Sockets: Internet Domains

**优先级**：🔴（INET TCP/UDP 实战）  
**前置**：[Ch58 TCP/IP 基础](../chapter-58-tcpip-fundamentals/notes.md)  
**后置**：[Ch60 Server Design](../chapter-60-server-design/notes.md)

---

## 小节目录

- [59.1 –59.3 回顾 · 序列化](./notes/59.1-serialization.md)
- [59.4 –59.6 地址与转换](./notes/59.4-conversion.md)
- [59.7 `getaddrinfo` / `getnameinfo`（核心）](./notes/59.7-getaddrinfo-getnameinfo.md)
- [59.8 –59.9 UDP / TCP 模型](./notes/59.8-udp-tcp.md)
- [59.13 SIGPIPE](./notes/59.13-sigpipe.md)

---

## 章节目标


`sockaddr_in(6)`；pton/ntop；`getaddrinfo`；UDP/TCP 迭代 C/S；SIGPIPE；序列化；vs UNIX。

---


---

## vs UNIX（汇总）


| | AF_UNIX | AF_INET(6) |
|--|---------|------------|
| 范围 | 本机 | 跨机 |
| DGRAM | 可靠 | UDP 可丢 |
| fd/凭证传递 | 可 | 否 |
| 字节序 | 不必 | 必须 |

---


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


---

## 参考


- Kerrisk · TLPI Ch59  
- `man 3 getaddrinfo` · `getnameinfo` · `man 7 tcp` · `udp`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
