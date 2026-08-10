# 1.1 软件调试的起源与误区

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

- 调试 (debugging) 词源：从 "bug"（飞蛾卡在继电器中）演变而来
- 常见调试误区：过度依赖调试器、忽视日志、不先理解代码就调试
- 内核调试的特殊性：不能像用户空间程序那样随意用 GDB 附加
- 内核 bug 的影响：整个系统崩溃，可能导致数据损坏

## 关键概念

| 误区 | 正确做法 |
|------|---------|
| 直接上调试器 | 先阅读代码、理解逻辑 |
| 随机修改试错 | 形成假设 → 验证 → 修复 |
| 忽视日志 | printk/dmesg 是第一信息源 |
| 忽视警告 | 编译器警告和 lockdep 警告都是真实 bug |

## HFT 关联

内核模块崩溃 = 整个交易系统宕机。理解调试方法论比掌握工具更重要。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 内核调试为什么比用户空间调试更困难？

> 内核错误会导致整个系统崩溃（Oops/Panic），无法像用户空间那样用 GDB 附加到进程。内核运行在最高特权级，没有安全网。内核代码庞大复杂，并发竞争难以复现。需要特殊工具（KGDB、ftrace、kprobes）而非标准 GDB。


**Q:** Linux 内核调试和用户态调试最大的区别是什么？

> 内核调试没有 GDB 的断点/单步（除非用 KGDB），因为内核本身是调试器的宿主。内核调试主要依赖日志（printk）、跟踪（ftrace）、运行时检查（KASAN/LOCKDEP）和崩溃分析（Oops/kdump）。用户态可以直接 attach GDB。

**Q:** 生产环境内核和开发环境内核在调试能力上有什么取舍？

> 生产内核通常关闭 DEBUG_INFO、LOCKDEP、KASAN 等调试选项以减少开销（KASAN 2-3x slowdown）。但保留 panic_on_oops、kdump、kmsg 以便崩溃后分析。开发内核开启所有调试选项，以最大概率暴露 bug。

</details>

## 交叉引用

- [05-linux-kernel LKD Ch18 调试](../05-linux-kernel/)
- [05.6 ch03 printk](chapter-03-printk/notes/section-3-1.md)
