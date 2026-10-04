## 5.5 观测工具

> 章节导航：[5.4 分析方法论](./section-5.4-性能分析方法论.md) · 上一篇 ← · 下一篇 [5.6 常见陷阱](./section-5.6-常见陷阱Gotchas.md) · [本章导读](../README.md)

**本节讲什么**：应用层观测的工具地图（syscall 追踪 / CPU 剖析 / Off-CPU / 事件追踪 / 自定义探针）、每类的开销量级与生产纪律、多线程程序的观测口径。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | **多线程看 TID 不看 PID** | 进程级平均稀释热路径线程 |
| 2 | strace 开发用，**生产换 perf trace** | ptrace 双陷阱机制（[ch13.11](../../chapter-13-perf/notes/section-13.11-perf-trace-系统调用追踪.md)） |
| 3 | On-CPU 与 **Off-CPU 是互补半圆** | 剖析只回答一半 |
| 4 | `syscount` 做 **syscall 资产盘点** | 热路径不应有意外 syscall |
| 5 | USDT/静态探针是**业务级观测**的正路 | 比 uprobe 稳定便宜 |

---

### 一、工具地图

| 工具 | 类型 | 用途 | 开销 |
|------|------|------|------|
| **`strace`** | syscall 追踪（ptrace） | 开发/debug；**生产慎用** | **2-10× 降速** |
| `perf trace` | syscall 追踪（tracepoint） | 生产级 syscall 时长 | 低 |
| **`perf`** | 采样剖析 | CPU 火焰图、PMC | ~1-5% |
| BCC `profile` | BPF CPU 栈采样 | 全栈（内核+用户）、可过滤 | ~1-5% |
| **`offcputime`** | Off-CPU 栈 | 阻塞/等待分析 | 中 |
| `execsnoop` | exec 追踪 | 意外子进程/脚本调用 | 低 |
| **`syscount`** | syscall 计数排行 | syscall 资产盘点 | 低 |
| `pidstat -t` | 线程级 CPU | TID 级归属 | 低 |
| USDT / 静态探针 | 自定义 tracepoint | 业务阶段 span | 极低 |

> 工具全景：[Ch4 观测工具](../../chapter-04-observability-tools/) · 原理：[ch13 perf](../../chapter-13-perf/)、[ch14 ftrace](../../chapter-14-ftrace/)、[ch15 BPF](../../chapter-15-bpf/)

### 二、syscall 观测：三个工具的梯度

| 需求 | 工具 | 理由 |
|------|------|------|
| 开发调试，看参数与返回值 | `strace -f -T -e trace=network` | 信息最全（参数级别） |
| 生产看 syscall 时长分布 | `perf trace -p PID -s` | tracepoint 单点旁路，开销可接受（[ch13.11 机制对比](../../chapter-13-perf/notes/section-13.11-perf-trace-系统调用追踪.md)） |
| 只数频率不做计时 | `syscount` / bpftrace 单行 | 计数级最便宜 |

**strace 慢的机制**（为什么生产禁用）：ptrace 模型下每个 syscall 触发**两次陷阱**（入口停 + 出口停），每次都切换到 tracer 进程——syscall 从 ns 级变 µs 级。perf trace 用 tracepoint 单点旁路，无第二次陷阱。

**syscount 的独特用途——syscall 资产盘点**：

```bash
sudo syscount-bpfcc -p $(pidof strategy) 30
```

输出热路径进程全部 syscall 频率表——**预期清单**（recvfrom/sendto/epoll_wait/clock_gettime）之外出现任何东西都是要解释的：futex 飙高（锁竞争）、ioctl（意外的驱动交互）、open/read（配置轮询泄漏）、madvise/mmap（分配抖动）。「unexpected syscall 定位法」与 [ch13.11](../../chapter-13-perf/notes/section-13.11-perf-trace-系统调用追踪.md) 的 -s 汇总模式互补。

### 三、CPU 归属：On-CPU 与 Off-CPU 互补

```bash
# On-CPU：CPU 时间花在哪（剖析）
perf record -F 99 -g -p $(pidof strategy) -- sleep 30
# 或 BCC 版（内核+用户栈一次采全）
sudo profile-bpfcc -p $(pidof strategy) 30

# Off-CPU：不在 CPU 上的时间花在哪（阻塞）
sudo offcputime-bpfcc -p $(pidof strategy) 30
```

**两半圆的关系**：

| 视角 | 回答 | 盲区 |
|------|------|------|
| On-CPU 剖析 | CPU 周期烧在哪些栈 | 等锁/等 I/O/等调度的时间**不可见** |
| Off-CPU | 阻塞在哪些栈、多久 | 烧 CPU 的低效代码不可见 |

延迟问题（tick P99）往往在 Off-CPU 半圆——「CPU 才 30% 却延迟尖刺」的第一嫌疑就是阻塞等待；吞吐问题（每秒处理多少）在 On-CPU 半圆。**先用 pidstat -t 判断线程是 CPU-bound 还是阻塞-bound，再选半圆**。

