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

→ Ch 8 [fio 与 WSS](../../chapter-08-file-systems/) · [Ch 12](../../chapter-12-benchmarking/)

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

→ Ch 6/7 [PSI 概念](../../chapter-06-cpus/)

---


---

← [本章导读](../README.md)
