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

**HFT 锁/伪共享：** `perf c2c record` 或 Ch 6 PMC + [19-Hennessy](../../../19-computer-architecture/) — 争用严重时再开。

---


### 常见陷阱

1. perf c2c 不用——伪共享是 HFT 常见杀手，perf c2c 能直接量化 cache line 争用
2. perf sched 只看调度次数——latency 子命令看调度延迟（线程就绪到被 CPU 执行），这才是 HFT 关心
3. perf lock 不在开发阶段用——锁竞争在生产才暴露，但开发阶段用 perf lock 可以提前发现

<details>
<summary>自测题（点击展开）</summary>

1. perf c2c 能发现什么问题？
   <details><summary>答</summary>伪共享（false sharing）——多线程写同一 cache line 不同字段，cache 一致性协议打穿性能</details>
2. perf sched latency 和 sched record 的区别？
   <details><summary>答</summary>record 采集调度事件；latency 分析线程从就绪到被 CPU 执行的延迟——HFT 关心调度等待时间</details>
3. HFT 什么时候该用 perf lock？
   <details><summary>答</summary>怀疑锁竞争导致 tail latency——perf lock 量化锁等待时长和持有时长</details>

</details>


---

← [本章导读](../README.md)
