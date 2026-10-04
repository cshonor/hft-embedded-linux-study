## 9.7–9.9 可视化、实验与调优

> 章节导航：[9.6 观测工具](./section-9.6-观测工具.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：延迟热力图与「翼手龙」形态的判读、fio 微基准的参数语义矩阵、磁盘调优的分层手段与 HFT 裸机的最终形态。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 热力图看**时间轴上的分布演变** | 直方图看不到的「周期性翅膀」 |
| 2 | fio 的每个参数都在**改变问题** | iodepth/bs/rw 各测不同东西 |
| 3 | 调优先级：**应用 > 调度 > 硬件** | 免费的先做 |
| 4 | ionice/cgroup 是混部的**民事调解** | 备份不抢生产的队列 |
| 5 | HFT 终态：**热路径零同步盘 I/O** | 剩下的都是日志盘工程 |

---

### 一、延迟热力图（Latency Heat Maps）

**问题**：单一直方图（[9.6 biolatency](./section-9.6-观测工具.md)）是**全时段聚合**——周期性恶化会被平均掉。热力图加上时间轴：

```
延迟
 ▲                    ░░            ░░            ← 周期性「翅膀」：每 5 分钟
 │        ░▒▓█▓▒░         ░▒▓█▓▒░                （备份/log 滚动/GC 周期）
 │  ░▒▓████████████▓▒░▒▓████████████▓▒░          ← 「身体」：常态低延迟
 └────────────────────────────────────▶ 时间
       颜色 = 该 (时间, 延迟) 格子的 I/O 频次
```

| 图类型 | X 轴 | 用途 |
|--------|------|------|
| **Latency heat map** | 时间 | 分布随时间演变——周期性翅膀 |
| **Offset heat map** | I/O 的 LBA 偏移 | 哪些 LBA 范围慢——HDD 外圈/内圈差、SSD 的 GC 热区 |

**Gregg「翼手龙」形**：低延迟「身体」+ 高并发下突然抬起的「翅膀」= **总线/控制器饱和**（吞吐加大时设备还能接但排队延迟跳档）——多设备共享 HBA/PCIe 时典型。工具：HeatMap/FlameScope（[ch2 可视化](../../chapter-02-methodologies/)）、trace-cmd + KernelShark（[ch14](../../chapter-14-ftrace/)）。

**判读三问**：
1. 身体的基线在哪（介质正常水平）？
2. 翅膀的周期是什么（对齐什么事件）？
3. 翅膀的延迟档位（跳一档 = 排队；跳数量级 = 设备层问题）？

### 二、微基准测试（fio）

| 工具 | 特点 |
|------|------|
| **fio** | 灵活引擎（libaio/io_uring/syslet）；P99/P99.99；自定义延迟分布 |
| **ioping** | 类 ping 的轻量延迟探测（快速 sanity check） |

**fio 参数矩阵——每个参数都在改变你测的问题**：

| 参数 | 语义 | 测的是什么 |
|------|------|-----------|
| `--rw=randread/randwrite` | 随机 | 寻址能力（HDD 灾难区/SSD 常态） |
| `--rw=read/write` | 顺序 | 流能力（缓存预读友好） |
| `--bs=4k` vs `1m` | 块大小 | 小块测 IOPS/延迟，大块测带宽 |
| `--iodepth=1` | 单深 | **单笔往返延迟**（低延迟口径） |
| `--iodepth=32` | 深队列 | 排队吞吐上限 |
| `--direct=1` | 绕 page cache | 设备真实能力 |
| `--time_based --runtime=60` | 定时 | 稳态（防写完即止的假象） |
| `--percentile_list=99:99.9:99.99` | 分位 | tail 行为 |

```bash
fio --name=lat4k --filename=/dev/nvme1n1 --direct=1 --rw=randread \
    --bs=4k --iodepth=1 --runtime=60 --time_based \
    --percentile_list=50:99:99.9:99.99
# HFT 日志盘验收：另跑 randwrite QD1（写路径 + GC 行为）
ioping -c 10 /var/log/hft
```

**HFT 验收口径**：日志 NVMe 单独 fio baseline——randread QD1 的 P99.9 + randwrite QD1 的 P99.9（写路径含 FTL/GC 抖动，是日志盘的真实工作负载形状）。与 [ch8](../../chapter-08-file-systems/)、[ch12](../../chapter-12-benchmarking/) 一样：`direct=1` 或 size >> RAM，否则测的是 page cache。

### 三、调优分层

| 层级 | 手段 | 说明 | 收益量级 |
|------|------|------|---------|
| **应用** | 少 I/O、异步日志、批量写、O_DIRECT（避免双缓冲） | [ch5/ch8](../../chapter-08-file-systems/) | 10~1000× |
| **优先级** | `ionice -c3`（idle class） | 备份降级——**queue 有空闲才发** | 消除备份尖刺 |
| **cgroups** | `io.max` 读写 IOPS/带宽上限 | 混部隔离（[ch11](../../chapter-11-cloud-computing/)） | 民事调解 |
| **调度器** | `/sys/block/*/queue/scheduler` | NVMe `none`；HDD `mq-deadline` | 小 |
| **队列** | `nr_requests` 调小、`rq_affinity` | 低延迟口径（[9.4](./section-9.4-硬件与软件架构.md)） | 小-中 |
| **预读** | `read_ahead_kb` | 顺序读放大（随机读调小防污染 cache） | 中 |
| **RAID/硬件** | BBU + write cache、条带对齐 | 掉电一致性 + R5 写惩罚 | 数量级（R5 写） |

```bash
# 备份进程 I/O 设为 idle 类：只有队列空闲时才发
ionice -c 3 -p $(pgrep backup)

# cgroup v2 限容器 IOPS（混部）
echo "8:0 rbps=104857600 wiops=2000" > /sys/fs/cgroup/mixed/io.max
```

**ionice 的机制**：CFQ/BFQ 系调度器按 class 排序发 I/O——idle class 只有队列空闲才调度。**注意**：NVMe `none` 调度器下 ionice 无效（没有软件调度层可插队）——混部隔离要改用 cgroup `io.max`（限额）或物理分盘。

### 四、HFT 裸机的磁盘终态

| 组件 | 形态 |
|------|------|
| tick 热路径 | **零同步磁盘等待**——内存数据结构 + mlock；任何 fsync 都是事故 |
| 日志 | 独立 NVMe + 顺序写 + 异步批量 flush + `none` 调度器 |
| replay/回放 | 独立盘（与日志分盘，避免 GC 互扰） |
| 备份/归档 | `ionice -c3` + 错峰 + io.max 限额 |
| 监控 | PSI io + biolatency 巡检（**不是** %iowait） |
| swap | 关闭或 swapoff；热路径内存 mlock |

**上线前检查**（runbook 素材）：

```
□ fio baseline 归档（randread/randwrite QD1 分位）
□ 调度器确认 none（NVMe）
□ swap 关闭 / swappiness=0
□ 备份任务 ionice + 错峰
□ 日志盘与其他盘物理分离
□ PSI io 巡检项配置
```

### 衔接

- 上一节：[9.6 观测工具](./section-9.6-观测工具.md)
- 关联：[ch2 热力图方法](../../chapter-02-methodologies/)、[ch12 fio 口径](../../chapter-12-benchmarking/notes/section-12.2-基准测试的类型.md)、[ch8 文件系统调优](../../chapter-08-file-systems/)、[ch11 cgroup io](../../chapter-11-cloud-computing/notes/section-11.3-操作系统虚拟化-容器.md)
- 下一章：[ch10 网络](../../chapter-10-network/)

---

### 常见陷阱

1. **fio 默认参数直接跑**——不带 direct=1 测的是 cache，iodepth 没定测的问题不明（[ch12 拷问](../../chapter-12-benchmarking/)第一组问题就是问这个）。
2. **NVMe 上用 ionice 限备份**——none 调度器下没有软件插队层；用 cgroup io.max。
3. **nr_requests 一路调大求吞吐**——低延迟场景反向：队列越深单笔等待越长。
4. **只看直方图不看时间轴**——周期性翅膀（备份/GC/log 滚动）在全时段直方图里隐形，要热力图。

<details>
<summary>自测题（点击展开）</summary>

1. 翼手龙形态的翅膀说明什么？
   <details><summary>答</summary>高并发下总线/控制器饱和——设备还能接 I/O 但排队延迟跳档；身体是常态低延迟，翅膀是饱和期的延迟抬升。</details>
2. fio 的 iodepth=1 与 iodepth=32 各测什么？
   <details><summary>答</summary>QD1 测单笔往返延迟（低延迟口径）；QD32 测排队下的吞吐上限（带宽口径）——HFT 日志盘验收用前者（另加 randwrite）。</details>
3. 为什么 NVMe 上 ionice 无效？
   <details><summary>答</summary>ionice 作用于软件调度器的 class 排序；NVMe 常用 none 调度器（无软件层），混部隔离要用 cgroup io.max 限额。</details>
4. offset heat map 能发现什么？
   <details><summary>答</summary>哪些 LBA 范围慢——HDD 外内圈速差、SSD GC 热区、坏块重映射的局部异常。</details>
5. HFT 机器 swap 为什么要关？
   <details><summary>答</summary>一次 page-in 就是 ms 级 I/O 停顿；热路径内存应 mlock 常驻物理内存——swap 活动即纪律失效信号。</details>

</details>


---

← [本章导读](../README.md)
