## 4.5 四大追踪器

Gregg 归纳的现代 Linux **高级追踪** 分工：

| 工具 | 定位 | 擅长 |
|------|------|------|
| **perf** | 官方剖析器 | CPU 采样、PMC、部分 trace、火焰图 |
| **Ftrace** | 内核内置 | 内核函数路径、调度、irq、latency histogram |
| **BCC** | eBPF + Python/Lua 前端 | 复杂脚本、生产级工具集（biolatency…） |
| **bpftrace** | eBPF 单行 DSL |  ad hoc 查询、一行命令、教程友好 |

**关系：**

```
        ┌─────────── 数据源 ───────────┐
        │ /proc  PMC  tracepoint     │
        │ kprobe  uprobe  USDT       │
        └─────────────┬──────────────┘
                      │
     ┌────────────────┼────────────────┐
     ▼                ▼                ▼
   perf            Ftrace          eBPF 引擎
     │                │                │
     │                │         ┌──────┴──────┐
     │                │         ▼             ▼
     └────────────────┴────  BCC        bpftrace
```

**HFT 实践路径：**

1. **perf** — 火焰图、cache miss（Ch 13）
2. **bpftrace** — syscall 计数、run queue 延迟、网络栈 tracepoint（Ch 15 + 附录 C）
3. **Ftrace** — 内核延迟 odd case（Ch 14）
4. **BCC** — 现成工具不够时再写 Python BPF

---


---

← [本章导读](../README.md)
