# 6.3 BPF 工具（四）：On/Off-CPU 剖析 — profile / offcputime

> 底本：《BPF之巅》第 6 章 CPU，6.3.8–6.3.9 节（印刷 p227–236）。**本章最重要的两个工具**：两者互补，合起来覆盖线程生命周期全部时间。

## 6.3.8 profile — 定时采样调用栈 🔴

BCC 工具，定时采样所有 CPU 上**正在运行**的代码调用栈并统计出现频率 — CPU 占用分析最有用的 BPF 工具之一（硬中断占用见 hardirqs）。

```bash
# profile
Sampling at 49 Hertz of all threads by user + kernel stacks. Hit Ctrl-c to end.
    承接完整调用栈（内核态→用户态），栈底是进程名(PID)，末尾是该栈出现次数
    get_page_from_freelist
    alloc_pages_nodemask
    ...
    write;[unknown];iperf(29136);  15088   ← 频率最高的栈
```

- 默认 **49Hz** 同时采样用户态 + 内核态调用栈（可 `-F` 调整）
- 输出按出现频率升序；每行末尾数字 = 采样命中次数 ≈ CPU 时间占比

**CPU 火焰图**（图 6-5）：

```bash
profile -af 30 > out.stacks01        # -f 折叠输出，-a 标记内核函数 [k]
./flamegraph.pl --color-java < ../out.stacks01 > out.svg
```

- 折叠格式：整栈一行、函数以 `;` 分隔；`--color-java` 按 `[k]` 标记给内核函数不同颜色
- SVG 支持点击缩放；最宽的塔 = CPU 时间最长路径（书例：vfs_write→…→get_page_from_freelist / free_pages_ok）

**相对 perf(1) 的关键优势**：**频率统计在内核态（BPF map）完成** — perf 要把所有采样送到用户态再统计，消耗 CPU + 可能产生文件系统/磁盘 I/O；profile 在内核里聚合，只输出最终结果，额外开销几乎可忽略。

BCC 选项：`-F freq`、`-U`/`-K`（仅用户/内核栈）、`-a`（标记 [k]）、`-d`（用户/内核栈之间加分隔符）、`-f`（折叠输出）、`-p PID`。

bpftrace 单行：

```bash
bpftrace -e 'profile:hz:49 /pid/ { @samples[ustack, kstack, comm] = count(); }'
```

## 6.3.9 offcputime — 脱离 CPU 时间 + 阻塞栈 🔴

profile 的对立面：统计线程**阻塞/离核**的时间，并输出**导致阻塞的调用栈** — 回答"线程为什么不在 CPU 上"。

```bash
# offcputime 5
    sk_stream_wait_memory;tcp_sendmsg;…;vfs_write;write;[unknown];iperf(14657)  5625
    sk_wait_data;tcp_recvmsg;…;recv;[unknown];iperf(14659)                    1021497
    …;sys_select;libc select;[unknown];offcputime(14667)                      5004039
```

栈底数字 = 该栈累计离核微秒数。书例三栈：iperf 等发送缓冲内存 5ms / 等 socket 数据 1.02s / offcputime 自己等 select 超时 5s。

**off-CPU 火焰图**（图 6-6）：`offcputime -f -K -u 5 > out.txt` → `flamegraph.pl --bgcolor=blue`（蓝底与 CPU 火焰图区分，作者惯用配色）。大部分塔由睡眠等待构成 — 点击缩放看具体应用。

- 跟踪 sched_switch：离核记时间戳+栈，重新上线求差，内核态 map 聚合
- **开销警示**：繁忙系统切换高频，额外消耗**可能超 10%** → 生产环境**短期运行**；可用 `-p PID`、`-K`/`-u`、`-U`（仅用户态线程）收窄范围
- 用户态栈常不完整（libc/libpthread 无帧指针），off-CPU 场景比 profile 更明显 — 解决方案见 13.2.9（libpthread 禁用帧指针消除等）
- 生产用途：长锁等待等问题的栈级定位

