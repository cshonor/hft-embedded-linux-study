## 8.7–8.8 实验与调优

### fio 基准测试

**fio** = 灵活 I/O 测试器 — 支持随机分布、延迟分位（P99、P99.99）、多线程。

```bash
fio --name=seqread --filename=/data/testfile --size=32G \
    --rw=read --bs=1M --direct=1 --ioengine=libaio \
    --runtime=60 --time_based --group_reporting
```

| 参数 | 含义 |
|------|------|
| **`--direct=1`** | O_DIRECT，测磁盘非 cache |
| **`--size`** | 必须 **> RAM** 才测真磁盘（否则测 cache） |
| **`--bs`** | 块大小 — 对齐应用真实 I/O |
| **`--rw`** | read/write/randread/randwrite |

**HFT：** 上线前对 **日志盘** 单独 fio — 确认与 NVMe 数据面 **不共享瓶颈**。

### 应用层调优

| API | 作用 |
|-----|------|
| **`posix_fadvise()`** | 顺序/随机、willneed、dontneed |
| **`madvise()`** | mmap 区域：SEQUENTIAL、RANDOM、DONTNEED |
| **`sync_file_range()`** | 范围刷盘（细粒度控制） |

**原则：** 给内核 **正确 hint** 比盲目增大 buffer 更有效。

### 挂载与 FS 参数

| 选项 / 参数 | 效果 |
|-------------|------|
| **`noatime` / `relatime`** | 减少读触发的元数据写 |
| **`barrier` / `nobarrier`** | 一致性 vs 性能 — **生产慎用 nobarrier** |
| **ext4 `data=ordered`** | 默认平衡 |
| **XFS `allocsize`** | 预分配减少碎片 |
| **ZFS `recordsize`** | 匹配应用 I/O 大小（如 128K） |

**HFT 日志盘示例（思路，按合规调整）：**

```
UUID=... /var/log/hft  xfs  noatime,nodiratime,logbufs=8  0 2
```

### USE 方法（File System 视角）

| 字母 | 问什么 |
|------|--------|
| **U** | cache 利用、FS 层 CPU |
| **S** | 慢 I/O 队列、应用阻塞在 read/write/fsync |
| **E** | I/O error、只读 remount |

---


---

← [本章导读](../README.md)
