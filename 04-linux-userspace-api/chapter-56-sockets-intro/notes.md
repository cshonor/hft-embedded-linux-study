# TLPI 第 56 章 — Sockets: Introduction

> 对应目录：`chapter-56-sockets-intro/`  
> 书名原文：**Sockets: Introduction**  
> ⚠️ **Socket = 通信端点 + fd。** STREAM=可靠字节流无边界；DGRAM=报文边界。Internet DGRAM(UDP) 可丢；**UNIX DGRAM 本地可靠**（满则发送阻塞）。`accept` 出 **conn fd**，listener 常驻。半关闭用 **`shutdown`**（无视 fd 引用计数）。

**优先级**：🔴（Socket API 总入口）  
**前置**：[Ch55 文件锁](../chapter-55-file-locking/notes.md) · 本地 IPC  
**后置**：[Ch57 UNIX 域](../chapter-57-sockets-unix-domain/notes.md) → [Ch58 TCP/IP](../chapter-58-tcpip-fundamentals/notes.md) → [Ch59 Internet](../chapter-59-internet-domains/notes.md)

---

## 章节目标

`socket`/`bind`/`listen`/`accept`/`connect`；地址结构；读写与关闭；STREAM vs DGRAM；UNIX vs INET。

---

## 56.1 概念

| 域 | 用途 |
|----|------|
| `AF_UNIX` | 本机 IPC |
| `AF_INET` / `AF_INET6` | 网络 |

| 类型 | |
|------|--|
| `SOCK_STREAM` | 连接、可靠、双向字节流、有序无重复 |
| `SOCK_DGRAM` | 无连接、保留边界；INET 不可靠；**UNIX 可靠** |

C/S：服务器 `socket→bind→listen→accept`；客户端 `socket→connect`（通常不 bind）。

---

## 56.2–56.4 创建与地址

```c
int socket(int domain, int type, int protocol);  /* protocol 常 0 */
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

统一入口 `struct sockaddr`；实际用 `sockaddr_un` / `_in` / `_in6`。  
`addrlen` 用 **`socklen_t`**。代码用 `AF_*`（与 `PF_*` 同值）。

---

## 56.5 面向连接 API

| 调用 | |
|------|--|
| `listen(fd, backlog)` | 仅 STREAM；backlog≈未完成+已完成队列上限，**≠最大并发** |
| `accept` | 阻塞等连接；返回**新 conn fd**；可取对端地址 |
| `connect` | STREAM：三次握手；DGRAM：只记默认对端 |

Demo：[`code/`](./code/)（UNIX 流式极简 C/S）

---

## 56.6–56.8 I/O 与关闭

`read`/`write` 可用；`send`/`recv` 带 flags（`MSG_PEEK` 等）。  
STREAM 无边界 → 应用层分包。

| | `close` | `shutdown(how)` |
|--|---------|-----------------|
| 语义 | fd 引用 −1；归零才断 | 直接关通道；`SHUT_RD/WR/RDWR` |
| 半关闭 | 难 | **`SHUT_WR` 发 FIN，仍可读** |
| fork 后 | 一方 close 未必断 TCP | 推荐显式 shutdown |

默认阻塞；`O_NONBLOCK` → `EAGAIN`/`EWOULDBLOCK`（配 epoll 等）。

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

## 陷阱

1. STREAM 无消息边界  
2. 勿关 listener；关的是 conn fd  
3. UNIX DGRAM ≠ UDP 可靠性  
4. fork 复制 fd → close≠断连  
5. backlog≠最大客户端数  
6. addrlen 类型用 socklen_t  

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

## 参考

- Kerrisk · TLPI Ch56  
- `man 2 socket` · `bind` · `listen` · `accept` · `connect` · `shutdown`
