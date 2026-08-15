## ⑦ 大内核锁 · The Big Kernel Lock (BKL)

历史包袱：曾经存在一把 **覆盖几乎整个内核** 的巨锁 — 进内核就可能拿着它，粗暴保证「内核非抢占式互斥」。

| 属性 | 说明 |
|------|------|
| 粒度 | **极粗** — 扩展性差 |
| 命运 | **逐步撕掉**；现代内核 **禁止新代码使用** |
| 学习意义 | 理解「为何要细粒度锁 / RCU / per-CPU」 |

#### 为何淘汰

| 问题 | 后果 |
|------|------|
| 多核无法并行执行大量内核路径 | SMP 扩展性差 |
| 持锁时间长 | 延迟、抖动大 |
| 隐式全局序列化 | 难推理、难优化 |

```
旧世界:  用户态 ──syscall──► [ BKL ] 几乎全家桶串行
新世界:  细粒度 spinlock / mutex / RCU / per-CPU
```

#### 你需要记住的唯一行动项

| 规则 | |
|------|--|
| **新代码禁止 BKL** | 用 Ch 10 其它机制 |
| 读旧补丁/旧驱动见 `lock_kernel` | 当作考古，迁移掉 |

**HFT / 驱动：** 不要怀念「一把大锁省事」— 大锁 = 尾延迟与多核浪费。正确粗细：数据怎么共享，锁就围着数据走。

→ [Ch 9 争用与可扩展性](../../chapter-09-kernel-sync-intro/notes/section-9.6-争用和可扩展性.md) · [10.11 选型](./section-10.11-选型速查Ch-9--Ch-10.md)

### 常见陷阱

1. 以为 BKL 还在现代内核——2.6.37 完全移除，不存在了
2. 把 BKL 当通用锁——BKL 是特殊锁（可睡眠、递归、全局唯一），不推荐用于新代码
3. 以为 BKL 和 mutex 一样——BKL 可递归加锁、自动释放于 schedule()，mutex 不能

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** BKL（Big Kernel Lock）是什么？为什么被移除？

<details><summary>答案</summary>

BKL 是 Linux 早期从 SMP 过渡时用的全局锁。特点：① 全局唯一（一把锁保护所有）。② 可睡眠（schedule 时自动释放，返回后重新获取）。③ 可递归（同一进程可多次 lock）。移除原因：① 全局锁是性能瓶颈（多核扩展性差）。② 可睡眠+自动释放导致语义复杂。③ 阻碍 PREEMPT_RT。2.6.37 完全移除，所有 BKL 用户改为 mutex/spinlock。

</details>

**Q2.** BKL 移除后，原来用 BKL 的代码改用了什么？

<details><summary>答案</summary>

每个子系统逐个迁移：① ioctl → per-file mutex。② 文件系统 → per-superblock lock。③ 驱动 → per-device mutex。迁移过程持续多个版本（2.6.26-2.6.37），通过 `lock_kernel()`/`unlock_kernel()` 标记 BKL 用户，逐个替换。迁移后 SMP 扩展性显著提升。

</details>

**Q3.** BKL 的历史教训对 HFT 设计有什么启示？

<details><summary>答案</summary>

① 避免全局锁——用 per-thread/per-CPU 数据消除共享。② 可睡眠锁不是万能的——BKL 可睡眠但导致语义混乱。③ 锁的可扩展性比锁的正确性更难——BKL 是正确的但不可扩展。④ 逐步替换优于大重写——BKL 花了 5 年逐个迁移。HFT 设计：热路径无锁，冷路径细粒度锁。

</details>

</details>

---
