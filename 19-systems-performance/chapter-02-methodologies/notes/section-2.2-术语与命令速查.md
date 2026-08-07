## 2.2 术语 → 常用命令（HFT 速查）

> 术语说完要能 **落到命令** — 下面按指标列生产/压测第一反应；热路径磁盘常可跳过。

← [2.1 术语对齐](./section-2.1-HFT术语与团队对齐.md) · → [2.3.1 时间尺度](./section-2.3.1-时间尺度与排查走查.md)

| 术语 | 看什么 | 常用命令 | 备注 |
|------|--------|----------|------|
| **IOPS** | 磁盘每秒读写次数 | `iostat -dx 1` | **r/s、w/s**；HFT 热路径少碰盘，多用于 **日志/NVMe 冷路径** |
| | 磁盘极限 IOPS | `fio`（lab） | 压测签收容量，非实盘观测 |
| **Throughput** | 网卡带宽 / pps | `sar -n DEV 1` | **rxkB/s、txkB/s**；行情/发单网卡 |
| | 网卡 pps 交叉验证 | `tcpdump -i eth0 -c 1000 -tttt` | counter 异常时与解析层对照 → [2.5](./section-2.5-性能分析方法论.md#第一步每秒多少-tick埋点为主tcpdump-交叉) |
| | 磁盘吞吐 | `dd` / `fio` | 冷路径或回放盘；`dd` 粗测 |
| | 应用层 | 自建 counter | ticks/s、orders/s — **最贴近业务** |
| **Latency / 响应时间** | 系统调用耗时 | `perf trace -s` | 跟踪 syscall 时间线 |
| | 函数级延迟 | **bpftrace** / eBPF | 热路径、单笔 tail（→ [Ch1.7](../chapter-01-intro/notes/section-1.7-观测工具四层递进.md)） |
| | 端到端 P99/Max | 应用 histogram + 硬件打点 | 比纯 `perf` 更贴 tick→trade |
| **Utilization** | CPU 各核占比 | `mpstat -P ALL 1` | **%usr / %sys / %soft / %idle** |
| | 网卡带宽占用 | `ethtool -S eth0` | 结合 `sar -n DEV` 看是否打满链路 |
| | 进程级 | `pidstat 1` | 哪个策略 PID 吃 CPU |
| **Saturation** | CPU 运行队列 | `vmstat 1` | **r 列** — 可运行线程排队长度 |
| | TCP 连接积压 | `ss -s` | 连接数、重传相关摘要 |
| | 网卡 drop / 背压 | `ethtool -S` | `rx_missed_errors`、drop |
| | 锁 / softirq | `perf stat`、bpftrace | 饱和度在 **等锁/等中断** 时 |
| **Bottleneck** | 热点函数（第一眼） | `perf top -p <pid>` | 生产 **短窗口**、低频 |
| | 固化分析 | `perf record -g` → **火焰图** | 找「宽平台」函数，定嫌疑热路径 |
| **Errors** | 网卡/内核错误 | `ethtool -S`、`dmesg` | USE 里的 **E** |

### 固定套路

```
1. mpstat / vmstat / ethtool  →  Utilization + Saturation（resource 扫盲）
2. sar -n DEV                 →  网卡 Throughput
3. perf top                   →  Bottleneck 嫌疑函数
4. perf record + 火焰图       →  固化 workload 热点
5. perf trace / bpftrace      →  Latency tail 拆 syscall/函数
```

### HFT 注意

- 热路径：**mpstat、ethtool、perf** 优先；`iostat`/`dd` 除非怀疑日志/回放盘
- 实盘 `perf`：**采样别太重** — 与 [Ch1 观测 vs 实验](../chapter-01-intro/notes/section-1.8-实验与微观宏观基准.md) 一致
- P99/Max：**应用内分位数** + trace 抓 spike，不单靠 `perf top` 平均值

→ 工具详解：[Ch 4](../../chapter-04-observability-tools/) · [Ch 13 perf](../../chapter-13-perf/) · [Ch 15 BPF](../../chapter-15-bpf/)


### 常见陷阱

1. 命令记不住就用复杂工具——先掌握 vmstat/mpstat/ss/iperf3 五个基础命令，再上 BPF
2. 命令参数不记——vmstat 的 r 列、si/so 列含义不同，混淆会导致误判
3. 只看一条命令的输出——vmstat + mpstat + iostat 三条合在一起看才能定位是 CPU/内存/IO 哪段

<details>
<summary>自测题（点击展开）</summary>

1. vmstat 输出中 r 列和 b 列分别表示什么？
   <details><summary>答</summary>r = 运行队列长度（等 CPU 的线程数），b = 不可中断睡眠（通常等 I/O）的线程数</details>
2. HFT 排障第一反应应该跑哪几条命令？
   <details><summary>答</summary>vmstat 1（全局）、mpstat -P ALL 1（每核）、ss -tiepm（网络）、iostat -x 1（IO）</details>
3. 为什么 uptime 的 load average 不能代替 per-CPU 分析？
   <details><summary>答</summary>load average 是指数移动平均且混合了 D 态线程，8 核 load=8 不等于 100%，要看 mpstat 每核</details>

</details>


---

← [2.1](./section-2.1-HFT术语与团队对齐.md) · [2.3.1 时间尺度](./section-2.3.1-时间尺度与排查走查.md) · [本章导读](../README.md)
