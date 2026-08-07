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


### 常见陷阱

1. strace 生产直接跑——strace 开销巨大（每个 syscall 两次 ptrace），生产禁用或限时
2. perf trace 当 strace 用——perf trace 开销比 strace 低但仍有开销，生产限时长
3. ftrace 和 BPF 不分场景——ftrace 适合内核内建追踪，BPF 适合可编程聚合

<details>
<summary>自测题（点击展开）</summary>

1. 四大追踪器分别是什么？
   <details><summary>答</summary>strace（syscall）、perf trace（低开销 strace）、ftrace（内核追踪）、BPF（可编程追踪）</details>
2. 为什么 strace 不能在生产环境用？
   <details><summary>答</summary>每个 syscall 两次 ptrace 陷入，开销巨大——HFT 热路径会变成原来的 10-100 倍慢</details>
3. ftrace 和 BPF 各自适合什么场景？
   <details><summary>答</summary>ftrace 适合内核内建 tracepoint/函数追踪，BPF 适合可编程聚合（直方图/过滤/计算）</details>

</details>


---

← [本章导读](../README.md)
