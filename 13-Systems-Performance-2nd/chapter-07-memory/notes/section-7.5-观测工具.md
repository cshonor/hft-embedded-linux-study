## 7.5 观测工具

### 传统统计工具

| 工具 | 看什么 | 关键字段 |
|------|--------|----------|
| **`vmstat 1`** | 全局内存与 Swap | `free`、`buff/cache`、**`si`/`so`**、`swpd` |
| **`sar -r` / `sar -B`** | 历史内存、分页 | `-B`：pgpgin/out、fault |
| **`slabtop`** | 内核 Slab | 哪个 cache 占用异常 |
| **`numastat`** | NUMA 命中 | `numa_hit` vs `numa_foreign` |

### 进程级工具

| 工具 | 看什么 |
|------|--------|
| **`top` / `ps`** | **VSZ**（虚拟）vs **RSS**（常驻物理） |
| **`pmap -x` / `pmap -X`** | 映射明细；**PSS** = 共享页按比例分摊 |
| **`/proc/PID/smaps`** | 每映射 RSS/Pss/Shared — 脚本化分析 |

**VSZ vs RSS vs PSS：**

| 指标 | 含义 |
|------|------|
| **VSZ** | 地址空间大小 — 含未 touch 的映射，**可远大于 RAM** |
| **RSS** | 实际在物理内存的页 — 共享库 **整页算给每个进程** |
| **PSS** | 共享页按进程数分摊 — **更公平的总占用** |

### perf 与 BPF

| 工具 | 用途 |
|------|------|
| **`perf stat -e page-faults,major-faults,minor-faults`** | 缺页计数 |
| **`perf record -e page-faults -g`** | 缺页火焰图 |
| **`drsnoop`（BCC）** | 追踪 **direct reclaim** 路径延迟 |
| **`wss`（BCC，实验）** | 估算进程 WSS |

```bash
# Swap 是否在发生（持续监控）
vmstat 1 | awk 'NR>2 {print $7,$8}'   # si so

# 缺页热点（开发/压测环境）
perf record -e major-faults -g -p $(pidof strategy) -- sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > major-fault.svg
```

→ [Ch 13 perf](../../chapter-13-perf/) · [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C](../../appendix-C-bpftrace单行命令.md)

---


---

← [本章导读](../README.md)
