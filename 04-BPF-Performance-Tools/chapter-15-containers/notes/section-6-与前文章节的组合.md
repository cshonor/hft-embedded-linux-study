# 6. 与前文章节的组合

| 问题 | 工具链 |
|------|--------|
| 容器 CPU 慢 | `runqlat --pidnss` + `pidnss` + host `runqlat` |
| 容器 I/O 慢 | **`blkthrot`** + `biolatency` + `fileslower`（Ch 8） |
| 容器网络 | Ch 10 工具在 **host** 跑 + **netns** 过滤（进阶） |
| 内存 OOM | Ch 7 `oomkill` + cgroup `memory.events` |
| 应用栈 | Ch 13 — **仍须在 host** 对容器 PID trace |

---
