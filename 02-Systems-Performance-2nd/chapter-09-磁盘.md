# Ch 9 磁盘 · Disks

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**高负载时磁盘常成瓶颈** — CPU 空转等 I/O，吞吐可掉几个数量级。Ch 8 文件系统在上层挡 cache；本章到 **块设备 / HDD / SSD / RAID / blk-mq** 真刀真枪。  
> **HFT：** tick 热路径 **不应等磁盘**；但 **审计日志、replay、备份、SMART 健康** 仍会碰块层 — 懂 **await / biolatency / I/O wait 陷阱** 可避免把 CPU 问题误判成「磁盘好了」。

---

## 大白话 · 本章就五件事

> **别只看 IOPS 数字 — 随机/顺序、读写、块大小、队列深度全不一样。**

**① 时间拆开：请求时间 = 等待 + 服务（响应）。**

- **Wait time** = 在 OS/设备队列里等；**Service/Response time** = 设备真正干活。
- 现代盘内部也有队列 — OS 看到的「服务时间」常叫 **disk I/O latency**。

**② 延迟尺度差 4 个数量级 — 平均值是谎言。**

- 闪存 cache 命中 < 100 µs；HDD 顺序 ~1 ms、随机 ~8 ms；排队 + 控制器最差 **> 1 s**。
- 看 **直方图 / 热力图**，别看单一平均 `await`。

**③ IOPS 不平等 + 两个经典陷阱。**

- 5000 IOPS 没上下文 =  meaningless（随机写 4K ≠ 顺序读 1M）。
- **虚拟盘 100% 利用率** 可能只几块物理盘满；**%iowait** 低也不代表磁盘快（CPU 忙掩盖）。

**④ HDD vs SSD vs RAID vs Linux blk-mq。**

- HDD：寻道、旋转、电梯算法、**Sloth Disk**（不报错但秒级慢 I/O）。
- SSD：FTL、**写放大**、TRIM；**blk-mq** + mq-deadline/Kyber 适配百万 IOPS。

**⑤ 工具：iostat、PSI、biolatency、biosnoop、biostacks、fio。**

- **`biolatency -F`** 分开读/写/sync/flush；**翼手龙热力图** 看并发极限下的延迟突变。

下面按原书 9.1–9.9 展开。

---

## 9.1–9.3 核心概念与模型

### I/O 时间指标

```
I/O Request Time（端到端）
    = Wait Time（队列等待）
    + Service / Response Time（设备处理，含盘内队列）
```

| 术语 | 含义 | 谁量 |
|------|------|------|
| **Request time** | 发 I/O → 完成 | 应用 / 块层 |
| **Wait time** | 在 OS 或 HBA 队列中等待 | iostat `w_await` 等 |
| **Service / Response time** | 设备侧耗时 | `r_await`/`w_await`；BPF 直方图 |

**注意：** 磁盘固件内部排队 — OS 测到的 service time **不是**纯机械/闪存物理时间，统称 **disk response time / latency**。

### 时间尺度（量级感）

| 场景 | 典型延迟 |
|------|----------|
| SSD 读（无排队） | 10–100 µs |
| NVMe 读 | 可更低 |
| HDD 顺序读 | ~1 ms |
| HDD 随机读 | ~8 ms+ |
| 队列饱和 + 控制器 | **100 ms – 1 s+** |
| **Sloth Disk**（故障盘） | 个别 I/O **> 1 s**，无明确 SMART 错 |

**HFT：** P99 tick 尖刺若对齐 **block I/O** — 先 `biosnoop` 找 outlier，再 `smartctl`；热路径机器 **不应** 有常态 ms 级块 I/O。

### I/O 特征

| 维度 | 影响 |
|------|------|
| **随机 vs 顺序** | HDD 随机极慢；SSD 仍受 FTL/GC 影响 |
| **读 vs 写** | SSD 写常更慢；sync/flush 更慢 |
| **I/O 大小** | 4K 随机 vs 1M 顺序 — IOPS 与 MB/s 不可互换 |
| **队列深度** | 深度↑ 吞吐↑ 但 **延迟↑** — 低延迟系统控 queue depth |

