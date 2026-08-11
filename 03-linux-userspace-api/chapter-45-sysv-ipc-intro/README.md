# TLPI 第 45 章 — Introduction to System V IPC

**优先级**：🟡（SysV 三机制共用模型）  
**前置**：[Ch44 管道与 FIFO](../chapter-44-pipes-fifos/notes.md)  
**后置**：[Ch46 SysV 消息队列](../chapter-46-sysv-message-queues/notes.md) → [Ch47 信号量](../chapter-47-sysv-semaphores/notes.md) → [Ch48 共享内存](../chapter-48-sysv-shared-memory/notes.md)

---

## 小节目录

- [45.1 三类对象 · 统一 API](./notes/45.1-api.md)
- [45.2 Key（`key_t`）](./notes/45.2-keyt.md)
- [45.3 –45.4 标识符 · `ipc_perm`](./notes/45.3-ipcperm.md)
- [45.5 `get()` 算法（高频）](./notes/45.5-get.md)
- [45.6 内核持久 · 运维](./notes/45.6-ops.md)
- [45.7 –45.8 缺陷与限额](./notes/45.7-defects-quotas.md)
- [45.9 `IPC_RMID` 差异](./notes/45.9-ipcrmid.md)

---

## 章节目标


三类对象共用的 key/`get`/`ctl`/操作 API；`ipc_perm`；`get` 算法；内核持久与 `ipcs`/`ipcrm`；限额；`IPC_RMID` 差异；与 POSIX 对比铺垫。

---


---

## 思考题要点


1. `IPC_PRIVATE`：亲缘；`ftok`：无关进程约定路径。  
2. `ftok`：固定路径文件勿删；或固定 key + 约定文档。  
3. key≠id；id 非 fd → 无 epoll。  
4. `CREAT|EXCL`：独占创建。  
5. 内核持久 + 忘 RMID → 泄漏占限额。  
6. shm 的 RMID 须等 detach。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | get / ctl / 操作 三套对称 API |
| 2 | key 定位；id 操作；**非 fd** |
| 3 | `CREAT\|EXCL` 独占创建 |
| 4 | 内核持久 → `IPC_RMID` / `ipcrm` |
| 5 | `ftok` 吃 inode；文件重建 key 变 |
| 6 | shm 的 RMID 等全部 shmdt |

---


---

## 参考


- Kerrisk · TLPI **Ch45**（非中文分册「第 10 章」）  
- `man 3 ftok` · `man 1 ipcs` · `man 1 ipcrm`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
