# 8. Off-CPU 时间 — `offcputime` 🔴

与 `profile` **完美互补**：

| | `profile` | `offcputime` |
|---|-----------|--------------|
| **采样时机** | 线程 **在 CPU 上** | 线程 **被切换下 CPU**（阻塞） |
| **回答** | 在算什么 | **在等什么**（I/O、锁、futex…） |
| **输出** | 栈频率 → 火焰图 | 栈 + **等待时间** 汇总 |

```bash
sudo offcputime-bpfcc -p $(pidof myapp) 30
```

**典型发现：** 热路径在等 `futex`、等 `epoll`、等磁盘 — 引导到 [Ch 7 内存](../../chapter-07-memory/) / [Ch 9 磁盘](../../chapter-09-disk-io/) / 应用锁分析。

**HFT：** P99 升高但 `profile` 无热点 → **优先 offcputime**（是否在等锁或内核 I/O）。

---
