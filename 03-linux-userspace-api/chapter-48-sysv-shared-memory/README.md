# TLPI 第 48 章 — System V Shared Memory

**优先级**：🔴（无拷贝；配信号量）  
**前置**：[Ch47 SysV 信号量](../chapter-47-sysv-semaphores/notes.md)  
**后置**：[Ch49 mmap](../chapter-49-memory-mappings/notes.md) · [Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/notes.md)

---

## 小节目录

- [48.1 原理](./notes/48.1-principle.md)
- [48.2 –48.3 · 48.7 API](./notes/48.2-api.md)
- [48.6 陷阱：段内勿存指针](./notes/48.6-pointer-pitfalls-segment.md)
- [48.4 工程模型](./notes/48.4-model.md)
- [48.9 限额 · 运维](./notes/48.9-quotas-ops.md)

---

## 章节目标


`shmget`/`shmat`/`shmdt`/`shmctl`；延迟 `IPC_RMID`；指针陷阱；shm+sem 模型；限额与对比。

---


---

## 优缺点


✅ 无拷贝、多进程同区。  
❌ 非 fd、无 epoll「数据就绪」、须自管同步、内核持久易漏、API 老。  
→ 新项目常选 **POSIX shm + mmap**。

拷贝对比：pipe/mq ≈ 两次拷贝；shm ≈ **零业务拷贝**（仍有页表建立成本）。

---


---

## SysV IPC 速记（四章收束）


| | mq | sem | shm |
|--|----|-----|-----|
| 句柄 | 非 fd | 非 fd | 非 fd |
| 持久 | 内核 | 内核 | 内核 |
| RMID | 立即标记 | 立即标记 | **等 nattch=0** |
| 角色 | 消息 | 同步 | 数据 |

---


---

## 思考题要点


1. RMID 延迟到 nattch=0。  
2. attach/detach；exec 全 detach。  
3. 虚址不同 → offset。  
4. 无同步 → 竞态。  
5. attach++ / detach--；nattch=0 + DEST → 回收。  
6. pipe/mq 拷贝 vs shm 共享页。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 同物理页；无拷贝；须同步 |
| 2 | shmat(NULL)；exec 自动 detach |
| 3 | RMID 延迟；看 shm_nattch |
| 4 | 段内只用 offset |
| 5 | shm + sem 经典配对 |
| 6 | 新项目可 POSIX shm |

---


---

## 参考


- Kerrisk · TLPI Ch48（非「第 21 章」误标）  
- `man 2 shmget` · `shmat` · `shmdt` · `shmctl`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
