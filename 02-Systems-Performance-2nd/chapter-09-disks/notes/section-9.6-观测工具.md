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

→ [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C](../../appendix-C-bpftrace单行命令.md)

### 底层与硬件

| 工具 | 用途 |
|------|------|
| **`blktrace` + `blkparse`** | 块层极细追踪 |
| **`smartctl -a /dev/sdX`** | SMART 健康、重映射扇区 |
| **`MegaCli` / 厂商 CLI** | RAID 状态、物理盘、BBU |

**Sloth Disk 排查：** `biosnoop` 见单 I/O > 1s + SMART 仍「OK」→ 换盘测试。

---


---

← [本章导读](../README.md)
