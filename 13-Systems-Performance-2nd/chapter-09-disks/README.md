# Ch 9 磁盘 · Disks

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**高负载时磁盘常成瓶颈** — CPU 空转等 I/O，吞吐可掉几个数量级。Ch 8 文件系统在上层挡 cache；本章到 **块设备 / HDD / SSD / RAID / blk-mq** 真刀真枪。  
> **HFT：** tick 热路径 **不应等磁盘**；但 **审计日志、replay、备份、SMART 健康** 仍会碰块层 — 懂 **await / biolatency / I/O wait 陷阱** 可避免把 CPU 问题误判成「磁盘好了」。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 9.1–9.3 核心概念与模型 | [notes/section-9.1-9.3-核心概念与模型.md](./notes/section-9.1-9.3-核心概念与模型.md) |
| 9.4 硬件与软件架构 | [notes/section-9.4-硬件与软件架构.md](./notes/section-9.4-硬件与软件架构.md) |
| 9.5 分析方法论 | [notes/section-9.5-分析方法论.md](./notes/section-9.5-分析方法论.md) |
| 9.6 观测工具 | [notes/section-9.6-观测工具.md](./notes/section-9.6-观测工具.md) |
| 9.7–9.9 可视化、实验与调优 | [notes/section-9.7-9.9-可视化实验与调优.md](./notes/section-9.7-9.9-可视化实验与调优.md) |

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

- 上一章：[../chapter-08-file-systems/](../chapter-08-file-systems/)
- 下一章：[../chapter-10-network/](../chapter-10-network/)
- 内存 / cache：[../chapter-07-memory/](../chapter-07-memory/)
- 应用 I/O 阻塞：[../chapter-05-applications/](../chapter-05-applications/)
- 基准测试：[../chapter-12-benchmarking/](../chapter-12-benchmarking/)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- LKD 页回写：[03-Linux-Kernel-Development ch16](../../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-16-page-cache/)
- HFT 调优：[12-HFT ch05](../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
