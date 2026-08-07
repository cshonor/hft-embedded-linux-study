## ③ 读-写自旋锁 · Reader-Writer Spin Locks

允许多个读者 **并行** 持锁，写者 **独占**。仍是自旋锁家族 — **读者/写者都不能睡眠**。

| 角色 | 规则 |
|------|------|
| **读者** | 可与其他读者共存；见写者则等 |
| **写者** | 独占；等所有读者/写者离开 |

#### 偏袒读者 · Reader-biased

| 行为 | 含义 |
|------|------|
| 持续有新读者进入 | **写者可能长时间拿不到锁**（写者饥饿） |
| 适合 | **读极多、写极少**，且写可容忍延迟 |
| 不适合 | 写也要及时（改用 seqlock / RCU / 普通 spinlock） |

#### API 直觉

| API | 作用 |
|-----|------|
| `read_lock` / `read_unlock` | 共享读 |
| `write_lock` / `write_unlock` | 独占写 |
| `*_irqsave` / `*_bh` | 与 spinlock 相同：防中断/ softirq 重入 |

#### 代价

| 点 | 说明 |
|----|------|
| 缓存行乒乓 | 读者也要改锁状态 → 多核读多时也可能不划算 |
| 现代趋势 | 很多路径改用 **RCU**（读侧极快）或 **seqlock** |

**HFT：** 行情只读快照 + 偶发配置更新 → 想 rwlock；若写必须立即可见且读者极多 → 看 **seqlock（10.8）** 或用户态无锁环。**不要** 在读锁里做长循环。

→ [10.2 spinlock](./section-10.2-自旋锁.md) · [10.8 seqlock](./section-10.8-顺序锁.md)

### 常见陷阱

1. 以为读写锁总是比普通锁好——写多读少时读写锁退化（写者饥饿），反而更慢
2. 混淆 rwlock 和 RCU——rwlock 读端仍需原子操作（有开销），RCU 读端无开销
3. 在读写锁的读端做耗时操作——会阻塞写者（写者等所有读者退出）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 读写自旋锁（rwlock）适合什么场景？有什么缺点？

<details><summary>答案</summary>

适合：读多写少（如路由表/配置表），多个读者可并发。缺点：① 写者饥饿：如果有持续的新读者进来，写者可能无限等待。② 读端仍有原子操作开销（递增 reader count）。③ 公平性：部分实现有公平性保证（防止写者饥饿），但会降低读吞吐。现代内核更推荐 RCU（读端零开销）替代 rwlock。

</details>

**Q2.** RCU 相比 rwlock 有什么优势？

<details><summary>答案</summary>

RCU 读端：`rcu_read_lock()` 只禁抢占（无原子操作，零开销）。rwlock 读端：`read_lock()` 原子递增 reader count（有 cache line bouncing）。RCU 写端：复制 + 替换指针 + 等 grace period。rwlock 写端：等所有读者退出。RCU 适合：读极多写极少。rwlock 适合：读写都有但读多。RCU 缺点：写端延迟大（等 grace period）。

</details>

**Q3.** HFT 中如何选择读写锁 vs RCU vs 无锁？

<details><summary>答案</summary>

① 读极多写极少（路由表/配置）：RCU（内核）/ `std::shared_ptr<const T>`（用户态）。② 读写都有但读多：`std::shared_mutex`（用户态）/ rwlock（内核）。③ 热路径数据：无锁（SPSC 队列/per-thread 数据）。④ 配置变更：双缓冲（atomic swap pointer + 延迟释放旧版）。HFT 原则：热路径零开销，冷路径可接受锁。

</details>

</details>

---
