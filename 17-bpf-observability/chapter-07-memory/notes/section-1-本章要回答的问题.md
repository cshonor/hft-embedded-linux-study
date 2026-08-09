# 1. 本章要回答的问题

| 宏观现象 | BPF 要定位的 |
|----------|--------------|
| RSS 持续增长 | 谁 `malloc`/`mmap` 了没释放？→ `memleak`、`brkstack`、`mmapsnoop` |
| 首次访问变慢 | 哪些路径触发 **缺页**？→ `faults`、`ffaults` |
| 系统卡顿、分配变慢 | **直接回收**？→ `drsnoop`、`vmscan` |
| 进程被杀 | **OOM** 元凶？→ `oomkill` |
| 换入换出 | 谁在用 Swap？→ `swapin` |

**传统工具** 给 **数字**；BPF + **栈追踪** 给 **路径**。

---
