## 为何内核调试更难

| 用户空间 | 内核空间 |
|----------|----------|
| gdb、core dump | 一点错 → **Oops / panic** |
| 进程隔离 | **整机** 受影响 |
| 可频繁打印 | `printk` 淹没或 **拖死** 系统 |

→ **Ch 2** 无内存保护 · **Ch 5** 进程 vs 中断上下文



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核调试比用户态难在哪？

<details><summary>答案</summary>

1) 无 GDB 附加（内核崩溃 = 系统崩溃）；2) 无 core dump（oops 信息有限）；3) 并发竞争难复现（SMP + 抢占）；4) 无 libc 调试工具（valgrind/gdb 不能用）；5) 错误后果严重（数据损坏/panic）。内核调试主要靠：printk/oops 分析/kgdb/ftrace/eBPF。HFT 定制驱动调试还要考虑实时性约束。

</details>

</details>
---
