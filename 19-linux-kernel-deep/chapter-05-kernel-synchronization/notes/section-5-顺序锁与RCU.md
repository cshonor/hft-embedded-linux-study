## 5. 顺序锁 (Seqlocks) 与 RCU

---

### 一、顺序锁 (Seqlocks)

基于 **序列计数器** 的锁：

| 角色 | 行为 |
|------|------|
| **写者** | 加锁，更新数据，**序列号 +1** |
| **读者** | 读前读序列号 A → 读数据 → 读序列号 B；若 **A ≠ B** 或序列号为奇数（正在写），**重读** |

适合：**读极多、写很少**，且读者可容忍重试（如 jiffies、某些时间戳）。

---

### 二、读-拷贝-更新 (RCU)

**Read-Copy Update** — 高效 **读多写少** 的免锁（对读者）机制：

```
读者：无锁读（需内存屏障保证看到一致指针）
写者：复制副本 → 改副本 → 原子换指针
      → 等所有旧读者进入 quiescent state（静止态）
      → 释放旧副本
```

| 特点 | 说明 |
|------|------|
| 读者开销 | 极低 — HFT/网络路由等广泛使用（modern 内核） |
| 写者 | 延迟回收，实现较复杂 |
| 静止态 | 如一次调度切换、用户态边界 |

Modern 内核中 RCU 远比 2.6 更普遍；ULK 给出 **概念起点**。

---

### 三、HFT 关联

- 读路径无锁 → **延迟更稳定**  
- 写路径延迟回收 — 不适合写频繁的结构

### 常见陷阱

1. 把 ULK 讲的 RCU 当现代版——现代有 Tree RCU、Sleepable RCU (SRCU)、Tasks RCU，API 完全不同
2. 以为 seqlock 是通用读写锁——seqlock 只适合「写少读多 + 读端可容忍重试」的场景
3. 在 RCU 读端临界区中睡眠——普通 RCU (`rcu_read_lock()`) 不能睡眠，只有 SRCU (`srcu_read_lock()`) 可以

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** RCU（Read-Copy-Update）的核心思想是什么？

<details><summary>答案</summary>

读端无锁：`rcu_read_lock()` 只禁抢占（无开销），读者直接访问旧数据。写端复制：写者复制一份数据，修改副本，然后用 `rcu_assign_pointer()` 原子替换指针。回收：写者调用 `synchronize_rcu()` 等待所有读端退出，再释放旧数据。关键：读者看到的是旧版本或新版本，绝不会是中间状态。适合：读多写少的数据结构（路由表、VMA 链表）。

</details>

**Q2.** Tree RCU 和 ULK 讲的 RCU 有什么区别？

<details><summary>答案</summary>

ULK 讲的是经典 RCU（单一 grace period 检测）。Tree RCU（2.6.29+）把 CPU 组织成树形结构，每层汇报 quiescent state，避免全局扫描所有 CPU。在 1000+ CPU 系统上，Tree RCU 的 grace period 从秒级降到毫秒级。API 变化：`synchronize_rcu()` 仍可用，但底层实现完全不同。新增 `call_rcu()`（异步回收）和 `rcu_barrier()`（等所有 pending 回收完成）。

</details>

**Q3.** seqlock 在什么场景下比 RCU 更合适？

<details><summary>答案</summary>

Seqlock 适合：① 数据结构简单（几个计数器/时间戳）。② 写频率高于 RCU 的舒适区（RCU 写端开销大）。③ 读端可以容忍偶尔重读。典型用法：`jiffies` 和 `getnstimeofday()`——写者更新时间戳时递增 sequence number，读者检查前后 sequence 一致则读成功。不适合复杂数据结构（链表/树），因为读端重试代价高。

</details>

</details>

---

← [4. 自旋锁](./section-4-自旋锁.md) · 下一节 [6. 信号量与完成变量](./section-6-信号量与完成变量.md)
> ↔ [LKD Ch10 §10.8 顺序锁](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-10-sync-methods/notes/section-10.8-顺序锁.md)
