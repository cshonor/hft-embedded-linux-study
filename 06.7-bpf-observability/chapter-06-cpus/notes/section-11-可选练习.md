# 6.5 可选练习

> 底本：《BPF之巅》第 6 章 CPU，6.5 节（印刷 p253–254）。原书 13 题（未特别说明均可用 bpftrace 和 BCC 实现）— 括号内为学习提示。

## 基础操作题

1. 用 `execsnoop` 跟踪 `man ls` 命令产生的新进程。（体会 man 的 troff/pager 进程链）
2. `execsnoop -t` 跟踪生产系统 10 分钟、输出到日志文件。找到了什么进程？（sort | uniq -c 找高频命令）
3. CPU 压力实验：
   ```bash
   taskset -c 0 sh -c 'while :; do :; done' &
   taskset -c 0 sh -c 'while :; do :; done' &
   ```
   用 `uptime`（负载平均）、`mpstat -P ALL`（单核 100%）、`runqlen`（CPU0 队列=1）、`runqlat`（右尾右移）分析 CPU0 情况；**做完记得 kill**。

四工具对照读法（本题的教学内核——同一压力四种口径）：

| 工具 | 口径 | 本实验预期 |
|------|------|-----------|
| uptime | 1/5/15 分钟指数衰减均值 | 缓慢爬升（衰减惯性，反应最慢） |
| mpstat | CPU 利用率 | CPU0 100%，其余核 0% |
| runqlen | 瞬时队列长度采样 | CPU0 队列 1（两线程轮转，一跑一等） |
| runqlat | 入队→上 CPU 的等待时间分布 | 右尾右移（每线程等一个时间片） |

## 剖析实战题（4–8 题，围绕 dd 流水线）

4. 写一个只采样 CPU0 内核调用栈的工具/单行。（提示：`profile:hz:99 /cpu == 0/ { @[kstack] = count(); }`）
5. 用 `profile` 抓内核栈，分析下面命令的 CPU 用量（`df -h` 找本地盘替换 if=）：
   ```bash
   dd if=/dev/nvme0n1p3 bs=8k iflag=direct | dd of=/dev/null bs=1
   ```
6. 给第 5 题生成 **CPU 火焰图**（`profile -af` → flamegraph.pl）。
7. 用 `offcputime` 抓内核栈，分析第 5 题阻塞在哪里（bs=1 的第二个 dd 会大量阻塞在写管道）。
8. 给第 7 题生成 **off-CPU 火焰图**（--bgcolor=blue）。

5–8 题的期望结论（做完自查）：同一条 dd 流水线，**profile 图里占宽的是 copy_user/copy_page 一族**（在 CPU 上搬运数据），**offcputime 图里占宽的是 pipe_write 睡眠**（第二个 dd 每 1 字节读一次管道，把第一个 dd 也堵住）——on-CPU 与 off-CPU 两张图讲的是同一条流水线的两种时间，合起来才是全部墙钟时间。

## 开发题（9–12 题，无现成答案）

9. **procsnoop**：execsnoop 只见 execve(2)；fork/clone 不 exec 的进程池不可见。写 procsnoop 尽量输出所有新进程（跟踪 fork/clone 或 sched:sched_process_fork 跟踪点）。
10. 写 **bpftrace 版 softirqs**，输出软中断名字（向量 ID → 名字查表；计时配 softirq_exit 跟踪点）。
11. 写 **bpftrace 版 cpudist**（sched_switch 双探针计时 + hist 聚合；参考 runqlat 的 bpftrace 实现结构）。
12. 用 cpudist（任意版本）**分别**对主动/被动上下文切换输出直方图（区分 `prev_state == TASK_RUNNING`）。

### 开发题参考骨架（自测后再看）

题 9（procsnoop——跟踪点版最稳）：

```awk
// sched_process_fork 跟踪点天然覆盖 fork/clone/vfork 全部路径
tracepoint:sched:sched_process_fork {
    printf("%s pid=%d child=%d\n", comm, pid, args->child_pid);
}
```

题 11+12 合并（cpudist + 主动/被动分桶——sched_switch 单探针就够，双探针反而多余）：

```awk
tracepoint:sched:sched_switch {
    // pick_next_task 的耗时 = 本次调度延迟，可从 prev/next 算 on-CPU 时长：
    // 这里用「上 CPU 时刻」map 计时
    if (args->prev_state == TASK_RUNNING /* 0 */) {
        @ voluntary[comm] = count();      // 主动让出（睡眠等 IO）
    } else {
        @involuntary[comm] = count();     // 被抢占（时间片到/高优先级唤醒）
    }
}
// on-CPU 时长版：kprobe:finish_task_switch 配 sched_switch 存时间戳，
// 下一次 sched_switch 的 prev==上次 next 时求差——runqlat.bt 的镜像结构
```

注意题 12 的判定依据：`prev_state == TASK_RUNNING`（0）表示 prev 仍在 runnable 被抢——**被动**；非 0（S/D 等）是主动睡眠。这是"主动 vs 被动"唯一可靠的判据，比看进程类型猜靠谱。

## 开放难题（13 题）

13. （未解决的难题）输出线程因 **CPU 黏合度**浪费的等待时间：线程 RUNNABLE、有空闲 CPU，但因缓存热度不迁移（参考 `kernel.sched_migration_cost_ns` sysctl、`task_hot()` 可能被内联无法直接跟踪、`can_migrate_task()`）。

题 13 的破解思路笔记：定义"浪费的等待" = runqlat 中「队列空闲却仍在等」的那部分。近似做法：sched_wakeup 时记 `wakeups[tid]=nsecs`，sched_switch 上 CPU 时求差，同时对每个时刻查全核 idle 比例（runqlen 全局视图）——被观测的不是"为何不迁移"（task_hot 被内联），而是"迁移欠账"的总量。精度天花板在采样密度与 map 键空间，够写一篇小工具了。

## HFT 建议优先级

- **必做**：3（runqlat/runqlen 实验直觉）、5+6（火焰图全流程）、7+8（off-CPU 火焰图全流程）— 这六个覆盖交易机排查 80% 场景
- **选做**：4（单核采样，绑核剖析基础）、11（cpudist 手写一遍 sched_switch 计时就通了）
- 练习 13 的"缓存热度 vs 迁移"权衡正是绑核策略的理论内核 — 值得读调度器源码思考

## 常见陷阱

1. **实验 3 忘记清理 busy-loop 进程** — 占着 CPU0 影响后续所有实验
2. **第 5 题 dd 用了 /dev/nvme0n1p3 却没有 direct 标志** — 没有 iflag=direct 就绕过 page cache，测的是不同路径
3. **练习 12 只统计全部切换** — 不区分 prev_state 就回答不了"主动 vs 被动"的分布差异问题
4. **题 9 用 kprobe:do_fork 跟踪** — 新内核函数名已变（kernel_thread/fork 系统），用 sched_process_fork 跟踪点免受函数改名影响（tracepoint 是稳定 ABI，这也是全书"优先跟踪点"原则的实例）。
