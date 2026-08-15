# 8. Off-CPU 时间 — `offcputime` 🔴

与 `profile` **完美互补**：

| | `profile` | `offcputime` |
|---|-----------|--------------|
| **采样时机** | 线程 **在 CPU 上** | 线程 **被切换下 CPU**（阻塞） |
| **回答** | 在算什么 | **在等什么**（I/O、锁、futex…） |
| **输出** | 栈频率 → 火焰图 | 栈 + **等待时间** 汇总 |

```bash
sudo offcputime-bpfcc -p $(pidof myapp) 30
```

**典型发现：** 热路径在等 `futex`、等 `epoll`、等磁盘 — 引导到 [Ch 7 内存](../../chapter-07-memory/) / [Ch 9 磁盘](../../chapter-09-disk-io/) / 应用锁分析。

**HFT：** P99 升高但 `profile` 无热点 → **优先 offcputime**（是否在等锁或内核 I/O）。


### 常见陷阱

1. **把 off-CPU 时间等同于空闲** — off-CPU 包括等待 IO、等待锁、等待调度、睡眠等多种原因；不是所有 off-CPU 都是问题，但长 off-CPU 延迟需要分析
2. **offcputime 运行在所有进程上** — 全系统 offcputime 会产生海量数据（每个上下文切换都记录）；应按 PID 或 comm 过滤目标进程
3. **忽视 offcputime 的开销** — offcputime 在每次上下文切换时触发 BPF 程序，高频切换环境下开销不小；HFT 环境应短跑并按进程过滤

<details>
<summary>📝 自测题（点击展开）</summary>

1. **offcputime 工具测量什么？它和 runqlat 有什么区别？**

   <details>
   <summary>参考答案</summary>

   offcputime：测量线程不在 CPU 上执行的总时间（从离开 CPU 到重新被调度的间隔），并记录离开时的调用栈和原因。runqlat：只测量调度延迟（从 ready 到 running 的等待时间）。区别：offcputime 包含所有 off-CPU 原因（IO 等待、锁等待、睡眠、调度排队），runqlat 只看调度排队这一种。

   </details>

2. **off-CPU 分析对 HFT 延迟排查有什么价值？**

   <details>
   <summary>参考答案</summary>

   HFT 策略线程如果延迟高但 CPU 利用率低，说明时间花在了 off-CPU 等待上。offcputime 能显示：(1) 等待了多久（直方图/逐事件）；(2) 在哪个函数调用栈上等待的（定位阻塞点）；(3) 等待原因（sched_switch 的原因字段）。直接回答「线程被谁阻塞、阻塞了多久」。

   </details>

3. **offcputime 在生产环境运行需要注意什么？**

   <details>
   <summary>参考答案</summary>

   (1) 按进程过滤：`-p $(pidof myapp)` 避免全系统追踪产生海量数据；(2) 短跑：offcputime 在每次上下文切换触发 BPF 程序，长时间运行开销累积；(3) 设置超时：`--duration N` 只显示超过 N 秒的 off-CPU 时间，过滤短等待；(4) 理解输出：栈 + 时间 + 频率，长的 off-CPU 时间是排查重点。

   </details>

</details>

---