**IOPS Are Not Equal（Gregg）：**

```
"5000 IOPS" 必须附带：
  - 随机 or 顺序？
  - 读 or 写？
  - 块大小？
  - 队列深度？
  - 是否 O_DIRECT / 是否绕过 cache？
```

→ Ch 8 [fio 与 WSS](./chapter-08-文件系统.md#87-88-实验与调优) · [Ch 12](./chapter-12-基准测试.md)

### 指标陷阱

**虚拟磁盘使用率：**

- RAID / SAN 呈现 **单块 `sdX`** — `iostat` 100% util 可能只是 **部分成员盘满**，其他盘空闲。
- 需 **阵列管理工具**（`MegaCli`、厂商 CLI）看物理盘。

**I/O Wait（%iowait）：**

```
%iowait = CPU 时间中「空闲且至少有一个 I/O 未完成」的比例
```

| 误解 | 真相 |
|------|------|
| iowait 低 = 磁盘快 | CPU 上若有 **其他计算任务**，iowait **被稀释** |
| iowait 高 = 磁盘慢 | 可能 — 但要结合 **await、PSI、biolatency** |

**更可靠：** `/proc/pressure/io`（PSI）、`iostat await`、BPF 延迟直方图。

→ Ch 6/7 [PSI 概念](./chapter-06-中央处理器.md#66-67-观测工具与可视化)

---

## 9.4 硬件与软件架构

### 机械硬盘（HDD）

| 概念 | 说明 |
|------|------|
| **Seek time** | 磁头寻道 |
| **Rotational latency** | 等扇区转到头下 |
| **Short-stroking** | 只用外圈轨道 — 减寻道、减容量 |
| **Elevator seeking** | 电梯算法合并寻道 |
| **SMR** | 叠瓦式 — 顺序写友好，随机写惩罚大 |
| **Sloth Disk** | 慢 I/O 故障态 — 系统「卡但不报错」 |

### 固态硬盘（SSD / NVMe）

| 概念 | 说明 |
|------|------|
| **Erase-write cycle** | 闪存先擦后写 |
| **FTL** | 闪存转换层 — 逻辑块 ↔ 物理页 |
| **Write amplification** | 写 1 逻辑页可能触发多倍物理写 |
| **TRIM/discard** | 告知 SSD 块已弃 — 助 GC |
| **Wear leveling** | 均衡擦写 |

**HFT：** 日志 / 归档用 **独立 NVMe**；数据面网卡与 **日志盘争 PCIe** 要规划。

### RAID 与阵列

| 级别 | 读 | 写 | 备注 |
|------|----|----|------|
| **RAID 0** | 并行 | 并行 | 无冗余 |
| **RAID 1** | 可并行读 | 双写 | 镜像 |
| **RAID 5/6** | 好 | 写惩罚（parity） | 重建期性能差 |
| **RAID 10** | 好 | 较好 | 常用折中 |

**JBOD** = 只是捆绑，无 RAID 逻辑。

### Linux I/O 栈

```
Application → VFS → FS → Page Cache → Bio → Block Layer (blk-mq)
                                              → I/O Scheduler (mq-deadline / none / bfq)
                                              → Driver → Device
```

| 机制 | 说明 |
|------|------|
| **I/O merging** | 相邻 bio 合并 — 减中断 |
| **单队列时代** | noop / deadline / CFQ — HDD 友好 |
| **blk-mq** | **多队列** — 每 CPU 或每 NUMA 队列，适配 NVMe 百万 IOPS |
| **调度器** | NVMe 常 **`none`**；HDD 可用 **mq-deadline** |

```bash
cat /sys/block/nvme0n1/queue/scheduler
# 常见 [none] mq-deadline kyber bfq
```

→ Ch 8 [FS 上层](./chapter-08-文件系统.md) · [09 Rosen 块层](../09-Linux-Kernel-Networking/)（网络栈不同，块层见 LKD）

---

## 9.5 分析方法论

### USE 方法（Disk）

对 **每块磁盘** 及 **控制器**：

| 字母 | 问什么 | 工具 |
|------|--------|------|
| **U** Utilization | 设备忙的时间比 | `iostat %util` |
| **S** Saturation | 队列长度、等待 | `iostat await`、`avgqu-sz`、PSI io |
| **E** Errors | 驱动/HBA/磁盘错 | `dmesg`、`smartctl`、/proc/diskstats |

→ [附录 A](./appendix-A-USE方法Linux.md)

### 工作负载特征

| 问题 | 工具 |
|------|------|
| 哪块盘忙？ | `iostat -xz 1` |
| 哪个进程？ | `pidstat -d`、`biotop` |
| 什么 syscall 路径？ | `biostacks`、`biosnoop` |
| 负载均衡吗？ | 多盘 iostat 对比、RAID CLI |

### 延迟分析（全栈）

```
App 阻塞
  → syscall read/write/fsync 慢？
  → VFS/FS 锁或 journal？（Ch 8 ext4slower）
  → page cache miss → 块 I/O？
  → blk-mq 队列长？
  → 单块 Sloth Disk / RAID 降级？
```

**原则：** 自上而下 — 别在应用还在 page cache 命中时去调磁盘 scheduler。

---

## 9.6 观测工具

### 传统统计

| 工具 | 用法 | 关键字段 |
|------|------|----------|
| **`iostat -sxz 1`** | 每盘扩展统计 | `%util`、`await`、`r_await`、`w_await`、`avgqu-sz`、merge |
| **`sar -d`** | 历史磁盘 | 容量规划、事后分析 |
| **`pidstat -d 1`** | 进程 I/O | **kB_rd/s、kB_wr/s、iodelay** |
| **PSI** | `/proc/pressure/io` | some/full — 等 I/O stall |

```bash
iostat -sxz 1
# await: 平均响应 ms；avgqu-sz: 队列长；%util: 忙时比（SSD 上 util 亦需谨慎解读）
cat /proc/pressure/io
```

### BPF / BCC

| 工具 | 作用 | 技巧 |
|------|------|------|
| **`biolatency`** | I/O 延迟 **直方图** | **`-F`** 分 read/write/sync/flush |
| **`biosnoop`** | 每笔 I/O 起止、详情 | 找 **outlier**、重排序 |
| **`biotop`** | 按进程 I/O 排序 | 谁在读盘 |
| **`biostacks`** | 块 I/O + **发起栈** | 揪后台 journal、kswapd、flush 线程 |

```bash
sudo biolatency-bpfcc -F -m 5      # 分类型，5ms 一个桶
sudo biosnoop-bpfcc
sudo biostacks-bpfcc
```

→ [Ch 15 BPF](./chapter-15-BPF技术.md) · [附录 C](./appendix-C-bpftrace单行命令.md)

### 底层与硬件

| 工具 | 用途 |
|------|------|
| **`blktrace` + `blkparse`** | 块层极细追踪 |
| **`smartctl -a /dev/sdX`** | SMART 健康、重映射扇区 |
| **`MegaCli` / 厂商 CLI** | RAID 状态、物理盘、BBU |

**Sloth Disk 排查：** `biosnoop` 见单 I/O > 1s + SMART 仍「OK」→ 换盘测试。

---

## 9.7–9.9 可视化、实验与调优

### 延迟热力图（Latency Heat Maps）

**问题：** 单一平均 `await` 掩盖 **双峰** 与 **长尾**。

**Gregg「翼手龙 (Pterodactyl)」形：**

- X 轴：时间或 I/O 偏移；Y 轴：延迟；颜色：频次。
- 低延迟「身体」+ 高并发下突然抬起「翅膀」= **总线/控制器饱和**。

| 图类型 | 用途 |
|--------|------|
| **Latency heat map** | 延迟分布随时间变化 |
| **Offset heat map** | 哪些 LBA 范围慢（HDD 外圈/内圈、SSD GC） |

→ Ch 2 [热力图 / FlameScope](./chapter-02-方法论.md#210-统计与可视化)

### 微基准测试

| 工具 | 特点 |
|------|------|
| **fio** | 灵活；P99/P99.99；Pareto 分布 | 
| **ioping** | 类似 ping 的轻量延迟探测 |

```bash
fio --name=rand4k --filename=/dev/nvme1n1 --direct=1 --rw=randread \
    --bs=4k --iodepth=32 --runtime=60 --time_based \
    --percentile_list=99:99.9:99.99
ioping -c 10 /var/log/hft
```

**HFT：** 上线前 **日志 NVMe** 单独 fio baseline；与 Ch 8 一样 **direct=1 或 size >> RAM**。

### 调优

| 层级 | 手段 | 说明 |
|------|------|------|
| **应用** | 少 I/O、异步日志、O_DIRECT | Ch 5/8 |
| **ionice** | idle / best-effort class | 备份降优先级 |
| **cgroups blkio** | 读写带宽 / IOPS 上限 | 混部隔离 |
| **scheduler** | `/sys/block/*/queue/scheduler` | NVMe 常 none |
| **nr_requests** | 队列深度 | 低延迟可减小 |
| **RAID / 硬件** | BBU、write cache 策略 | 掉电一致性 |

```bash
# 备份进程 I/O 设为 idle 类
ionice -c 3 -p $(pgrep backup)
```

**HFT 裸机：**

- tick 路径 **零同步磁盘等待**
- 日志 / replay **独立盘** + `ionice` 备份
- 监控 **PSI io** + `biolatency`，而非仅 `%iowait`

---

## 本章 Checklist

- [ ] 能解释 **request / wait / response** 与 **IOPS 不平等**
- [ ] 知道 **%iowait** 与 **虚拟盘 %util** 的误导性
- [ ] 会用 **`iostat -sxz`** 读 `await`、`avgqu-sz`
- [ ] 跑过 **`biolatency -F`**，区分 read/write/sync/flush
- [ ] 会用 **`biostacks`** 找「谁发起的块 I/O」
- [ ] 日志盘有 **fio/ioping baseline**；关键盘有 **SMART** 监控

---

## HFT 精读捷径（Ch 9 在路线中的位置）

```
Ch 8  文件系统 — page cache 挡在前面
Ch 9  磁盘（本章：块层、HDD/SSD、RAID、biolatency）
  → Ch 10 网络 — HFT 主战场往往在这里而非磁盘
  → Ch 7/8  先排除 cache 再下钻本章
  → Ch 12 fio
```

**HFT 读法：**

| 场景 | 建议 |
|------|------|
| **tick / 发单热路径** | ⚪ 不应有块 I/O 等待 — 用 Ch 5 线程状态验证 |
| **日志 / 审计 / replay** | 🟡 精读 9.1–9.3 + 9.6 `biolatency` |
| **机器健康 / 共置** | 🟡 Sloth Disk、SMART、PSI io |

**本章最小行动集：**

1. **`iostat -sxz 1`** 60 秒 — 记录日志盘 `await` / `%util`。
2. **`sudo biolatency-bpfcc -F 10`** — 看读/写/sync 分布是否双峰。
3. **`cat /proc/pressure/io`** — 是否有 io some/full 压力。
4. **`smartctl -a`** 日志盘 — baseline SMART。

**Gregg 本章金句（HFT 版）：**

> **消除磁盘瓶颈可带来数量级提升** — 但对 HFT 更常见的是 **别让热路径碰磁盘**。  
> **平均 await 会撒谎**；用 **biolatency -F** 和 **热力图** 看 tail 和双峰。

---

## 相关章节

- 上一章：[chapter-08-文件系统.md](./chapter-08-文件系统.md)
- 下一章：[chapter-10-网络.md](./chapter-10-网络.md)
- 内存 / cache：[chapter-07-内存.md](./chapter-07-内存.md)
- 应用 I/O 阻塞：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- 基准测试：[chapter-12-基准测试.md](./chapter-12-基准测试.md)
- BPF：[chapter-15-BPF技术.md](./chapter-15-BPF技术.md)
- LKD 页回写：[05-LKD ch16](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-16-页高速缓存和页回写.md)
- HFT 调优：[11-HFT ch05](../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
