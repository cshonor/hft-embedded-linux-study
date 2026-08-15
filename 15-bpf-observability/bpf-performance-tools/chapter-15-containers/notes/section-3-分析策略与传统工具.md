# 3. 分析策略与传统工具（15.1.3 / 15.2 节）

> 底本：《BPF之巅》第 15 章 容器，15.1.3、15.2 节（印刷 p706–710）

## 15.1.3 分析策略（三步）

1. 检查硬件资源瓶颈及第 6/7 章问题——**尤其为运行中的应用创建 CPU 火焰图**
2. 检查是否遇到了 **cgroups 软限制**
3. 浏览运行第 6–14 章的 BPF 工具

> 作者经验：大部分容器问题由**应用程序或硬件**导致，而非容器配置。CPU 火焰图常显示应用级问题、与是否在容器中无关——先查这类问题，同时别忘查容器的资源限制。

## 15.2 传统工具

### 15.2.1 从主机上分析（表 15-1）

| 工具 | 类型 | 描述 |
|------|------|------|
| **systemd-cgtop** | 内核统计 | 针对 cgroups 的 top |
| **kubectl top** | 内核统计 | 针对 Kubernetes 资源的 top |
| **docker stats** | 内核统计 | Docker 容器资源使用 |
| **/sys/fs/cgroups** | 内核统计 | 原始 cgroups 统计 |
| **perf** | 统计和跟踪 | 支持 cgroups 过滤的多功能跟踪器 |

### 15.2.2 在容器内分析（表 15-2）

传统工具也可在容器内跑，但很多指标是**整个主机**的：

| 工具 | 容器内表现 |
|------|-----------|
| top(1)/ps(1) | 只显示容器进程（**pidns 感知**） |
| uptime(1) | **主机**平均负载 |
| mpstat(1)/vmstat(8)/iostat(1)/free(1) | **主机** CPU/磁盘/内存 |

**"容器感知"**：工具在容器内只显示本容器进程和资源。表 15-2 所有工具**没有一个完全容器感知**——容器内性能分析的已知问题（Linux 4.8 时代，随内核与工具更新会改善）。

### 15.2.3 systemd-cgtop

显示资源消耗最大的 cgroups（生产容器主机）：

```
# systemd-cgtop
Control Group                                Tasks  %CPU   Memory    Input/s Output/s
/docker                                       1082  790.1  42.1G
/docker/dcf3a...9d28fc4a1c72bbaff4a24834      200  610.5  24.0G
/docker/370a3...e64ca01198f1e843ade7ce21      170  174.0   3.0G
/system.slice                                 748    5.3   4.1G
...
```

`/docker/dcf3a...` 跑 200 个任务占 610.5% CPU（跨多核）+ 24GB 内存。systemd 也为系统服务（/system.slice）和用户会话（/user.slice）建了 cgroup。

### 15.2.4 kubectl top

```
# kubectl top nodes
NAME                        CPU(cores)  CPU%  MEMORY(bytes)  MEMORY%
bgregg-i-03cb3a7e46298b38e  1781m       10%   2880Mi         8%

# kubectl top pods
NAME                         CPU(cores)  MEMORY(bytes)
kubernetes-b94cb9bff-p7jsp   73m         9Mi
```

依赖 **metrics server**（按 K8s 初始化方式可能已默认装）；其他监控工具也可在 GUI 展示。

### 15.2.5 docker stats

```
# docker stats
CONTAINER    CPU %    MEM USAGE/LIMIT       MEM%   NET I/O     BLOCK I/O    PIDS
353426a09db1 526.81%  4.061GiB/8.5GiB       47.7%  2.818MB/0B  0B/0B        247
```

容器 353426a09db1 占 527% CPU、内存限额 8.5GB 用了 4GB、无网络 I/O、MB 级磁盘 I/O。

### 15.2.6 /sys/fs/cgroups

原始 cgroup 统计虚拟文件（各容器监控产品读它画图）：

```
# cat cpuacct.usage
1615816262506                    ← 总 CPU 时间（纳秒）

# cat cpu.stat
nr_periods 507                   ← 计时周期数
nr_throttled 74                  ← 被 CPU 限制的次数
throttled_time 3816445175        ← 总限制时间（ns）= 3.8 秒

# cat cpuacct.usage_percpu       ← 每 CPU 纳秒数（16 CPU = 16 字段）
```

**nr_throttled / throttled_time 是 CPU 限流（CFS quota 打满）的直接证据**。文档：内核 Documentation/cgroup-v1/cpuacct.txt。

### 15.2.7 perf

perf(1) 在主机上支持 **--cgroup（-G）** 过滤：

```
perf record --cgroup /containers.slice/5aad...   # CPU 剖析
perf stat -e syscalls:sys_enter_read* --cgroup /containers.slice/...  # 按事件计数
```

- 可记录进程上下文的任何事件（含系统调用）；perf stat 同样支持；可同时指定多个 cgroup
- perf 有自己的 BPF 界面（附录 D 例子）

## HFT 关联

- `cpu.stat` 的 **nr_throttled/throttled_time** 两行是 K8s 上策略服务抖动的头号嫌疑：CPU limit 设低了，每 100ms 周期被打断一次，尾延迟直接劣化——先 cat 这两行再谈别的
- 容器内 free/iostat 看到的是宿主机数据——监控面板若从容器内采集会全错，务必从主机/cgroup 接口采

<details>
<summary>自测题</summary>

1. 容器分析三步策略是什么？作者的经验结论？
   <details><summary>答</summary>①查硬件瓶颈+CPU 火焰图；②查 cgroup 软限制；③跑第 6–14 章 BPF 工具。经验：大部分问题是应用或硬件导致，而非容器配置。</details>

2. 哪两个 cpu.stat 字段直接证明 CPU 限流？
   <details><summary>答</summary>nr_throttled（被限次数）与 throttled_time（总限制时间）。</details>

3. "容器感知"指什么？表 15-2 的工具中哪些具备？
   <details><summary>答</summary>容器内运行时只显示本容器进程和资源；top/ps 因 pidns 只显示容器进程（部分感知），uptime/mpstat/vmstat/iostat/free 显示主机数据（不感知）。</details>

4. perf 如何按容器过滤？
   <details><summary>答</summary>--cgroup（-G）选项，record 与 stat 均支持，可同时指定多个 cgroup。</details>
</details>
