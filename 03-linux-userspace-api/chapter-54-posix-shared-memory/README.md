# TLPI 第 54 章 — POSIX Shared Memory

**优先级**：🔴（POSIX IPC 终章；无关进程高速区）  
**前置**：[Ch53 POSIX sem](../chapter-53-posix-semaphores/notes.md) · [Ch48 SysV shm](../chapter-48-sysv-shared-memory/notes.md) · [Ch49 mmap](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch55 File Locking](../chapter-55-file-locking/notes.md)

---

## 小节目录

- [54.1 原理](./notes/54.1-principle.md)
- [54.2 –54.4 标准流程](./notes/54.2-section-54-2.md)
- [54.5 生命周期](./notes/54.5-lifecycle.md)
- [54.6 vs System V shm](./notes/54.6-system-shm.md)

---

## 章节目标


`shm_open`/`ftruncate`/`mmap`/`unlink`；生命周期；vs SysV shm / 匿名共享 mmap；工程模板。

---


---

## IPC 路线收束（Ch43–54）


Pipe/FIFO → SysV 三件套 → mmap/VM → POSIX 三件套。  
下一章地图：**Ch55 文件锁** → 再进 Socket（Ch56+）。

---


---

## 陷阱


1. 忘 ftruncate → SIGBUS  
2. `MAP_PRIVATE` 不共享  
3. 无同步竞态  
4. 名格式错误  
5. 混淆匿名共享 mmap  
6. 忘 unlink → `/dev/shm` 残留  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | open → ftruncate → mmap SHARED |
| 2 | 新建 size=0；SIGBUS 常见因未扩 |
| 3 | unlink + 全 close/munmap 销毁 |
| 4 | 须同步；段内用 offset |
| 5 | 无关进程用 shm_open，非匿名 SHARED |
| 6 | 新项目常优于 SysV shm |

---


---

## 参考


- Kerrisk · TLPI Ch54  
- `man 3 shm_open` · `man 7 shm_overview`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
