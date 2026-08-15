# 2.4 工具选择决策树

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

根据 bug 类型选择合适的调试工具——没有万能工具，只有最合适的工具。

## 工具选择决策树

```
内核 Bug
├── 崩溃 (Crash/Oops)
│   ├── 看 Oops 日志 → addr2line 定位源码行
│   │   └── 需要更多信息 → KGDB 单步调试
│   └── 偶发崩溃 → KASAN 检测是否 UAF
│       └── 仍无法复现 → kdump 保留现场 + ftrace 记录
│
├── 挂死 (Hang/Lockup)
│   ├── soft lockup → ftrace 追踪调用链
│   │   └── 确认死循环 → KGDB 断点
│   ├── hard lockup → NMI watchdog + kdump
│   └── 进程卡住 → ps/ftrace 看阻塞位置
│       └── 等锁 → /proc/lockdep 检查
│
├── 内存错误
│   ├── 越界 (OOB) → KASAN (开发) / KFENCE (生产)
│   ├── Use-After-Free → KASAN / KFENCE
│   ├── 双重释放 → KASAN
│   ├── 内存泄漏 → kmemleak
│   └── 未初始化 → KMSAN (内核 MSAN)
│
├── 并发问题
│   ├── 死锁 → LOCKDEP
│   │   ├── 锁顺序反转 → LOCKDEP 自动报告
│   │   └── 自旋锁持有过久 → lock_stat
│   ├── 数据竞争 → KCSAN
│   └── 时序问题 → ftrace + kprobes
│       └── 需要精确时序 → ftrace function_graph
│
├── 性能问题
│   ├── 延迟毛刺 → ftrace function_graph
│   │   └── 确认函数 → perf record 热点分析
│   ├── CPU 热点 → perf record/report
│   ├── 锁竞争 → lock_stat / perf lock
│   └── 内存分配 → perf kmem / ftrace kmalloc
│
└── 功能错误
    ├── 调用流程 → ftrace function tracer
    ├── 参数/返回值 → kprobes/kretprobes
    ├── 特定消息 → dyndbg (pr_debug)
    └── 热路径细节 → trace_printk
```

## 按场景选择工具

### 场景 1：内核崩溃 (Oops/Panic)

```
步骤 1: 收集信息
├── dmesg > oops.log
├── 保存 call trace
└── 确认内核版本 (uname -r)

步骤 2: 定位源码
├── addr2line -e vmlinux <addr>
├── 或 faddr2line vmlinux <func>+<offset>
└── 查看源码逻辑

步骤 3: 确认 bug 类型
├── 空指针 → 检查初始化顺序
├── 越界 → KASAN 复现
├── UAF → KASAN + 检查释放路径
└── 逻辑错误 → KGDB 单步
```

### 场景 2：系统挂死 (Hang)

```
步骤 1: 确认挂死类型
├── 能 ping 但不能 SSH → soft lockup
├── 完全无响应 → hard lockup
└── 特定进程卡住 → D 状态 (uninterruptible sleep)

步骤 2: 收集信息
├── soft lockup: dmesg 中有 "soft lockup" 报告 + call trace
├── hard lockup: 需要 NMI watchdog + kdump
└── 进程卡住: /proc/<pid>/stack + /proc/<pid>/wchan

步骤 3: 分析
├── 死循环 → ftrace 看最后执行的函数
├── 死锁 → LOCKDEP 报告 + /proc/lockdep
└── I/O 等待 → 检查存储驱动
```

### 场景 3：性能问题

```
步骤 1: 确认性能指标
├── 延迟 → cyclictest / ftrace function_graph
├── 吞吐量 → perf stat / iperf3
└── CPU 利用率 → top / perf top

步骤 2: 定位瓶颈
├── 函数耗时 → ftrace function_graph
├── CPU 热点 → perf record -g
├── 锁竞争 → lock_stat
├── 内存分配 → perf kmem stat
└── I/O 等待 → perf sched / iostat

步骤 3: 优化验证
├── 修改后重新测量
└── 对比 before/after
```

## 工具开销对比

