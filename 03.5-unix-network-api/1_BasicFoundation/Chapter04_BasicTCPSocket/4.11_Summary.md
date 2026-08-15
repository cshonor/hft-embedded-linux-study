# 4.11 小结

> [study.md](../study.md) · [Ch 5](../Chapter05_TCP_Client_Server_Demo/study.md)

---

## 核心主旨

TCP 套接字编程的**物理骨架**。

---

## 章节核心提炼

| 角色 | API |
|------|-----|
| **客户** | `socket` + `connect` |
| **服务器** | `socket` + `bind` + `listen` + `accept` |

| 并发关键 | 要点 |
|----------|------|
| **backlog** | 未完成 + 已完成队列；满则**丢 SYN 不回 RST** |
| **fork + 引用计数** | **父 close(connfd)**，**子 close(listenfd)** |

---

## 个人学习总结

（待填）
