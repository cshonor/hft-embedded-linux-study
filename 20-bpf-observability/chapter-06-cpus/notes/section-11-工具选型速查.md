# 11. 工具选型速查

| 症状 | 优先工具 |
|------|----------|
| 整体 CPU 高 | `mpstat` → `profile` / 火焰图 |
| 延迟高但 CPU 不高 | `offcputime` |
| 怀疑调度/抢核 | `runqlat`、`runqslower` |
| 短命进程 | `execsnoop` |
| 单核打满 | `mpstat -P ALL` + `profile -C` |
| 缓存/IPC 差 | `perf stat`、`llcstat` |
| 中断风暴 | `hardirqs`、`softirqs` |
| syscall 过多 | `syscount` |


### 常见陷阱

1. **选工具时只看功能不看开销** — 同一问题多种工具可解，但开销差异大；HFT 应优先选 Map 聚合工具（低开销）而非逐行追踪工具（高开销）
2. **忽视 on-CPU 和 off-CPU 的选型逻辑** — CPU 利用率高→on-CPU 分析（profile/stackcount）；CPU 利用率低但延迟高→off-CPU 分析（offcputime/runqlat）
3. **试图用一个工具解决所有问题** — 每个工具回答一个特定问题；复杂排障需要多个工具组合，从概览到钻取逐步缩小范围

<details>
<summary>📝 自测题（点击展开）</summary>

1. **CPU 问题分析的工具选型决策树是什么？**

   <details>
   <summary>参考答案</summary>

   Step 1：CPU 利用率高？→ Yes：on-CPU 分析（profile 采样→stackcount 栈→火焰图定位热点）；No：off-CPU 分析（offcputime 看等待原因→runqlat 看调度延迟）。Step 2：有短命进程？→ execsnoop。Step 3：中断影响？→ irq_handler 追踪。Step 4：上下文切换多？→ sched_switch 统计。

   </details>

2. **HFT CPU 排障的推荐工具组合是什么？**

   <details>
   <summary>参考答案</summary>

   快速排查（秒级）：mpstat -P ALL（看核级利用率）+ runqlat（看调度延迟分布）。深度分析（分钟级）：profile:hz:99（on-CPU 热点）+ offcputime（off-CPU 原因）+ execsnoop（短命进程）。精确追踪（秒级短跑）：bpftrace sched_switch + irq_handler。原则：从低开销概览到高开销钻取。

   </details>

3. **如何根据 CPU 利用率判断用 on-CPU 还是 off-CPU 工具？**

   <details>
   <summary>参考答案</summary>

   CPU 利用率 > 80%（计算密集）：on-CPU 分析——用 profile/stackcount 找 CPU 热点，优化算法。CPU 利用率 < 50% 但延迟高：off-CPU 分析——用 offcputime 找等待原因（锁/IO/调度），用 runqlat 看调度延迟。两者都需要：如果 on-CPU 和 off-CPU 都没有明显异常，检查中断和内核调度策略。

   </details>

</details>

---
