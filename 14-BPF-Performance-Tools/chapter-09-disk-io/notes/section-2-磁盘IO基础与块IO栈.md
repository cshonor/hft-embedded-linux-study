# 2. 磁盘 I/O 基础与块 I/O 栈

### 块 I/O 栈 (Block I/O Stack)

数据向下传递的典型路径：

```
文件系统 (ext4/xfs…)
    → 块设备接口
    → 卷管理 / LVM（可选）
    → device mapper（可选）
    → 块层 + I/O 调度器
    → HBA / SCSI / NVMe 驱动
    → 磁盘 / SSD / 阵列 / 网络存储
```

**HFT 共置机：** 系统盘写日志、数据盘 mmap 冷读、**swap 换出** 都会出现在块层 — 即使策略进程「不读写文件」。

### I/O 调度器 (Linux 5.0+)

| 调度器 | 特点 |
|--------|------|
| **mq-deadline** | 多队列默认常见；读/写 deadline |
| **Kyber** | 面向低延迟 SSD 的简化 mq 调度 |
| **BFQ** | 按进程公平，偏交互/桌面 |
| **none** | NVMe 上常见 — 软件调度最小化 |

**Multi-queue (blk-mq)：** 现代内核与 NVMe 队列深度匹配，替代旧单队列 CFQ 时代。

### 时间指标拆解

| 术语 | 含义 |
|------|------|
| **Wait Time** | 请求在 **OS/调度队列** 中排队等待 |
| **Service Time** | **设备实际处理** 时间 |
| **Request Time** | Wait + Service — **应用感知** 的块 I/O 时间 |

**`biosnoop`** 区分 **`QUE(ms)`**（排队）与 **`LAT(ms)`**（总延迟）— 判断 **盘慢** vs **队列拥塞**。

→ SysPerf 磁盘章：[chapter-09-disks](../../../13-Systems-Performance-2nd/chapter-09-disks/)

---
