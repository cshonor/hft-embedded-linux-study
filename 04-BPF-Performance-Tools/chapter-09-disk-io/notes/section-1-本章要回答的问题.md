# 1. 本章要回答的问题

| 传统工具局限 | BPF 补什么 |
|--------------|------------|
| `iostat` 只有 **平均值** | **`biolatency`** 直方图 + 长尾 |
| `blktrace` **开销大、日志海量** | **`biosnoop`** 可控采样、内核聚合 |
| 不知 **谁** 在打盘 | **`biotop`** |
| 不知 **哪段内核/应用栈** 发起 I/O | **`biostacks`** |

```
Ch 8 逻辑 I/O（cachestat / fileslower）
        ↓ cache miss / fsync / swap
块层 Block Layer（biolatency / biosnoop / biostacks）
        ↓
I/O 调度器 mq-deadline / Kyber / BFQ（iosched）
        ↓
SCSI / NVMe 驱动（scsilatency / nvmelatency）
        ↓
设备
```

---
