# 2.1 内核调试的挑战

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

- 内核代码量大（6.x 约 3000 万行），定位问题范围困难
- 并发竞争：多核 + 中断 + 抢占导致 bug 难以复现
- 不可重现性：某些 bug 只在特定时序/负载下触发
- 副作用：调试本身改变了时序，可能掩盖 bug (Heisenbug)

## Heisenbug

> 加了 printk 就不复现，去掉就复现——因为 printk 改变了时序。

应对策略：
1. 使用无侵入工具（ftrace、kprobes）
2. 使用硬件调试器（KGDB 单步）
3. 记录而非暂停（trace_printk 而非 printk）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 什么是 Heisenbug？为什么内核调试中常见？

> Heisenbug 指添加调试代码后 bug 消失的现象（类比海森堡测不准原理）。内核中常见因为：1) 调试代码改变了时序，掩盖了竞争条件；2) printk 序列化输出改变并发行为；3) 编译器优化在调试模式下不同。应使用 ftrace 等无侵入工具减少副作用。


**Q:** 内核调试中最难的问题类型是什么？为什么？

> 竞态条件（race condition）最难——依赖特定时序，难以复现。其次是内存损坏（memory corruption）——症状出现在远离 root cause 的地方。应对：竞态用 LOCKDEP/KCSAN，内存损坏用 KASAN/KFENCE。

**Q:** 为什么 "增加 printk 后 bug 消失" 是常见的调试困境？

> printk 改变时序——额外的 I/O 操作延迟改变了竞态窗口。这本质是 Heisenbug。解决：用 trace_printk（写入 trace buffer 不做 I/O）或 ftrace function tracer（不修改源码）。

</details>

## 交叉引用

- [05.6 ch03 trace_printk](chapter-03-printk/notes/section-3-5.md)
- [05.6 ch09 ftrace](chapter-09-ftrace/notes/section-9-2.md)
