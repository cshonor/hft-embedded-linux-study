## ⑥ 争用和可扩展性 · Contention and Scalability

#### 锁争用（lock contention）

| 定义 | 影响 |
|------|------|
| 锁 **已被占用**，其他线程 **排队/自旋** 等待 | **瓶颈** — CPU 空转或睡眠唤醒开销 |

**HFT：** `perf lock`、延迟尖刺 — **高争用 mutex** 在热路径上是 **P99 杀手**。

#### 可扩展性（scalability）

| 定义 | 说明 |
|------|------|
| 增加 CPU 数量时，系统 **性能提升程度** | 理想线性扩展很少见 |

| 敌人 | 原因 |
|------|------|
| **粗粒度大锁** | 多 CPU **挤一扇门** — 扩展性极差 |
| **细粒度过多锁** | 无争用时 **加锁开销** 本身浪费 |

#### 锁粒度（granularity）

| 粒度 | 争用高时 | 争用低时 |
|------|----------|----------|
| **粗** | 扩展差 | 实现简单 |
| **细** | 扩展好 | 开销可能偏大 |

#### 工程建议（作者）

```
从简单锁开始 ──►  profiling 见争用 ──► 再细化粒度 / per-CPU / RCU
```

| 阶段 | 做法 |
|------|------|
| 初版 | **一把锁护结构** — 正确优先 |
| 优化 | 仅在有 **实测争用** 时拆锁、减临界区 |

→ **Ch 10** spinlock、mutex、seqlock、RCU 选型

### 常见陷阱

1. 以为锁的正确性就够了——锁的争用程度直接影响性能和可扩展性
2. 混淆锁持有时间和锁等待时间——持有时间是「锁住了多久」，等待时间是「等了多久才拿到」
3. 以为增加 CPU 数量总能提升性能——锁争用下，增加 CPU 反而降低吞吐（锁竞争恶化）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 锁争用（contention）怎么测量？

<details><summary>答案</summary>

内核：① `perf lock record` + `perf lock report`：记录锁等待时间和持有时间。② `/proc/lock_stat`：lockdep 统计。③ `bpftrace -e 'tracepoint:lock:lock_acquire { ... }'`：追踪锁获取。用户态：① `perf lock`。② `Valgrind --tool=drd`。③ `pthread_mutex` 的 `trylock` 探测。指标：con-<N>（等待次数）、wait-total（总等待时间）、hold-total（总持有时间）。

</details>

**Q2.** 为什么增加 CPU 在锁争用下反而降低性能？

<details><summary>答案</summary>

假设一把全局锁保护共享数据。N 个 CPU 同时请求锁：① 只有 1 个拿到，其余 N-1 个 spin 等待。② N 越大，spin 浪费的 CPU 越多。③ 锁释放时 N-1 个 CPU 抢锁 → cache line bouncing。④ 吞吐随 N 增加先升后降（Amdahl's Law 的锁版本）。解决：① 减小临界区。② per-CPU 数据。③ 无锁数据结构（RCU）。

</details>

**Q3.** HFT 如何设计无锁/低争用数据结构？

<details><summary>答案</summary>

① SPSC 环形队列：单生产者单消费者，`atomic<head>` + `atomic<tail>` + release/acquire 序。② per-thread 缓存：每线程独立操作，定期聚合。③ RCU 模式：读端无锁（`atomic load` 指针），写端复制+替换+延迟回收。④ 分片锁：`sharded_hashmap`，N 个 bucket 各一把锁，减少争用。⑤ `std::shared_mutex`：多读单写，适合读多写少。

</details>

</details>

---
