# 1.1 软件调试的起源与误区

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

- 调试 (debugging) 词源：1947 年 Grace Hopper 在 Harvard Mark II 计算机中发现飞蛾卡在继电器中，用胶带将飞蛾贴在日志本上标注 "first actual case of bug being found"
- 内核调试的特殊性：不能像用户空间程序那样随意用 GDB 附加，内核本身是调试器的宿主
- 内核 bug 的影响：整个系统崩溃（Oops/Panic），可能导致数据损坏、文件系统损坏
- 调试方法论比工具更重要：理解代码逻辑 → 形成假设 → 验证假设 → 修复

## 调试的历史脉络

| 年代 | 事件 | 意义 |
|------|------|------|
| 1947 | Mark II 继电器中的飞蛾 | "Bug" 一词的起源 |
| 1960s | 交互式调试器 (DDT) | 第一个运行时调试工具 |
| 1970s | gdb/dbx | 源码级符号调试 |
| 1990s | Linux 引入 ksymoops | 内核栈回溯符号化 |
| 2000s | ftrace/kprobes/KASAN | 内核专用运行时分析工具 |
| 2010s | eBPF/bpftrace | 可编程的内核观测 |
| 2020s | KCSAN/KFENCE | 低开销生产环境检测 |

## 常见调试误区

| 误区 | 正确做法 | 为什么 |
|------|---------|--------|
| 直接上调试器 | 先阅读代码、理解逻辑 | 不理解代码的调试是盲目的 |
| 随机修改试错 | 形成假设 → 验证 → 修复 | 随机修改可能引入新 bug |
| 忽视日志 | printk/dmesg 是第一信息源 | 日志记录了故障发生时的状态 |
| 忽视编译器警告 | 修复所有 warning | 警告往往是真实 bug 的线索 |
| 忽视 lockdep 警告 | 认真对待每一处锁依赖警告 | lockdep 警告几乎 100% 是真实死锁 |
| 过度依赖 printk | 优先用 ftrace/kprobes | printk 改变时序，可能掩盖竞态 |

## 内核调试 vs 用户空间调试

| 维度 | 用户空间 | 内核空间 |
|------|---------|---------|
| 调试器附加 | `gdb attach PID` | 需要 KGDB/QEMU，不能直接附加 |
| 崩溃影响 | 进程退出，系统继续 | 整个系统挂掉 (Panic) |
| 断点/单步 | 硬件断点，随时可用 | 需要 KGDB 或 KDB |
| 内存检查 | Valgrind/ASAN | KASAN/KFENCE/kmemleak |
| 性能分析 | perf/gprof | perf/ftrace（共享工具） |
| 日志输出 | printf，开销低 | printk 可能阻塞（串口输出） |
| 并发模型 | 线程/进程 | SMP + 中断 + 抢占 + softirq |

## 调试方法论

```
1. 复现 (Reproduce)
   └─ 稳定复现是调试的前提
2. 隔离 (Isolate)
   └─ 二分法缩小触发条件
3. 假设 (Hypothesize)
   └─ 根据代码逻辑形成假设
4. 验证 (Verify)
   └─ 用工具验证假设（ftrace/kprobes）
5. 修复 (Fix)
   └─ 修复 root cause，不是 symptom
6. 回归 (Regression)
   └─ 确保修复不引入新问题
```

## HFT 关联

内核模块崩溃 = 整个交易系统宕机。HFT 系统对内核稳定性的要求极高：

- 交易网卡驱动 bug → 网卡停止收包 → 整个交易系统无法收行情
- 内存损坏 → 随机性的订单数据错误
- 死锁 → 系统挂死，需要人工干预重启

理解调试方法论比掌握工具更重要——在市场交易时间内，每分钟宕机损失可能数百万。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 内核调试为什么比用户空间调试更困难？

> 内核错误会导致整个系统崩溃（Oops/Panic），无法像用户空间那样用 GDB 附加到进程。内核运行在最高特权级，没有安全网。内核代码庞大复杂（6.x 约 3000 万行），并发竞争难以复现。需要特殊工具（KGDB、ftrace、kprobes）而非标准 GDB。

**Q2:** 什么是 "Heisenbug"？为什么内核调试中常见？

> Heisenbug 指添加调试代码后 bug 消失的现象（类比海森堡测不准原理）。内核中常见因为：调试代码改变了时序，掩盖了竞争条件；printk 序列化输出改变并发行为；编译器优化在调试模式下不同。应使用 ftrace 等无侵入工具减少副作用。

**Q3:** Linux 内核调试和用户态调试最大的区别是什么？

> 内核调试没有 GDB 的断点/单步（除非用 KGDB），因为内核本身是调试器的宿主。内核调试主要依赖日志（printk）、跟踪（ftrace）、运行时检查（KASAN/LOCKDEP）和崩溃分析（Oops/kdump）。用户态可以直接 attach GDB。

**Q4:** 生产环境内核和开发环境内核在调试能力上有什么取舍？

> 生产内核通常关闭 DEBUG_INFO、LOCKDEP、KASAN 等调试选项以减少开销（KASAN 2-3x slowdown）。但保留 panic_on_oops、kdump、kmsg 以便崩溃后分析。开发内核开启所有调试选项，以最大概率暴露 bug。

**Q5:** 调试方法论中 "修复 root cause" 和 "修复 symptom" 的区别？

> 修复 symptom：增加一个 NULL 检查避免 panic，但不查找为什么指针为 NULL。修复 root cause：追踪指针为 NULL 的原因（如 UAF、初始化顺序错误）。只修 symptom 会导致 bug 在其他路径再次出现。

</details>

## 交叉引用

- [05.6 ch02 内核调试挑战](../../chapter-02-approaches/notes/01-kernel-debug-challenges.md)
- [05.6 ch03 printk 基础](../../chapter-03-printk/notes/01-printk-basics-loglevel.md)
- [05-linux-kernel LKD Ch18 调试](../../../05-linux-kernel/)
