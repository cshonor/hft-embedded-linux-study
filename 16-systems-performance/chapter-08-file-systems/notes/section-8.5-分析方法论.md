## 8.5 分析方法论

### 延迟分析（Latency Analysis）

**测量层次：**

```
① 应用事务计时（端到端）
② syscall 层（strace / BPF 测 read/write 耗时）
③ VFS 层（vfs_read/write 追踪）
④ FS 层（ext4_file_read_iter / xfs_* 延迟直方图）
```

**事务成本（Transaction Cost）：**

```
事务成本 = 事务总时间中阻塞在 FS I/O 上的比例
```

若 < 1% — FS 不是瓶颈；若 > 10% — 查 cache 命中、慢 fsync、元数据风暴。

→ Ch 2 [延迟分解](../../chapter-02-methodologies/)

### 工作负载特征

| 维度 | 问什么 | 工具 |
|------|--------|------|
| **IOPS** | 每秒多少次 I/O | iostat、`filetop` |
| **吞吐量** | MB/s | `filetop`、sar |
| **I/O 大小** | 4K vs 1M | `biosnoop`、fio |
| **读/写比** | 读多还是写多 | sar、BPF |
| **随机/顺序** | 预取是否有效 | fio `--rw=randread` vs `read` |

### 微基准测试注意：WSS

| 测试集大小 | 实际测到的是 |
|------------|--------------|
| **WSS << RAM** | **page cache 性能** — 极快，误导 |
| **WSS >> RAM** | 磁盘 + FS 真实路径 |
| **O_DIRECT** | 绕过 cache，测磁盘/FS 直连 |

```bash
# 清空 page cache（仅测试环境！生产禁止）
echo 3 | sudo tee /proc/sys/vm/drop_caches
```

→ [Ch 12 基准测试](../../chapter-12-benchmarking/)

---


---

← [本章导读](../README.md)
