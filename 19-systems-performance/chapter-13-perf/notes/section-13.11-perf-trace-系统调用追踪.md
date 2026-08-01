## 13.11 `perf trace` — 系统调用追踪

类似 **strace**，基于 perf 基础设施 — **通常更低开销**。

```bash
perf trace -p $(pidof strategy) -- sleep 5
perf trace -e open,read,write,mmap -- sleep 3
```

| vs strace | perf trace |
|-----------|------------|
| 经典、功能全 | 集成 perf 生态 |
| 开销常较大 | 相对轻 |
| 生产慎用 | **仍限时长** |

**HFT：** 发现热路径 unexpected `read`/`mmap` — 开发机 `perf trace` 5 秒定位 syscall 类型。

---


---

← [本章导读](../README.md)