Off-CPU 的机制：采样「离开 CPU 时的栈 + 等待时长」（sched_switch/sched_wakeup tracepoint 配对）——比剖析新，判读也多一个坑：**等待不等于浪费**（等 tick 是正常等待；等锁才是问题）——要按等待源分类看。

### 四、事件追踪层

| 工具 | 用途 |
|------|------|
| `execsnoop` | 谁 exec 了什么——意外子进程（shell 调用、cron 泄漏） |
| USDT 探针 | 应用编译期埋的稳定探针（`-DUSDT` / SystemTap SDT） |
| uprobe（定制） | 没埋 USDT 时的补位——生产慎挂热函数 |

**USDT vs uprobe 的取舍**（[ch15.2](../../chapter-15-bpf/notes/section-15.2-bpftrace.md)）：

| | USDT | uprobe |
|--|------|--------|
| 稳定性 | ABI 级（升级不破） | 函数名绑定（改代码即失效） |
| 开销 | 探针点设计过（低） | 任意函数任意热度 |
| 前提 | 源码可控（自己埋） | 二进制有符号 |

**业务级 span 的正确路**：在关键阶段（decode/strategy/risk/send）埋 USDT，BPF 一次采样端到端分解——这比在应用里写日志（分配/格式化开销）干净得多，与 [14-HFT ch09 的延迟测量](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md) 直接衔接。

### 五、多线程观测口径

```bash
pidstat -t -p $(pidof strategy) 1     # -t：线程级（TID）
```

| 陷阱 | 正确做法 |
|------|---------|
| 进程级平均掩盖热路径 | `-t` 看 TID；热线程单列判读 |
| 不知道哪个线程是热路径 | 启动时命名线程（pthread_setname_np） |
| 线程迁移干扰判读 | 结合 [ch6 绑核](../../chapter-06-cpus/) 看 per-CPU |

### 六、生产纪律

| 工具 | 生产策略 |
|------|---------|
| strace | ❌（只进开发环境） |
| perf record/profile | ✅ 短窗口限 PID |
| offcputime/syscount/execsnoop | ✅ 短窗口（30s 级） |
| uprobe 热函数 | ⚠️ staging 验证开销后短窗口 |
| USDT | ✅ 常驻可接受（探针开销设计过） |

### 衔接

- 上一节：[5.4 分析方法论](./section-5.4-性能分析方法论.md)
- 下一节：[5.6 常见陷阱](./section-5.6-常见陷阱Gotchas.md)（符号/栈/inline）
- 关联：[Ch4 观测工具](../../chapter-04-observability-tools/)、[ch13 perf](../../chapter-13-perf/)、[ch15 BPF](../../chapter-15-bpf/)、[附录 C](../../appendix-C-bpftrace单行命令.md)、[06.7-BPF](../../../06.7-bpf-observability/)

---

### 常见陷阱

1. **pidstat 不指定 TID**——进程级平均把热路径线程和 housekeeping 线程搅在一起。
2. **perf record 不加 -g**——只有当前函数没有调用栈，无法做火焰图。
3. **uprobe 生产直接挂热函数**——每次调用都有开销，µs 级路径显著增延迟；用 USDT。
4. **延迟问题只跑 On-CPU 剖析**——阻塞时间在剖析里不可见；pidstat -t 判断后选半圆。
5. **offcputime 输出全当问题**——等 tick 是正常等待，按等待源分类判读。

<details>
<summary>自测题（点击展开）</summary>

1. pidstat 查看 HFT 多线程程序要注意什么？
   <details><summary>答</summary>用 -t 看 TID——进程级数据会把热路径线程和 housekeeping 线程平均掉；配合线程命名。</details>
2. strace 和 perf trace 的机制差异？
   <details><summary>答</summary>strace 是 ptrace 双陷阱（进出各停一次，syscall ns→µs）；perf trace 用 tracepoint 单点旁路只记事件——生产用后者。</details>
3. On-CPU 和 Off-CPU 各自的盲区？
   <details><summary>答</summary>On-CPU 看不见阻塞（锁/IO/调度等待）；Off-CPU 看不见低效计算——延迟问题先想 Off-CPU，吞吐问题先想 On-CPU。</details>
4. syscount 在 HFT 的独特用途？
   <details><summary>答</summary>syscall 资产盘点：热路径进程的 syscall 应限于预期清单（recvfrom/epoll/clock...），任何意外项（futex 飙高/open 轮询）都是线索。</details>
5. USDT 为什么比裸 uprobe 适合业务观测？
   <details><summary>答</summary>ABI 稳定（代码升级不破）、探针点开销经过设计、语义明确（业务阶段名）——uprobe 绑函数名且任意热度。</details>

</details>


---

← [本章导读](../README.md)
