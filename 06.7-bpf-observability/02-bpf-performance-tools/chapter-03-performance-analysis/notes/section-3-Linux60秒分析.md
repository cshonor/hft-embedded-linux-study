# 3.3 Linux 60 秒分析（10 个传统工具逐个讲）

> 底本：《BPF之巅》第 3 章性能分析，3.3 节（印刷 p77–84，含 3.3.1–3.3.10）

作者登录一台表现不佳的 Linux 后**最初 60 秒**做的事（Netflix 性能工程团队发表过）。为什么 BPF 书讲非 BPF 工具：这些现成资源可能直接定位问题；即便不能，也暴露根因线索、指引后续 BPF 工具。

```bash
uptime
dmesg | tail
vmstat 1
mpstat -P ALL 1
pidstat 1
iostat -xz 1
free -m
sar -n DEV 1
sar -n TCP,ETCP 1
top
```

## 3.3.1 uptime — 平均负载

`load average: 2.74, 2.54, 2.58`：1/5/15 分钟**指数衰减滑动窗口**。Linux 的负载含**想跑的进程 + 不可中断 I/O 阻塞的进程**（D 状态）。判读：高 15 分钟 + 低 1 分钟 = **你错过了问题现场**（容错环境机器可能已自动下线）。

## 3.3.2 dmesg | tail — 最近 10 条内核日志

找导致性能问题的错误：例中 OOM killer 杀 perl、TCP SYN flooding 丢请求（还指了下一步方向：查 SNMP 计数器）。

## 3.3.3 vmstat 1 — 虚拟内存与系统概况

关键列（首行是开机以来均值，忽略）：

- `r`：正在执行+等待执行的进程数——**比 load average 更好的 CPU 饱和度指标**（不含 I/O）；r > CPU 数即饱和
- `si/so`：换入换出非零 = 内存紧张
- `us/sy/id/wa/st`：用户/内核/空闲/IO 等待/**被虚拟化窃取**——例中 us 主导 → 下一步剖析用户态代码

## 3.3.4 mpstat -P ALL 1 — 每 CPU 分解

发现**单核打满**：例中 CPU0 %usr=100%——单线程瓶颈特征，换更多核无用。高 %iowait → 磁盘工具跟进；高 %sys → syscall 跟踪与 CPU 剖析。

## 3.3.5 pidstat 1 — 每进程 CPU

默认滚动输出（优于 top 的静态屏）。**百分比是对全部 CPU 之和**：500% = 5 个满载核。例中 java 进程 12178/12569 波动 200–500%。注意：某些 pidstat 版本曾把上限错限为 100%，多线程下输出错误（该改动已撤回，留意识别）。

## 3.3.6 iostat -xz 1 — 存储设备 I/O

关键列：

- `r/s w/s rkB/s wkB/s`：负载画像用
- `await`：**应用承受的平均响应时间（队列时间+服务时间）**，超预期 = 饱和或设备问题
- `avgqu-sz`：队列长度，>1 可能饱和（多盘虚拟设备可并行，需甄别）
- `%util`：繁忙时间占比；**>60% 常致性能变差**；但注意它不是容量意义上的使用率——100% 时并行设备可能仍能接更多负载

例：md0 虚拟设备 ~300MB/s 写，背后是两块 nvme。

## 3.3.7 free -m — 内存

看 **available** 列（含可回收的 buffer/cache）是否接近 0；缓存占内存是好事不是浪费。（新版 free 才有 available；-w 可分开显示 buff/cache。）

## 3.3.8 sar -n DEV 1 — 网卡

`rxkB/s txkB/s` 检查是否打满接口带宽上限；`rxpck/s` 包速率画像。

## 3.3.9 sar -n TCP,ETCP 1 — TCP 概况

- `active/s`：本地发起 connect() 数；`passive/s`：远端发起 accept() 数——负载画像利器
- `retrans/s`：重传数——网络或对端问题征兆

## 3.3.10 top — 收尾二次确认

汇总浏览系统与进程摘要，交叉验证前面看到的现象。

## 60 秒清单速查表（判读 → 下一步）

| 工具 | 看什么 | 异常时下一步 |
|---|---|---|
| uptime | 1/5/15min 负载趋势 | 高 15 低 1 = 错过现场 |
| dmesg | OOM/网络错误 | 按 log 指引查计数器 |
| vmstat | r、si/so、us/sy/wa/st | r>核数→runqlat；wa 高→iostat |
| mpstat | 单核 100%、%sys | 单核满→profile；sys 高→syscall 跟踪 |
| pidstat | 哪个进程吃 CPU | 对该进程剖析 |
| iostat | await、avgqu-sz、%util | 高→biolatency/biosnoop |
| free | available | 低→内存工具 |
| sar DEV | 带宽打满？ | → 网络工具 |
| sar TCP | retrans、active/passive | retrans→tcpretrans |
| top | 交叉确认 | — |

## HFT 关联

- 这 10 条命令可直接做成交易机的**开盘前 60 秒体检脚本**：单核打满（mpstat）、调度排队（vmstat r）、磁盘 await 尖刺、TCP 重传，全是延迟杀手。
- vmstat 的 `st`（steal）在云交易机上尤其要盯：宿主机超卖直接吃掉你的延迟预算。
- iostat await 的教训适用于 HFT 所有"平均值"：平均 await 5ms 可能藏着 p99 800ms——第 2 章 biolatency 直方图才是答案。

## 陷阱

- vmstat 首行是开机以来均值，分析要跳过。
- 把 %util=100% 当"容量满了"——并行设备可能还有余量。
- load average 含 D 状态进程：高负载不一定是 CPU 不够，可能全是 IO 等待（看 vmstat r 更准）。

## 自测

<details>
<summary>1. 为什么 vmstat 的 r 列比 load average 更适合查 CPU 饱和度？</summary>

r 只统计正在执行+等待 CPU 的进程，不含不可中断 I/O；load average 把 D 状态也算进去，IO 重时虚高。
</details>

<details>
<summary>2. await 和 %util 各自的含义与局限？</summary>

await=I/O 平均响应时间（队列+服务），是应用真实感受；%util=设备繁忙时间占比，并行设备 100% 时仍可能有剩余容量，不是容量规划意义上的使用率。
</details>

<details>
<summary>3. pidstat 显示 500% 意味着什么？</summary>

该进程占用了 5 个满载 CPU 核心（百分比是全核之和）。
</details>
