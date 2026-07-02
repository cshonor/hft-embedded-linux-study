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

→ Ch 8 [FS 上层](../../chapter-08-file-systems/) · [09 Rosen 块层](../../../11-Linux-Kernel-Networking/)（网络栈不同，块层见 LKD）

---


---

← [本章导读](../README.md)
