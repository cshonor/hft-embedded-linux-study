# 7. BPF 工具：workq 与小任务（14.4.12–14.4.14）

> 底本：《BPF之巅》第 14 章 内核，14.4.12–14.4.14 节（印刷 p694–697）

## 14.4.12 workq

bpftrace 工具（作者 2019-03-14）：跟踪**工作队列请求并按函数统计延迟直方图**：

```
# workq.bt
@us[intel_atomic_commit_work]:
[16K, 32K)   10191            ← 16~32us 档

@us[kcryptd_crypt]:
[4, 8]       4864
[8, 16]      10746            ← 主要 4~32us
[16, 32]     2887
[32, 64]     456
[128, 256]   190
```

kcryptd_crypt()（dm-crypt 加密工作队列）被频繁调用，延迟通常 4~32us。

源代码——**workqueue 跟踪点对**：

```bash
tracepoint:workqueue:workqueue_execute_start
{
    @start[tid] = nsecs;
    @wqfunc[tid] = args->function;      // 工作函数指针
}

tracepoint:workqueue:workqueue_execute_end
/@start[tid]/
{
    @us[ksym(@wqfunc[tid])] = hist(nsecs - @start[tid]);
    delete(@start[tid]); delete(@wqfunc[tid]);
}
```

测量**执行开始到结束**的时间，以函数名为键存直方图。

## 14.4.13 小任务（tasklet）

- 2009 年 Anton Blanchard 提出过 tasklet 跟踪点补丁，**至今不在内核中** → 只能 kprobe
- 小任务函数在 `tasklet_init()` 中注册，例如 net/ipv4/tcp_output.c：

```c
tasklet_init(&tsq->tasklet, tcp_tasklet_func, (unsigned long)tsq);
```

用 BCC **funclatency(8)** 跟踪 tcp_tasklet_func() 延迟：

```
# funclatency -u tcp_tasklet_func
     usecs           : count     distribution
         2 -> 3      : 83
         8 -> 15     : 10
        16 -> 31     : 22
        32 -> 63     : 100
        64 -> 127    : 61
```

可按需用 bpftrace + kprobes 为任何 tasklet 函数写自定义工具。

## 14.4.14 其他工具

跨章复用：runqlat(8)（第6章 运行队列延迟）、syscount(8)（第6章）、hardirq(8)/softirq(8)（第6章）、xcalls(8)（第6章 CPU 交叉调用）、vmscan(8)（第7章）、vfsstat(8)（第8章）、cachestat(8)（第8章）、biostacks(8)（第9章）、skblife(8)（第10章）。

本章级两个特殊工具：

- **inject(8)**（BCC）：用 `bpf_override_return()` **修改内核函数返回错误**，测试错误处理路径（故障注入）
- **criticalstat(8)**（BCC，Joel Fernandes 2018-06-18）：测量内核**原子性临界区**（禁 IRQ/抢占区间），显示持续时间与调用栈；默认显示**禁 IRQ 超过 100us** 的路径——找内核中的延迟源。需要内核编译启用 CONFIG_DEBUG_PREEMPT 和 CONFIG_PREEMPTIRQ_EVENTS

## HFT 关配

- workq 直方图看的是**内核下半部分的执行延迟**：网卡驱动、dm-crypt、延迟写盘都在工作队列里；交易机上 `nvme*`/驱动 workq 函数的尾部延迟直接叠加到 I/O 完成时间
- **criticalstat 是交易系统的隐藏宝石**：禁 IRQ 超 100us 的内核路径 = 抖动源，任何在跑实时行情的机器都值得跑一次
- tcp_tasklet_func（TSQ 小任务）正是第 10 章 TCP 小包发送的下半部分——两章在此交汇

<details>
<summary>自测题</summary>

1. workq 用哪两个跟踪点？如何得到函数名？
   <details><summary>答</summary>workqueue:workqueue_execute_start / _end；start 时存 args->function 指针，end 时 ksym() 翻译。</details>

2. 小任务为什么不能用跟踪点？如何跟踪 tcp_tasklet_func 的延迟？
   <details><summary>答</summary>tasklet 跟踪点补丁（2009）至今未进内核；只能 kprobe，例如 funclatency -u tcp_tasklet_func。</details>

3. criticalstat(8) 测什么？默认阈值？需要哪些内核选项？
   <details><summary>答</summary>禁 IRQ/抢占的原子临界区时长与调用栈，默认 >100us；需 CONFIG_DEBUG_PREEMPT + CONFIG_PREEMPTIRQ_EVENTS。</details>

4. inject(8) 的作用？
   <details><summary>答</summary>用 bpf_override_return() 让内核函数返回错误，测试错误路径（故障注入）。</details>
</details>
