# 3. 传统内存分析工具

[Ch 3 § 60 秒](../../chapter-03-performance-analysis/) 已含部分命令；本章补充内存专项：

| 工具 | 看什么 |
|------|--------|
| `dmesg` | **OOM killer** 日志、杀谁、call trace |
| `free -h` | 总量、used、available、buff/cache |
| `vmstat 1` | `si/so` swap、`sc` 扫描、`pgmajfault` |
| `ps aux` / `top` | 进程 **RSS**、`%MEM` |
| `pmap -x PID` | 各 **VMA 段** 大小（heap、stack、mmap） |
| `sar -B 1` | 缺页率、页扫描活动 |
| `perf stat -e cache-misses,page-faults` | PMC 级 fault / cache |

```bash
free -h
vmstat 1
pmap -x $(pidof myapp) | tail -1    # total RSS 行
dmesg | grep -i oom
```

**局限：** 知「RSS 高」，不知 **哪条调用栈在分配** — 交给 BPF。

---
