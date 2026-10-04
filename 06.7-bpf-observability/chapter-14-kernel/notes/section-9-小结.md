# 9. 小结（14.8）

> 底本：《BPF之巅》第 14 章 内核，14.8 节（印刷 p700）

## 原书小结

本章着重于**内核分析**，作为前面面向资源章节之外的补充：总结了包括 **Ftrace** 在内的传统工具，然后使用 BPF 以及**内核内存分配、唤醒和工作队列请求**，更详细地研究了 **off-CPU 分析**。

## 本章工具全景

| 主题 | 工具 | 探针 | 开销 |
|------|------|------|------|
| 平均负载 | loads | kaddr(avenrun) | 无 |
| off-CPU | offcputime（--state 2） | sched 跟踪点 | 高 |
| 唤醒 | wakeuptime | kprobe schedule/try_to_wake_up | **高** |
| off+唤醒合并 | offwaketime | 同上 | **高** |
| 内核互斥锁 | mlock / mheld | kprobe mutex_* | **高** |
| 自旋锁 | funccount / stackcount / CPU 剖析 | kprobe（无 kretprobe） | 中 |
| 内核内存（对象） | kmem | kmem 跟踪点 | 高 |
| 内核内存（页） | kpages | mm_page_alloc | 高 |
| 泄漏 | memleak | 内核分配器 | 高（调试用） |
| slab 速率 | slabratetop | kprobe kmem_cache_alloc | 中 |
| NUMA 迁移 | numamove | kprobe migrate_misplaced_page | 低 |
| 工作队列 | workq | workqueue 跟踪点 | 低 |
| 小任务 | funclatency 等 | kprobe（无跟踪点） | 中 |
| 临界区 | criticalstat | preemptirq 跟踪点 | 中 |
| 故障注入 | inject | bpf_override_return | 按需 |

## 方法论回顾

1. 九步策略：已知量工作负载 → 找跟踪点 → CPU 剖析 → funccount → stackcount → funcgraph → 参数 → 延迟 → 自定义工具
2. off-CPU 分析三部曲：offcputime（在哪阻塞）→ wakeuptime（谁唤醒）→ offwaketime（合并成一条链 + 火焰图）
3. 内核优先用跟踪点（稳定），kprobe 有内联/黑名单/维护三大坑

## HFT 关联

- 内核章对交易系统的价值集中在**抖动溯源**：offwaketime 串因果链、criticalstat 抓禁 IRQ 临界区、numamove 防 NUMA 均衡偷 CPU、workq 看驱动下半部分延迟
- 与第 6/7/10 章工具互为表里：本章只收"以内核为对象"的工具，资源类问题先回对应章节

<details>
<summary>自测题</summary>

1. 本章 off-CPU 分析三部曲的递进关系？
   <details><summary>答</summary>offcputime 显示阻塞侧栈 → wakeuptime 补上唤醒侧栈（阻塞栈"没有定论"时的另一半）→ offwaketime 把两栈合并为一条因果链（可火焰图）。</details>

2. 哪些内核锁行为只能靠 CPU 剖析研究？
   <details><summary>答</summary>自旋锁（spin_lock* 的 kretprobe 会死锁被禁）与互斥锁的乐观自旋（midpath）——都以消耗 CPU 的形式出现在剖析/火焰图中。</details>
</details>
