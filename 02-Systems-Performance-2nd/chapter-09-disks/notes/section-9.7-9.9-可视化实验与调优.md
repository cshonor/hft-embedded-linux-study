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

→ Ch 2 [热力图 / FlameScope](../../chapter-02-methodologies/)

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


---

← [本章导读](../README.md)
