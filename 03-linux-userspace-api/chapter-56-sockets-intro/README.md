# TLPI 第 56 章 — Sockets: Introduction

**优先级**：🔴（Socket API 总入口）  
**前置**：[Ch55 文件锁](../chapter-55-file-locking/notes.md) · 本地 IPC  
**后置**：[Ch57 UNIX 域](../chapter-57-sockets-unix-domain/notes.md) → [Ch58 TCP/IP](../chapter-58-tcpip-fundamentals/notes.md) → [Ch59 Internet](../chapter-59-internet-domains/notes.md)

---

## 小节目录

- [56.1 概念](./notes/56.1-concepts.md)
- [56.2 –56.4 创建与地址](./notes/56.2-creation.md)
- [56.5 面向连接 API](./notes/56.5-api.md)
- [56.6 –56.8 I/O 与关闭](./notes/56.6-close.md)

---

## 章节目标


`socket`/`bind`/`listen`/`accept`/`connect`；地址结构；读写与关闭；STREAM vs DGRAM；UNIX vs INET。

---


---

## UNIX vs Internet（导论）


| | AF_UNIX | AF_INET(6) |
|--|---------|------------|
| 范围 | 本机 | 跨机 |
| 地址 | 路径 / 抽象名 | IP:port |
| DGRAM | 可靠 | UDP 可丢 |
| 权限 | 文件权限 | 应用层 |
| 特色 | 可传 fd/凭证 | 协议栈 |

本机优先 UNIX；要远程用 Internet。

---


---

## 陷阱


1. STREAM 无消息边界  
2. 勿关 listener；关的是 conn fd  
3. UNIX DGRAM ≠ UDP 可靠性  
4. fork 复制 fd → close≠断连  
5. backlog≠最大客户端数  
6. addrlen 类型用 socklen_t  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | socket→bind→listen→accept / connect |
| 2 | STREAM 字节流；DGRAM 有边界 |
| 3 | accept 新 fd；listener 常留 |
| 4 | shutdown 半关闭；close 看引用 |
| 5 | UNIX DGRAM 可靠；UDP 不 |
| 6 | 本机 UNIX；跨机 INET |

---


---

## 参考


- Kerrisk · TLPI Ch56  
- `man 2 socket` · `bind` · `listen` · `accept` · `connect` · `shutdown`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
