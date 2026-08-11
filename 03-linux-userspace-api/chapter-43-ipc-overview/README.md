# TLPI 第 43 章 — Interprocess Communication Overview

**优先级**：🟡（IPC 全书地图）  
**前置**：[Ch42 共享库高级 / dlopen](../chapter-42-shared-libraries-advanced/notes.md)  
**后置**：[Ch44 管道与 FIFO](../chapter-44-pipes-fifos/notes.md)

---

## 小节目录

- [43.1 三大类](./notes/43.1-section-43-1.md)
- [43.2 命名与句柄（表 43-1 精简）](./notes/43.2-section-43-2.md)
- [43.2 可访问范围 + 持久性（表 43-2 · 高频）](./notes/43.2-persistence-access-scope.md)
- [43.3 SysV vs POSIX（铺垫）](./notes/43.3-sysv.md)
- [43.4 –43.5 选型原则](./notes/43.4-selection.md)

---

## 章节目标


三类 IPC；通信再分「拷贝型 vs 共享内存」；命名/句柄表；持久性；SysV vs POSIX 铺垫；选型原则。

---


---

## 后续阅读路线


| 章 | 目录 |
|----|------|
| 44 | [pipes-fifos](../chapter-44-pipes-fifos/notes.md) |
| 45–48 | SysV IPC 全套 |
| 49–50 | [memory-mappings](../chapter-49-memory-mappings/notes.md) 等 |
| 51–54 | POSIX IPC |
| 55 | 文件锁 |
| 56+ | Socket |

---


---

## 思考题（43.6）


1. 匿名管道无路径名，只能靠继承 fd → 仅相关进程。  
2. 进程持久 vs 内核持久；SysV 忘删 → 泄漏。  
3. 共享内存可见同一数据，无原子/序保证 → 需同步。  
4. 无名 sem 无名字可打开，须放在双方都能看见的共享区。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 通信 / 同步 / 信号 三类 |
| 2 | 拷贝型 vs 共享内存（最快且须同步） |
| 3 | 命名：路径·fd vs SysV key·id |
| 4 | 持久：进程 / 内核 / 文件系统 |
| 5 | 跨机只用 Internet socket |
| 6 | 内核持久记得 unlink/显式删 |

---


---

## 参考


- Kerrisk · TLPI Ch43（非 Ch17）  
- 后续各章 `man` 页见对应笔记


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
