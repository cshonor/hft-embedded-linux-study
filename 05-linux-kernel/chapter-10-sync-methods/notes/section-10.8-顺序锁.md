## ⑧ 顺序锁 · Seqlocks

**偏袒写者** 的读写同步：读者 **无锁快速路径**，通过序号检测是否与写者冲突；冲突则重读。

| 角色 | 行为 |
|------|------|
| **写者** | 拿锁（或关抢占）→ 序号变奇 → 写数据 → 序号变偶 |
| **读者** | 读序号 → 读数据 → 再读序号；若期间序号变了 → **重试** |

```
读者:
  do {
    seq = read_seqbegin(&lock);
    /* 读共享数据快照 */
  } while (read_seqretry(&lock, seq));
```

#### 与 rwlock 对比

| | rwlock | seqlock |
|--|--------|---------|
| 偏向 | **读者**（写者可能饿） | **写者**（读者可能重试） |
| 读者 | 要拿读锁 | **通常不写锁变量**（只读 seq） |
| 适合 | 读多写少、写可等 | 读极多、写很少、**写必须及时** |
| 读者副作用 | — | 读侧 **不可有副作用**（可能重复执行） |

#### 适用数据

| 适合 | 不适合 |
|------|--------|
| 统计计数器、时钟相关、配置快照 | 读者要「只执行一次」的副作用 |
| 写极短 | 写很长（读者狂重试） |

**HFT：** 行情最新价、全局配置版本号很适合 seqlock 思维；用户态也可做 seqlock 风格。注意 **多字段一致**：必须整段读在 begin/retry 之间，并配合屏障语义。

→ [10.3 rwlock](./section-10.3-读-写自旋锁.md) · [10.10 屏障](./section-10.10-排序和屏障.md)

### 常见陷阱

1. 以为 seqlock 是通用读写锁——只适合写少读多 + 读端可容忍重试的场景
2. 在读端忽略 sequence 检查——seqlock 读端必须检查前后 sequence 一致，否则可能读到半写状态
3. 在写端用多个步骤——写端持锁期间应尽快完成，持锁时间 = 写者互斥时间

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** seqlock 的工作原理？读写端各做什么？

<details><summary>答案</summary>

写端：`write_seqlock()` → sequence++（奇数）→ 写数据 → sequence++（偶数）→ `write_sequnlock()`。写端之间互斥（spinlock）。读端：`seq1 = read_seqbegin()` → 读数据 → `seq2 = read_seqretry(seq1)` → 如果 seq1 是奇数或 seq1 != seq2 → 重读。读端无锁（不阻塞写者），但可能需要重试。

</details>

**Q2.** seqlock 适合什么场景？不适合什么？

<details><summary>答案</summary>

适合：① 写极少读极多。② 读端可以容忍偶尔重试。③ 数据简单（几个字段，重读代价低）。典型：`jiffies`（时间戳）、`getnstimeofday()`、统计计数器。不适合：① 复杂数据结构（链表/树），重读代价高。② 写频繁（写者互斥 + 读者频繁重试）。③ 读端需要阻塞写者。这些场景用 RCU 或 rwlock。

</details>

**Q3.** HFT 中 seqlock 的用户态实现？

<details><summary>答案</summary>

```c
// 无锁读取时间戳
struct { std::atomic<uint32_t> seq; uint64_t value; } ts;
// 写端
uint32_t s = ts.seq.load(std::memory_order_relaxed);
ts.seq.store(s + 1, std::memory_order_release);  // 奇数
ts.value = rdtsc();
ts.seq.store(s + 2, std::memory_order_release);  // 偶数
// 读端
uint32_t s1, s2; uint64_t v;
do {
    s1 = ts.seq.load(std::memory_order_acquire);
    v = ts.value;
    s2 = ts.seq.load(std::memory_order_acquire);
} while (s1 != s2 || s1 & 1);  // 重试
```

</details>

</details>


> ↔ [ULK Ch5 §5 顺序锁与RCU](../../../18-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-5-顺序锁与RCU.md)
---