bpftrace 版（kprobe 范例）：

```bash
#!/usr/local/bin/bpftrace
#include <linux/sched.h>
BEGIN { printf("Tracing nanosecond time in off-cpu stacks. Ctrl-c to end.\n"); }

kprobe:finish_task_switch
{
    $prev = (struct task_struct *)arg0;     // 刚被切下的线程
    @start[$prev->pid] = nsecs;             // 记录离核时刻
    $last = @start[tid];                    // 刚上线的线程
    if ($last != 0) {
        @[kstack, ustack, comm] = sum(nsecs - $last);
        delete(@start[tid]);
    }
}

END { clear(@start); }
```

## profile + offcputime 组合

| | profile | offcputime |
|---|---------|------------|
| 测量 | 在 CPU 上跑什么 | 不在 CPU 上时等什么 |
| 原理 | 定时采样（49Hz） | sched_switch 事件跟踪 |
| 开销 | 可忽略 | 可能 >10%，短期用 |
| 输出 | 栈 × 出现次数 | 栈 × 累计离核时长 |

线程总时间 = on-CPU（profile）+ off-CPU（offcputime）。只看 profile 会漏掉"等锁/等 I/O"型延迟 — 两者必须成对使用。

## HFT 关联

- 策略 P99 毛刺排查标准流程：`profile -af 30` 出 CPU 火焰图（在核热点）+ `offcputime -f -K -u 5` 出 off-CPU 火焰图（阻塞点，蓝色塔里找 futex/epoll/nanotime）
- 收单路径剖析用 `profile -F 49 -p <策略pid>` 降噪，只看策略自身的栈
- 行情突发时 offcputime 的 futex 塔暴涨 = 内部锁竞争，直接指向需改无锁结构的代码
- `-F 49` 而非 50：同样的防锁定步进考虑（见 6.2.4）

## 常见陷阱

1. **只 profile 不 offcputime** — CPU 利用率低但延迟高的问题全在 off-CPU 侧，profile 永远看不见
2. **忘看 [unknown] 占比** — 用户态栈缺失（无帧指针）会让火焰图大面积 [unknown]，先修编译选项（-fno-omit-frame-pointer）再采
3. **生产长跑 offcputime** — 事件跟踪开销 >10% 量级；先 -p 限定进程，限 5–10 秒窗口
4. **把 offcputime 的栈当"出错现场"** — 栈是线程**进入睡眠时刻**的路径（正常等待也会记录），要按时长排序看大头

<details>
<summary>📝 自测题（点击展开）</summary>

1. **profile 相比 perf record 的核心优势是什么？**

   <details>
   <summary>参考答案</summary>

   频率统计（count 聚合）在内核态的 BPF map 中完成，perf_event 采样触发时只增加计数器，最终只把聚合结果传到用户态。perf(1) 要把每个采样记录送到用户态统计，消耗 CPU 还可能产生文件/磁盘 I/O。采样型工具 + 内核态聚合 = 开销可忽略。
   </details>

2. **offcputime 的输出栈是什么时刻的调用栈？**

   <details>
   <summary>参考答案</summary>

   线程**离开 CPU 进入睡眠那一刻**的调用栈（finish_task_switch 时的 kstack+ustack）。它解释"为什么会睡"——栈顶是阻塞点（如 futex_wait、sk_wait_data），往下是发起阻塞的业务路径。
   </details>

3. **火焰图里 off-CPU 用蓝色背景的目的是？**

   <details>
   <summary>参考答案</summary>

   与 CPU（on-CPU）火焰图在视觉上区分：两者图形相似但语义完全不同（时间宽度在 on-CPU 图=CPU 占用，在 off-CPU 图=阻塞时长）。统一配色体系（栈颜色一致、仅换背景）便于同一套阅读习惯。
   </details>

</details>
