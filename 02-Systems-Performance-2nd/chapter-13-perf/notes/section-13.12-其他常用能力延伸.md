## 13.12 其他常用能力（延伸）

| 子命令 | 用途 |
|--------|------|
| `perf mem` | 内存访问剖析 |
| `perf sched` | 调度延迟、迁移 |
| `perf lock` | 锁竞争 |
| `perf c2c` | **伪共享 / cache line** 争用（需支持） |
| `perf annotate` | 源码/汇编级热点 |

```bash
perf sched record -p $(pidof strategy) -- sleep 10
perf sched latency
```

**HFT 锁/伪共享：** `perf c2c record` 或 Ch 6 PMC + [04-Hennessy](../../../04-Computer-Architecture-6th/) — 争用严重时再开。

---


---

← [本章导读](../README.md)
