# TLPI 第 50 章 — Virtual Memory Operations

**优先级**：🔴（低延迟 / JIT / 大映射调优）  
**前置**：[Ch49 mmap](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/notes.md)

---

## 小节目录

- [50.1 总览](./notes/50.1-section-50-1.md)
- [50.2 `mprotect`](./notes/50.2-mprotect.md)
- [50.3 `mlock` / `mlockall`](./notes/50.3-mlock-mlockall.md)
- [50.4 `mincore`](./notes/50.4-mincore.md)
- [50.5 `madvise`](./notes/50.5-madvise.md)

---

## 章节目标


`mprotect` · `mlock*` · `mincore` · `madvise`；页对齐约束；实时锁定与 advice 语义。

---


---

## 对比速记


| 调用 | 作用 | 权限 |
|------|------|------|
| mprotect | 改 r/w/x | 通常不需 root |
| mlock* | 禁 swap | ulimit / 特权 |
| mincore | 驻留快照 | 否 |
| madvise | 访问提示 | 否 |

---


---

## 陷阱


1. 未页对齐 → `EINVAL`  
2. mprotect 越权 → `EACCES`  
3. mlock 非引用计数叠加  
4. mincore 不可靠同步  
5. `MADV_DONTNEED` + PRIVATE 丢数据（Linux）  
6. `MCL_FUTURE` 耗尽锁定额  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 四件套：protect / lock / mincore / advise |
| 2 | 页对齐；JIT 写完再 RX |
| 3 | mlock 抗 swap；ulimit；exec 解锁 |
| 4 | mincore 仅快照 |
| 5 | madvise 非强制；DONTNEED 移植坑 |
| 6 | 大量 mlock 挤内存 |

---


---

## 参考


- Kerrisk · TLPI Ch50（非「第 45 章」误标）  
- `man 2 mprotect` · `mlock` · `mincore` · `madvise`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
