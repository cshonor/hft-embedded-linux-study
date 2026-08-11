# TLPI 第 63 章 — Alternative I/O Models

**优先级**：🔴（C10K / 网络服务基础）  
**前置**：[Ch62 Terminals](../chapter-62-terminals/notes.md) · Socket  
**后置**：[Ch64 Pseudoterminals](../chapter-64-pseudoterminals/notes.md)

---

## 小节目录

- [63.1 五模型（读 socket 两阶段：就绪 → 拷到用户态）](./notes/63.1-socket.md)
- [63.2 –63.3 `select` / `poll`](./notes/63.2-select-poll.md)

---

## 章节目标


五模型；`select`/`poll`；与非阻塞/SIGIO/AIO 对比；为 epoll 铺垫。

---


---

## Linux 延伸：`epoll`（本章铺垫）


解决：就绪集拷贝、线性扫、大连接。  
`epoll_create` → `epoll_ctl` → `epoll_wait`；边缘/水平触发。生产网络服务首选（TLPI 本章不深挖，HFT/高并发必补）。

---


---

## 对比速记


| | 阻塞点 | CPU | 场景 |
|--|--------|-----|------|
| 阻塞 IO | read | 低 | 简单工具 |
| 非阻塞轮询 | 无 | 极高 | 勿单用 |
| select/poll | 多路调用 | 中，随 fd 升 | 中小并发 |
| SIGIO | 主循环 | — | 少用 |
| AIO | 无（真异步） | 低 | 文件为主 |

---


---

## 陷阱


1. 多路复用 ≠ 异步  
2. 阻塞 fd + 多路复用易卡死事件环  
3. FIN → POLLIN，须 `recv==0` 关连  
4. select 的 timeval 可能被改  
5. nfds 漏写 max+1  
6. 关 fd 后未从集合摘掉  
7. `EAGAIN` ≠ 真错误  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 五模型；就绪 vs 拷贝两阶段 |
| 2 | 多路复用=同步等就绪 |
| 3 | select：FD_SETSIZE；每次重填 |
| 4 | poll：无 1024 限；仍线性 |
| 5 | 信号驱动≠AIO |
| 6 | 大连接 → epoll |

---


---

## 参考


- Kerrisk · TLPI Ch63  
- `man 2 select` · `poll` · `epoll`（Linux）


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
