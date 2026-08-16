# 6.3 BPF 工具（二）：运行队列 — runqlat / runqlen / runqslower

> 底本：《BPF之巅》第 6 章 CPU，6.3.3–6.3.5 节（印刷 p215–223）。CPU 饱和度分析的黄金三件套。

## 6.3.3 runqlat — 运行队列延迟 🔴

**测量**：线程从**唤醒（进 RUNNABLE）→ 实际上 CPU 运行**的等待时间，直方图输出。需求超过供给（CPU 饱和）时的核心量化工具。

**健康示例**（48-CPU 生产 API 机，CPU 42%）：

```
# runqlat 10 1
     usecs           : count  distribution
         0 -> 1      : 3149   |\
         2 -> 3      : 304613 |@@@@@@@@ ...
         4 -> 7      : 274541 |@@@@@@@ ...
         8 -> 15     : 58576  |@ ...
        16 -> 31     : 15485  |
        ...
     8192 -> 16383   : 24     |     ← 少量离群点
```

大部分 <15µs — 正常。注意即使 42% 使用率、非饱和，仍有零星高延迟尾部。

**病态示例**（36-CPU 编译机，并行度误设为 72 → CPU 超载）：

```
     8192 -> 16383   : 14549  |@@@@@@@@@@@@@@@@@@@@
    16384 -> 32767   : 5589   |@@@@@@@
    32768 -> 65535   : 372    |
    65536 -> 131071  : 910    |
```

三峰分布、主峰在 8–16ms — 每个线程等待显著。交叉验证：`sar -uq 1` 显示 `%idle=0`、`runq-sz=72`（队列长度超过 36 个 CPU）。

原理与开销：

- 跟踪 `sched:sched_wakeup` / `sched_wakeup_new`（记时间戳）+ `sched:sched_switch`（求差）
- 被动上下文切换：切出时 `prev_state == TASK_RUNNING` → 线程回到队列，时间戳要**重记**（bpftrace 版的核心逻辑，见下）
- 繁忙系统唤醒/切换每秒可超 10 万次，每事件 >1µs 处理即有感 → **短期运行**

BCC 选项：`-m`（毫秒单位）、`-P`（每 PID 一张直方图）、`--pidnss`（每 PID namespace）、`-P PID`、`-T`（时间戳，`runqlat -T 1` 每秒出图适合趋势记录）。

bpftrace 版（教科书级，必背）：

```bash
#!/usr/local/bin/bpftrace
#include <linux/sched.h>
BEGIN { printf("Tracing CPU scheduler... Hit Ctrl-c to end.\n"); }

tracepoint:sched:sched_wakeup,
tracepoint:sched:sched_wakeup_new
{
    @qtime[args->pid] = nsecs;
}

tracepoint:sched:sched_switch
{
    if (args->prev_state == TASK_RUNNING)
        @qtime[args->prev_pid] = nsecs;      // 被动切换：重新入队
    $ns = @qtime[args->next_pid];
    if ($ns) {
        @usecs = hist((nsecs - $ns) / 1000);
        delete(@qtime[args->next_pid]);
    }
}

END { clear(@qtime); }
```

## 6.3.4 runqlen — 运行队列长度

**采样**（99Hz）各 CPU 运行队列长度，线性直方图（lhist）输出。开销近乎为零。

- 作者定位：**runqlat 是一级指标**（直接、按比例影响性能），**runqlen 是二级指标**（解释 runqlat 为什么高）。类比：超市排队，你关心的是等待**时间**不是队伍**长度**
- 但 runqlen 用**定时采样**（99Hz）而非事件跟踪 → 7×24 监控优先用 runqlen，发现问题再用 runqlat 量化
- `-C` 按 CPU 分别输出 — 识别调度器**负载均衡问题**。书中例：4 线程绑死 CPU0，`runqlen -C` 显示 cpu=0 队列长 3、其他 CPU 全 0
- `-O` 输出**运行队列占有率**（队列非零时间占比）— 适合做监控/报警的固定指标

bpftrace 版难点：内核头文件没有完整 `cfs_rq`，需自定义**部分结构体**（cfs_rq_partial）只取 `nr_running`；BTF 普及后无需再这样。

