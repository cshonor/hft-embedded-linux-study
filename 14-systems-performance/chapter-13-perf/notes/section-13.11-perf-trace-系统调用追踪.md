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


### 常见陷阱

1. perf trace 生产长跑——虽然比 strace 轻但仍有开销，生产应限时长
2. perf trace 不限事件——trace -e 指定 syscall 类型，全量 trace 开销巨大
3. strace 和 perf trace 不区分——strace 用 ptrace（开销巨大），perf trace 用 perf 基础设施（相对轻）

<details>
<summary>自测题（点击展开）</summary>

1. perf trace 和 strace 的根本区别？
   <details><summary>答</summary>strace 用 ptrace（每 syscall 两次陷入，开销巨大）；perf trace 用 perf 基础设施（相对轻）</details>
2. perf trace 在生产环境的注意事项？
   <details><summary>答</summary>限时长 + 指定事件（-e open,read,write）——全量 trace 开销巨大</details>
3. HFT 什么时候用 perf trace？
   <details><summary>答</summary>发现热路径 unexpected read/mmap——开发机 perf trace 5 秒定位 syscall 类型</details>

</details>


---

← [本章导读](../README.md)