| 工具 | 典型开销 | 生产可用 | 开发推荐 |
|------|---------|---------|---------|
| ftrace function | 1-5% | ✅ | ✅ |
| ftrace function_graph | 5-10% | ⚠️ | ✅ |
| kprobes | 1-3% | ✅ | ✅ |
| trace_printk | <0.1% | ✅ | ✅ |
| eBPF/bpftrace | 1-5% | ✅ | ✅ |
| printk | 不可预测 | ❌ | ⚠️ |
| KASAN | 50-100% | ❌ | ✅ |
| KFENCE | ~1% | ✅ | ✅ |
| LOCKDEP | 5-10% | ⚠️ | ✅ |
| KCSAN | 10-20% | ❌ | ✅ |
| KGDB | 暂停 CPU | ❌ | ✅ |
| perf record | 1-5% | ✅ | ✅ |

## HFT 关联

HFT 内核模块调试的典型路径：

| 问题 | 第一步 | 第二步 | 第三步 |
|------|--------|--------|--------|
| 崩溃 | Oops 日志 + addr2line | KASAN 复现 | KGDB 断点 |
| 挂死 | ftrace 看卡在哪 | LOCKDEP 检查死锁 | kdump 保留现场 |
| 延迟毛刺 | ftrace function_graph | perf record 热点 | lock_stat 锁竞争 |
| 内存泄漏 | kmemleak 定期检测 | 代码审查释放路径 | KASAN 确认 UAF |
| 数据竞争 | KCSAN 检测 | ftrace 追踪时序 | 代码审查锁覆盖 |

原则：**从低开销工具开始，逐步升级**。生产环境只用 ftrace/eBPF/KFENCE/dyndbg。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 发现内核 soft lockup 警告后，第一步应该做什么？

> 查看完整 dmesg 日志中的 soft lockup 报告，它会打印当前 CPU 上正在执行的调用栈 (call trace)。根据调用栈判断是哪个函数卡住（常见原因：死循环、自旋锁持有过久、长时间不允许调度的代码段）。然后用 ftrace 确认假设。

**Q2:** 一个内核模块出现空指针解引用 panic，你会按什么顺序使用调试工具？

> (1) 收集 Oops 日志（dmesg）；(2) addr2line 定位源码行；(3) 如果是偶发，加 KASAN 重现（可能检测到 UAF 导致的空指针）；(4) 如果仍无法复现，加 kprobe 在崩溃点前打印相关变量；(5) 最终用 KGDB 设条件断点。

**Q3:** HFT 系统出现偶发性延迟毛刺（偶发 100us+ 延迟），如何调试？

> (1) ftrace function_graph 追踪热路径函数耗时；(2) 配合 trace_printk 标记关键时间点；(3) 检查是否有中断/softirq 打断（ftrace trace events）；(4) 检查锁竞争（lock_stat）；(5) 检查内存分配延迟（ftrace kmalloc/kfree）。注意：不能用 printk 调试（会改变时序）。

**Q4:** LOCKDEP 报告 "possible deadlock"，但系统没有真正死锁，怎么处理？

> LOCKDEP 报告的是"潜在死锁"——锁顺序不一致，在特定时序下可能真正死锁。即使当前没死锁也必须修复。分析 LOCKDEP 报告中的锁依赖链，找到不一致的获取顺序，统一全局锁顺序。

**Q5:** 生产环境只能用低开销工具，如何调试偶发 bug？

> (1) 确保KFENCE 开启（1% 开销检测内存错误）；(2) 配置 kdump 自动保留崩溃现场；(3) 预置 ftrace/trace-cmd 脚本，出问题时一键收集；(4) 用 eBPF 做长期监控（bpftrace 可安全运行）；(5) dyndbg 预埋 pr_debug，按需开关。

</details>

## 交叉引用

- [05.6 ch07 Oops](chapter-07-oops/notes/01-oops-vs-panic.md)
- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch08 LOCKDEP](chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch09 ftrace](chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch10 soft lockup](chapter-10-panic-lockup/notes/02-soft-lockup.md)