```bash
profile:hz:99
{
    $task = (struct task_struct *)curtask;
    $my_q = (struct cfs_rq_partial *)$task->se.cfs_rq;
    $len = $my_q->nr_running;
    $len = $len > 0 ? $len - 1 : 0;   // 减去正在运行的 idle 任务
    @runqlen = lhist($len, 0, 100, 1);
}
```

## 6.3.5 runqslower — 延迟超阈值告警

**打印**等待超过阈值（默认 10000µs = 10ms）的线程名、PID、延迟值 — 把 runqlat 的"长尾"变成逐事件日志，直接回答"**哪个应用**受害了"。

书例（48-CPU、45% 使用率、13 秒内 10 次超 10ms）：

```
Tracing run queue latency higher than 10000 us
COMM             TIME     PID     LAT(us)
python3          17:42:49 4590    16345
pool-25-thread-  17:42:50 4683    50001
ForkJoinPool.co  17:42:53 5898    11935
grpc-default-wo  17:43:01 5794    11637
tomcat-exec-296  17:43:02 6373    12083
```

55% 空闲仍有毫秒级延迟 → 繁忙多线程应用 + 调度器迁移慢导致队列不均。

- 实现：kprobe 挂 `ttwu_do_wakeup()` / `wake_up_new_task()` / `finish_task_switch()`（未来会改成跟踪点，与 runqlat 一致）
- 开销与 runqlat 同级（kprobe 事件跟踪）→ 无输出也有代价，短期用
- 选项：`-p PID`；位置参数改阈值（µs）

## HFT 关联

- **runqlat 是绑核健康度体温计**：dedicated 策略核上 runqlat 应近全在 0–15µs 桶；右尾出现毫秒级 = 有东西抢核（内核线程/中断/邻居）
- 常驻监控用 runqlen -O（或 mpstat r 列），事故时 runqlat 10 秒 + runqslower 抓 PID
- sar -uq 交叉验证 runq-sz 超过 CPU 数 = 硬超载（CPU 数就是队列长度上限的健康线）

## 常见陷阱

1. **忽视 runqlat 直方图尾部** — 平均值正常但毫秒级尖峰足以让 HFT 策略超时；离群点就是事故现场
2. **runqlen 的 -O 占有率与直方图混用** — 监控报警用 -O 单一数字，诊断用直方图看分布
3. **忘掉被动切换重记时间戳** — 自己实现 runqlat 时漏掉 `prev_state == TASK_RUNNING` 分支，会把被抢占线程的历史等待清零，低估延迟
4. **7×24 跑事件跟踪版** — runqlat/runqslower 是 kprobe/tracepoint 跟踪，繁忙系统开销可观；常驻监控换 runqlen（99Hz 采样）

<details>
<summary>📝 自测题（点击展开）</summary>

1. **runqlat 和 runqlen 的本质区别？为什么说 runqlat 是一级指标？**

   <details>
   <summary>参考答案</summary>

   runqlat 测线程从唤醒到上 CPU 的等待时间（事件跟踪，直方图）；runqlen 采样运行队列长度（99Hz 定时采样）。runqlat 直接按比例映射到性能损失（等 10ms 就是延迟 +10ms），队列长度不一定（队伍长但挪得快就没伤害）。runqlen 的价值：开销极低适合常驻监控 + 解释 runqlat 高的原因（负载不均 vs 全面超载）。
   </details>

2. **手写 runqlat 时 sched_switch 里为什么要在 prev_state == TASK_RUNNING 时重记时间戳？**

   <details>
   <summary>参考答案</summary>

   被动（抢占式）上下文切换的线程没有进入 SLEEP，而是回到 RUNNABLE 重新排队。若不重记，它再次被调度时算出的"等待时间"会包含之前已经上过 CPU 的时间戳，产生假数据。重记保证测量的是"本轮排队时长"。
   </details>

3. **CPU 45% 空闲，runqslower 却报 50ms 延迟，可能的原因？**

   <details>
   <summary>参考答案</summary>

   负载不均：多线程应用（书例 ForkJoinPool/grpc 线程池）在调度器完成迁移前把某些 CPU 的队列压满，而其他 CPU 空闲。用 runqlen -C 按核看队列长度验证；检查 CPU 亲和设置是否把线程圈死在少数核上。
   </details>

</details>
