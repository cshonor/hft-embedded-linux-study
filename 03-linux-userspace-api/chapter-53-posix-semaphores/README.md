# TLPI 第 53 章 — POSIX Semaphores

**优先级**：🔴（进程/线程同步）  
**前置**：[Ch52 POSIX mq](../chapter-52-posix-message-queues/notes.md) · [Ch47 SysV sem](../chapter-47-sysv-semaphores/notes.md)  
**后置**：[Ch54 POSIX 共享内存](../chapter-54-posix-shared-memory/notes.md)

---

## 小节目录

- [53.1 概念](./notes/53.1-concepts.md)
- [53.2 命名](./notes/53.2-section-53-2.md)
- [53.3 操作（两形态共用）](./notes/53.3-operations.md)
- [53.4 匿名](./notes/53.4-section-53-4.md)
- [53.5 fork / exec](./notes/53.5-fork-exec.md)
- [53.6 vs SysV · vs mutex](./notes/53.6-sysv-mutex.md)

---

## 章节目标


命名 / 匿名；wait/post/timed；fork/exec；vs SysV / mutex。

---


---

## 陷阱


1. 跨进程匿名：`pshared`+共享内存  
2. API 混用 destroy/close  
3. 无 SEM_UNDO  
4. timed 绝对时间  
5. getvalue TOCTOU  
6. pshared=0 fork 后不可跨进程  
7. 名 `/a/b` 非法  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 命名 vs 匿名；单个计数器 |
| 2 | open 原子初值；无 init 竞态 |
| 3 | 跨进程匿名 → 共享内存 |
| 4 | 无 SEM_UNDO；无所有权 |
| 5 | 线程互斥用 mutex |
| 6 | unlink / 全 close 销毁命名对象 |

---


---

## 参考


- Kerrisk · TLPI Ch53  
- `man 3 sem_overview` · `sem_open` · `sem_init`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
