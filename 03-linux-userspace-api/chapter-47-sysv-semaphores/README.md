# TLPI 第 47 章 — System V Semaphores

**优先级**：🔴（同步原语；配共享内存）  
**前置**：[Ch45 导论](../chapter-45-sysv-ipc-intro/notes.md) · [Ch46 消息队列](../chapter-46-sysv-message-queues/notes.md)  
**后置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/notes.md)

---

## 小节目录

- [47.1 概念](./notes/47.1-concepts.md)
- [47.2 –47.4 API](./notes/47.2-api.md)
- [47.5 初始化竞态（经典坑）](./notes/47.5-initialization.md)
- [47.8 `SEM_UNDO`](./notes/47.8-semundo.md)
- [47.9 二元信号量 ≈ 互斥](./notes/47.9-semaphore-mutex.md)
- [47.10 –47.11 限额 · 缺陷](./notes/47.10-defects-quotas.md)

---

## 章节目标


信号量集；`semget`/`semctl`/`semop`；初始化竞态；`SEM_UNDO`；二元互斥；限额与缺陷。

---


---

## 思考题要点


1. 上节安全初始化。  
2. `SEM_UNDO` 兜底 ≠ 防死锁；无超时仍可永久阻塞。  
3. 多 `sembuf` 原子 → 减死锁。  
4. `IPC_RMID` → 阻塞 `semop` 失败返回（`EIDRM`）。  
5. 无所有者 vs mutex。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 信号量**集**；get/ctl/op |
| 2 | get 不初始化 → EXCL 创建者 SETVAL |
| 3 | sem_op：+/−/0；多 op 原子 |
| 4 | SEM_UNDO = 退出撤销 |
| 5 | 无所有权；内核持久 |
| 6 | 新项目用 POSIX sem |

---


---

## 参考


- Kerrisk · TLPI Ch47（非「第 20 章」误标）  
- `man 2 semget` · `semop` · `semctl`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
