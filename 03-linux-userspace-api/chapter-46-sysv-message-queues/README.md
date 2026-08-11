# TLPI 第 46 章 — System V Message Queues

**优先级**：🔴（首个完整 SysV 实例；消息 vs 字节流）  
**前置**：[Ch45 SysV IPC 导论](../chapter-45-sysv-ipc-intro/notes.md)  
**后置**：[Ch47 SysV 信号量](../chapter-47-sysv-semaphores/notes.md)

---

## 小节目录

- [46.1 特性（相对 Pipe/FIFO）](./notes/46.1-pipe-fifo.md)
- [46.2 API](./notes/46.2-api.md)
- [46.3 –46.5 `msqid_ds` · 限额 · 运维](./notes/46.3-msqidds.md)
- [46.6 –46.7 模型与缺陷](./notes/46.6-model-defects.md)

---

## 章节目标


`msgget`/`msgsnd`/`msgrcv`/`msgctl`；`msgtyp` 三规则；`msqid_ds` 与限额；与 pipe 对比；局限。

---


---

## 思考题要点


1. `msgtyp` 0 / >0 / <0（上表）。  
2. `EINTR`；SA_RESTART 无效。  
3. mq RMID 立即标记 vs shm 等 detach。  
4. `MSG_NOERROR` vs `E2BIG`。  
5. id≠fd。  
6. 崩溃 + 内核持久 → `ipcrm`/`IPC_RMID`。  
7. **0 长度消息合法**。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 消息边界 + `mtype>0` |
| 2 | `msgtyp`：0 / 等于 / ≤\|n\| 最小 |
| 3 | `msgsz` 不含 mtype；正文可 0 |
| 4 | 中断 → EINTR，无 RESTART |
| 5 | 非 fd；内核持久；IPC_RMID |
| 6 | 新项目优先 POSIX mq / UDS |

---


---

## 参考


- Kerrisk · TLPI Ch46（非「第 19 章」误标）  
- `man 2 msgget` · `msgsnd` · `msgrcv` · `msgctl`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
