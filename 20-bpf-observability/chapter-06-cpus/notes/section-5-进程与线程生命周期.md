# 5. 进程与线程生命周期

### `execsnoop`

追踪 **新进程 exec** — 系统范围。

```bash
sudo execsnoop-bpfcc
```

| 场景 | 价值 |
|------|------|
| 短命 shell 循环、健康检查脚本 | CPU 被吃掉但 `top` 里一闪而过 |
| 异常 fork 风暴 | 看谁在不断拉起子进程 |

### `exitsnoop`

追踪 **进程退出**，含 **存活时长 (Age)**、退出码/信号。

```bash
sudo exitsnoop-bpfcc
```

**HFT：** 排查 watchdog 反复重启、子进程崩溃循环。


### 常见陷阱

1. **忽视短命进程对延迟的影响** — HFT 环境中 fork/exec 产生的短命进程会抢占 CPU、引发调度抖动；execsnoop 可以发现隐藏的短命进程
2. **混淆线程状态转换的观测点** — 线程的 ready→running 转换用 sched_wakeup tracepoint，running→blocked 用 sched_stat_sleep；用错 tracepoint 会漏事件
3. **忽视上下文切换的 cache 影响** — 每次上下文切换都会 flush TLB、污染 cache；HFT 关键路径上的上下文切换数应最小化（isolcpus+nohz_full）

<details>
<summary>📝 自测题（点击展开）</summary>

1. **进程/线程生命周期的关键事件有哪些？bpftrace 如何追踪？**

   <details>
   <summary>参考答案</summary>

   关键事件：exec（新进程加载）、fork/clone（创建新线程/进程）、exit（退出）、sched_switch（上下文切换）、sched_wakeup（唤醒）。追踪：`tracepoint:sched:sched_process_exec`（新进程）、`tracepoint:sched:sched_switch`（上下文切换）、`tracepoint:sched:sched_process_exit`（退出）。用 `@[comm] = count()` 统计频率。

   </details>

2. **短命进程为什么是 HFT 的隐患？如何发现？**

   <details>
   <summary>参考答案</summary>

   短命进程（存在时间 < 采样间隔）会被 top/mpstat 完全漏掉，但它们：(1) 消耗 CPU 时间片；(2) 触发上下文切换和 cache 污染；(3) 可能竞争锁和内存。发现方法：`bpftrace -e 'tracepoint:sched:sched_process_exec { printf("%s -> %s\n", strftime("%H:%M:%S"), comm) }'` 或 BCC `execsnoop`。

   </details>

3. **上下文切换对 HFT 延迟有什么影响？如何减少？**

   <details>
   <summary>参考答案</summary>

   每次上下文切换：(1) 保存/恢复寄存器（~微秒级）；(2) flush TLB；(3) L1/L2 cache 部分污染；(4) 可能跨核迁移（cache 全 miss）。减少方法：(1) isolcpus 隔离关键核；(2) nohz_full 减少定时器中断；(3) taskset 绑核避免迁移；(4) 关闭 SMT；(5) 关闭不必要的服务和守护进程。

   </details>

</details>

---
