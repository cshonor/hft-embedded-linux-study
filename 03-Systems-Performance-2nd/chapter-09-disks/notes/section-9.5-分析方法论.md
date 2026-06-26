## 9.5 分析方法论

### USE 方法（Disk）

对 **每块磁盘** 及 **控制器**：

| 字母 | 问什么 | 工具 |
|------|--------|------|
| **U** Utilization | 设备忙的时间比 | `iostat %util` |
| **S** Saturation | 队列长度、等待 | `iostat await`、`avgqu-sz`、PSI io |
| **E** Errors | 驱动/HBA/磁盘错 | `dmesg`、`smartctl`、/proc/diskstats |

→ [附录 A](../../appendix-A-USE方法Linux.md)

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


---

← [本章导读](../README.md)
