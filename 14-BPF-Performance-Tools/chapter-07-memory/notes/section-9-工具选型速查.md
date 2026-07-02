# 9. 工具选型速查

| 症状 | 优先工具 |
|------|----------|
| 进程 OOM 被杀 | `dmesg` → `oomkill` |
| RSS 缓慢上涨 | `memleak`（限时） |
| 堆增长 | `brkstack` |
| 大 mmap | `mmapsnoop` |
| 启动后首次慢 | `faults`、`ffaults` |
| 莫名卡顿 | `drsnoop`、`vmscan` |
| swap 活动 | `vmstat si/so` → `swapin` |
| 大页配置验证 | `hfaults` |
| cache/LLC | [Ch 6 `llcstat`](../../chapter-06-cpus/) |

---
