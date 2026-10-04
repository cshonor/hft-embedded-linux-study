## 9.4 硬件与软件架构

> 章节导航：[9.1–9.3 核心概念与模型](./section-9.1-9.3-核心概念与模型.md) · 上一篇 ← · 下一篇 [9.5 分析方法论](./section-9.5-分析方法论.md) · [本章导读](../README.md)

**本节讲什么**：HDD/SSD 的内部机制（寻道/FTL/GC/写放大）、RAID 级别的性能特征、Linux I/O 栈从 syscall 到设备的全链路与 blk-mq 多队列架构。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | HDD 延迟 = 寻道 + 转速 | 8ms 随机的物理根源 |
| 2 | SSD 的 FTL 是**翻译 + GC 引擎** | 写放大是延迟抖动的来源 |
| 3 | **blk-mq 为多核/多队列而生** | 单队列锁竞争是老栈的瓶颈 |
| 4 | NVMe 调度器常选 **none** | 设备自己会调度，软件层别添乱 |
| 5 | RAID 写惩罚可量化 | R5 = 4 I/O 写 1 条带 |

---

### 一、机械硬盘（HDD）

| 概念 | 说明 | 量级 |
|------|------|------|
| **Seek time** | 磁头移动到目标磁道 | 4–10 ms（随机 I/O 的主要成本） |
| **Rotational latency** | 等扇区转到头下 | 平均半圈：7200rpm ≈ 4.2 ms |
| **Short-stroking** | 只用外圈轨道——减寻道行程、减容量 | 换容量换延迟 |
| **Elevator seeking** | 电梯算法合并寻道路径 | I/O 调度器的 HDD 逻辑 |
| **SMR** | 叠瓦式——磁道重叠，顺序写友好 | 随机写触发整带重写，惩罚巨大 |
| **Sloth Disk** | 慢 I/O 故障态——系统「卡但不报错」 | 见 [9.6](./section-9.6-观测工具.md) |

随机读 8ms 的分解：寻道 ~4ms + 平均转速等待 ~4ms——**物理几何决定**，调软件无用。

### 二、固态硬盘（SSD / NVMe）

| 概念 | 说明 |
|------|------|
| **Erase-write cycle** | 闪存写前必须先擦（页写块擦） |
| **FTL** | 闪存转换层——逻辑块 ↔ 物理页映射，含垃圾回收 |
| **Write amplification** | 写 1 逻辑页可能触发多倍物理写（GC 搬迁 + 元数据） |
| **TRIM/discard** | 告知 SSD 块已弃——GC 不用搬死数据 |
| **Wear leveling** | 均衡擦写（块有擦写寿命） |

**⭐ SSD 延迟抖动的机制**：FTL 的 GC 在后台搬迁有效数据腾出空闲块。空闲块充足时 GC 不打扰前台 I/O；**空闲块不足时（盘写满/OP 耗尽）GC 变成前台同步操作**——随机写延迟从 100µs 暴涨到 ms 级。三个工程推论：

1. **别把 NVMe 写满**——预留 20%+ 让 GC 有余地（企业盘的 over-provisioning 就是预留的空闲块）。
2. **写入顺序化**——顺序写让 GC 成批搬运，随机写让它碎片化。
3. **写延迟比读延迟方差大**——P99 写延迟差 10× 的盘往往不是坏了，是 GC 被触发了（biolatency -F 分开看读写）。

**HFT**：日志/归档用**独立 NVMe**；数据面网卡与日志盘争 PCIe lane 要规划（高 pps 时网卡 DMA 与盘提交互相挤压——查 `lspci` 的 lane 分配与带宽共享拓扑）。

### 三、RAID 与阵列

| 级别 | 读 | 写 | 备注 |
|------|----|----|------|
| **RAID 0** | 并行 | 并行 | 无冗余 |
| **RAID 1** | 可并行读 | 双写 | 镜像 |
| **RAID 5/6** | 好 | **写惩罚（parity）** | 小写 = RMW：R5 写 1 块 = 2 读 + 2 写（4 I/O） |
| **RAID 10** | 好 | 较好 | 常用折中 |

**R5 写惩罚的机制**：条带化后一次小块写需要「读旧数据 + 读旧校验 → 算新校验 → 写数据 + 写校验」——4KB 的逻辑写变成 4 次 4KB 物理 I/O。全条带写（write-back cache 凑齐条带）无惩罚——所以**阵列卡 BBU + write cache** 对 R5 写性能是数量级差异的开关。

**重建期**：坏盘换新后的全量重建会持续数小时到数天，期间**所有成员盘满负荷读**——生产性能断崖，且二次故障风险最高（要监控重建进度并预案）。

**JBOD** = 只是捆绑，无 RAID 逻辑。

### 四、Linux I/O 栈

```
Application → VFS → FS（ext4/xfs）→ Page Cache
                                      ↓ writeback / readahead
              Bio → Block Layer (blk-mq)
                      → I/O Scheduler (none / mq-deadline / bfq)
                      → Driver → Device
```

