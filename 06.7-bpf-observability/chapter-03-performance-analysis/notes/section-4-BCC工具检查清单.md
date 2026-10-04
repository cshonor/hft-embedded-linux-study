# 3.4 BCC 工具检查清单（11 个工具逐个讲）

> 底本：《BPF之巅》第 3 章性能分析，3.4 节（印刷 p84–89，含 3.4.1–3.4.11）

作者放在 BCC 仓库 docs/tutorial.md 的通用清单——60 秒传统工具之后的第一波 BPF 深探：

```bash
execsnoop   opensnoop   ext4slower  biolatency  biosnoop   cachestat
tcpconnect  tcpaccept   tcpretrans  runqlat     profile
```

## 3.4.1 execsnoop — 新进程跟踪

跟踪每次 `execve(2)` 打印一行。价值：**短命进程消耗 CPU 但周期采样的监控看不到**（Netflix 案例见第 1 章：每秒 30 个批量创建进程）。→ 第 6 章

## 3.4.2 opensnoop — open 跟踪

每次 `open(2)`（及变体）打印路径与结果（ERR 列）。打开的文件透露应用的工作：数据/配置/日志文件；**反复打开不存在的文件**会导致异常或性能受损（例中 snmpd 打开 vendor 文件 ERR=-1 ENOENT）。→ 第 8 章

## 3.4.3 ext4slower — 慢文件系统操作

跟踪 ext4 读/写/打开/同步，**只打印超过阈值（默认 10ms）的操作**。定位或排除"应用在文件系统等慢磁盘"这类问题。其他 FS 有对应工具：btrfsslower / xfsslower / zfsslower。→ 第 8 章

## 3.4.4 biolatency — 磁盘 I/O 延迟直方图

从发请求到完成的延迟直方图（log2 桶）。比 iostat 平均值强：能看出**多峰分布**（例：0–1ms 峰 + 8–15ms 峰=两类 I/O）与**离群点**（512–1023ms）。作者注：log2 桶粒度粗，要精细分布可改线性直方图或用 biosnoop 记录每笔 I/O 再离线统计。→ 第 9 章

## 3.4.5 biosnoop — 每笔磁盘 I/O 日志

逐笔打印 I/O（含延迟），可**搜寻时序模式**（如写之后的读排队）。→ 第 9 章

## 3.4.6 cachestat — 页缓存命中率

每秒打印 HITS/MISSES/HITRATIO（例 86–95%）。低命中率给出调优方向。→ 第 8 章

## 3.4.7 tcpconnect — 主动连接跟踪

每次 `connect()` 打印源/目的地址。找**不寻常的连接**：配置低效或入侵行为。→ 第 10 章

## 3.4.8 tcpaccept — 被动连接跟踪

tcpconnect 的搭档，每次 `accept()` 打印一行。两者合起来完整画像连接建立。→ 第 10 章

## 3.4.9 tcpretrans — TCP 重传跟踪

每次重传打印地址+**当时连接的内核状态**：

- **ESTABLISHED 状态重传** → 外部网络可能有问题
- **SYN_SENT 状态重传** → CPU 饱和征兆或内核丢包

→ 第 10 章

## 3.4.10 runqlat — 调度延迟直方图

线程等待 CPU 的时间直方图。定位超预期的 CPU 等待：CPU 饱和、配置错误或调度问题。例中 16–33ms 桶有 809 次明显异常。→ 第 6 章

## 3.4.11 profile — CPU 采样剖析器

按 49Hz 采样**所有线程的用户+内核栈**，打印消重后的调用栈及次数（例：iperf 的 write 路径 58 次）。理解哪些代码路径吃 CPU。→ 第 6 章

## 两套清单如何衔接（60 秒 → BCC）

| 60 秒清单发现 | BCC 跟进 |
|---|---|
| vmstat r 高 / mpstat 饱和 | runqlat → profile |
| mpstat 单核 100% | profile |
| iostat await 高 | biolatency → biosnoop |
| vmstat/pidstat 疑短命进程 | execsnoop |
| free 内存紧张 | cachestat |
| sar retrans 高 | tcpretrans |
| sar active/s 异常 | tcpconnect/tcpaccept |
| 应用卡在 IO | ext4slower / opensnoop |

## HFT 关联

- 交易机每日延迟体检的 BPF 波次就按这 11 个工具来：runqlat（调度抖动=HFT 头号敌人）、profile（热路径变化检测）、tcpretrans（对端/线路问题）、execsnoop（异常的定时任务/脚本抢占 CPU）。
- ext4slower 的"阈值过滤"设计思想值得自研工具借鉴：**只报超过延迟预算的事件**，风暴时输出量可控。
- profile 的 49Hz 又见第 2 章"非整频率防锁步"纪律。

## 陷阱

- biolatency 的 log2 直方图读法：相邻桶宽度翻倍，比较"次数"要想到桶宽不同。
- runqlat 直方图长尾（几十 ms 桶有计数）就是调度问题信号，别被"多数在 0–8µs"迷惑。
- cachestat 只看页缓存，不是所有缓存（如应用自管缓存）。

## 自测

<details>
<summary>1. 为什么 execsnoop 能发现传统监控发现不了的 CPU 消耗？</summary>

周期采样的监控（top 等）容易错过毫秒级短命进程；execsnoop 逐事件跟踪 execve，一个不漏。
</details>

<details>
<summary>2. tcpretrans 中 SYN_SENT 状态重传和 ESTABLISHED 状态重传各指向什么？</summary>

SYN_SENT：本机 CPU 饱和或内核丢包（连 SYN 都处理不过来）；ESTABLISHED：外部网络/对端问题。
</details>

<details>
<summary>3. runqlat 测量的是什么时间？</summary>

线程从变为可运行到真正上 CPU 运行的等待时间（调度队列延迟），反映 CPU 饱和与调度问题。
</details>
