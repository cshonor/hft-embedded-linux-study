# 2.4 工具选择决策树

> ⬜ 跳读

## 本节要点

```
内核 Bug
├── 崩溃 (Crash/Oops)
│   ├── 看 Oops 日志 → addr2line 定位源码行
│   └── 需要更多信息 → KGDB 单步调试
├── 挂死 (Hang/Lockup)
│   ├── soft lockup → ftrace 追踪调用链
│   └── hard lockup → NMI watchdog + kdump
├── 内存错误
│   ├── 越界/UAF → KASAN
│   ├── 泄漏 → kmemleak
│   └── 生产环境 → KFENCE
├── 并发问题
│   ├── 死锁 → LOCKDEP
│   ├── 数据竞争 → KCSAN
│   └── 时序问题 → ftrace + kprobes
└── 性能问题
    ├── 延迟 → ftrace function_graph
    └── CPU 热点 → perf record
```

## HFT 关联

HFT 内核模块调试的典型路径：
1. 崩溃 → Oops 日志 + addr2line → 定位代码行
2. 挂死 → ftrace 看哪个函数卡住
3. 延迟毛刺 → ftrace function_graph 测量函数耗时
4. 内存泄漏 → kmemleak 定期检测

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 发现内核 soft lockup 警告后，第一步应该做什么？

> 查看完整 dmesg 日志中的 soft lockup 报告，它会打印当前 CPU 上正在执行的调用栈 (call trace)。根据调用栈判断是哪个函数卡住（常见原因：死循环、自旋锁持有过久、长时间不允许调度的代码段）。然后用 ftrace 确认假设。


**Q:** 一个内核模块出现空指针解引用 panic，你会按什么顺序使用调试工具？

> (1) 收集 Oops 日志（dmesg）；(2) addr2line 定位源码行；(3) 如果是偶发，加 KASAN 重现（可能检测到 UAF 导致的空指针）；(4) 如果仍无法复现，加 kprobe 在崩溃点前打印相关变量；(5) 最终用 KGDB 设条件断点。

</details>

## 交叉引用

- [05.6 ch07 Oops](chapter-07-oops/notes/section-7-1.md)
- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/section-5-2.md)