| 层 | 职责 | 性能相关 |
|----|------|---------|
| VFS/FS | 路径解析、inode、journal | [ch8](../../chapter-08-file-systems/) |
| **Page Cache** | 读写缓冲 | 命中即免 I/O（[ch8 WSS](../../chapter-08-file-systems/)） |
| **blk-mq** | 多队列块层 | 多核扩展的核心 |
| 调度器 | 排序/合并/限额 | NVMe 常关 |
| 驱动 | 提交到设备 | nvme/ahci/megaraid |

**⭐ blk-mq（multi-queue block layer）为什么存在**：

```
旧单队列（~2014 前）：                     blk-mq（v4.0+）:
  所有 CPU ──┐                              每 CPU 一个 software queue
             ├→ 全局请求队列（一把锁）        → （可选合并）→ hardware queue(s)
  所有 CPU ──┘   ↑ 百万 IOPS 时锁竞争        NVMe: 每盘每 namespace 可多 hctx
     与远端 cache line 弹跳                  → 无全局锁
```

NVMe 百万 IOPS 时代，旧栈的**单队列锁**成为瓶颈——blk-mq 让提交路径每 CPU 独立（与 [ch13 的 per-PMU](../../chapter-13-perf/)、[slab 的 per-CPU](../../../06-linux-mm/chapter-08-slab-allocator/) 是同一设计哲学：**共享变每 CPU**）。

**调度器选择**：

```bash
cat /sys/block/nvme0n1/queue/scheduler
# 常见 [none] mq-deadline kyber bfq
```

| 调度器 | 适用 | 逻辑 |
|--------|------|------|
| **none** | NVMe/快 SSD | 直通——设备内部队列已够智能，软件层合并/排序反而加延迟 |
| **mq-deadline** | HDD / 混合 | 读写分队列 + 最后期限防饿死（读优先——读阻塞往往有进程在等） |
| kyber | 快速设备 | 基于延迟目标的自适应 |
| bfq | 桌面/交互 | 按进程公平带宽（吞吐代价大） |

**HFT 判据**：NVMe 日志盘用 `none`（延迟最小）；任何软件层调度对 µs 级延迟都是净增项。

### 五、关键队列参数

```bash
ls /sys/block/nvme0n1/queue/
# nr_requests        软件队列深度（排队上限）
# read_ahead_kb      预读窗口（顺序读放大）
# rotational         0=SSD（影响调度器默认行为）
# rq_affinity        完成 interrupt 的 CPU 亲和（低延迟关注）
# io_poll / iopoll   轮询模式（irq → poll，降延迟提 CPU 占用）
```

`nr_requests` 调小的效果：队列短 → 单笔等待小、吞吐降——**低延迟与吞吐的旋钮**（与 [9.1 的队列深度权衡](./section-9.1-9.3-核心概念与模型.md) 同一根曲线）。`io_poll` 是 io_uring/高性能路径的方向：中断换轮询（与 DPDK 的 poll mode 同哲学）。

### 衔接

- 上一节：[9.1–9.3 核心概念与模型](./section-9.1-9.3-核心概念与模型.md)
- 下一节：[9.5 分析方法论](./section-9.5-分析方法论.md)（USE + 延迟分解树）
- 关联：[ch8 文件系统](../../chapter-08-file-systems/)（page cache 与 journal）、[ch13 per-CPU 设计哲学](../../chapter-13-perf/)、[06-linux-mm 伙伴系统](../../../06-linux-mm/chapter-06-physical-page-allocation/)（page cache 的物理供给）

---

### 常见陷阱

1. **NVMe 默认留着 mq-deadline**——多数发行版已默认 none，但老系统升级来的可能没有；软件调度对 NVMe 是纯开销。
2. **SSD 写满后延迟暴涨当盘坏**——GC 前台化是机制不是故障；预留 OP 空间 + 顺序化写入。
3. **R5 阵列不配 BBU/write cache**——小写惩罚直接吞掉 75% 写性能。
4. **重建期的性能悬崖不预案**——重建数小时满负荷，生产盘要预案降级或错峰。

<details>
<summary>自测题（点击展开）</summary>

1. HDD 随机读 8ms 的构成？
   <details><summary>答</summary>寻道 ~4ms + 平均转速等待 ~4ms（7200rpm 半圈）——几何决定，软件调优无效。</details>
2. SSD 空闲块不足时为什么写延迟暴涨？
   <details><summary>答</summary>FTL 的 GC 从后台搬迁变前台同步操作——写前必须先腾块；预留 OP 空间/顺序化写入是工程对策。</details>
3. blk-mq 解决什么问题？
   <details><summary>答</summary>旧单队列的全局锁在多核+百万 IOPS 下成瓶颈；blk-mq 每 CPU software queue + 多 hardware queue，消除提交路径的全局锁。</details>
4. NVMe 调度器为什么选 none？
   <details><summary>答</summary>设备内部队列（64K+ 深度）已做调度与合并；软件层再排序合并只增加延迟——快设备上「不调度」是最优调度。</details>
5. R5 一次 4KB 小写物理上是几次 I/O？
   <details><summary>答</summary>4 次：读旧数据 + 读旧校验 + 写新数据 + 写新校验（RMW）；全条带写或 write-back 凑条带可免惩罚。</details>

</details>


---

← [本章导读](../README.md)
