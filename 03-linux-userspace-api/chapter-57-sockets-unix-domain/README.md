# TLPI 第 57 章 — Sockets: UNIX Domain

**优先级**：🔴（本机低延迟 IPC）  
**前置**：[Ch56 Socket 导论](../chapter-56-sockets-intro/notes.md)  
**后置**：[Ch58 TCP/IP 基础](../chapter-58-tcpip-fundamentals/notes.md)

---

## 小节目录

- [57.1 `struct sockaddr_un`](./notes/57.1-struct-sockaddrun.md)
- [57.2 STREAM](./notes/57.2-stream.md)
- [57.3 DGRAM（高频）](./notes/57.3-dgram.md)
- [57.4 权限](./notes/57.4-permission.md)
- [57.5 `socketpair`](./notes/57.5-socketpair.md)
- [57.6 抽象命名空间（Linux）](./notes/57.6-namespace-abstraction.md)

---

## 章节目标


`sockaddr_un`；STREAM/DGRAM；权限；`socketpair`；抽象命名空间；工程选型。

---


---

## 选型（嵌入式 + HFT）


1. 本机组件通信 → UDS（优先于 127.0.0.1 TCP）  
2. 守护进程 → 抽象名（Linux）  
3. 父子临时双向 → `socketpair`  
4. 要消息边界且本机 → UNIX DGRAM  

---


---

## 思考题要点


1. 崩溃残留：启动 `unlink` 或改用抽象名。  
2. 抽象不受 umask/文件权限。  
3. fork 后关无用 fd；需半关闭用 `shutdown`。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 路径型 bind 前 unlink |
| 2 | UNIX DGRAM ≠ UDP：本地可靠 |
| 3 | connect 需写权限 + 目录 x |
| 4 | socketpair：无名互连对 |
| 5 | 抽象：`path[0]=0`，无文件 |
| 6 | 本机延迟：UDS > loopback TCP |

---


---

## 参考


- Kerrisk · TLPI Ch57  
- `man 7 unix` · `man 2 socketpair`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
