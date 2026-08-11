# TLPI 第 51 章 — Introduction to POSIX IPC

**优先级**：🟡（POSIX 三件套地图；对标 SysV）  
**前置**：[Ch45–48 SysV](../chapter-45-sysv-ipc-intro/notes.md) · [Ch49–50 mmap/VM](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch52 mq](../chapter-52-posix-message-queues/notes.md) → [Ch53 sem](../chapter-53-posix-semaphores/notes.md) → [Ch54 shm](../chapter-54-posix-shared-memory/notes.md)

---

## 小节目录

- [51.1 三类机制](./notes/51.1-mechanism.md)
- [51.2 统一模型](./notes/51.2-model.md)
- [51.3 POSIX vs System V（核心表）](./notes/51.3-system.md)
- [51.4 三件预览](./notes/51.4-section-51-4.md)

---

## 章节目标


三类 POSIX IPC；文件风 API 与引用计数；vs SysV 总表；选型。

---


---

## 选型（TLPI 倾向）


1. 新 Linux → **优先 POSIX**；事件驱动 → POSIX mq  
2. 老 UNIX / 遗留 → SysV  
3. 大批量 → shm + POSIX sem  
4. 简单流 → pipe / UNIX 域 socket  

---


---

## 思考题要点


1. unlink 删名；close 降引用；全 close 销毁。  
2. mq 返回 fd；SysV 非 fd。  
3. 命名：无关进程；匿名：线程/共享区。  
4. SysV 集可原子多 op；POSIX 单计数器更简单。  
5. unlink 后已打开句柄仍有效。  
6. shm_open → ftruncate → mmap。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | open / close / unlink 文件风 |
| 2 | unlink≠立刻毁；等最后 close |
| 3 | mq fd 可 epoll；SysV 不能 |
| 4 | 名 `/foo`；Linux 在 /dev/mqueue·shm |
| 5 | 新项目优先 POSIX |
| 6 | 细节见 Ch52–54 |

---


---

## 参考


- Kerrisk · TLPI Ch51（非「第 31 章」误标）  
- `man 7 mq_overview` · `sem_overview` · `shm_overview`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
