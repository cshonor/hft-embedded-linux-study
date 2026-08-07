## 5.5 同步基础


> ↔ [CSAPP §12.5 信号量](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.5-信号量与预线程化.md)

### 硬件同步原语

| 原语 | 行为 |
|------|------|
| **Atomic exchange** | 原子交换 |
| **Test-and-set** | 测试并置位 — 简单锁基础 |
| **Fetch-and-increment** | 原子加一 — ticket 锁等 |
| **LL/SC** (Load-Linked / Store-Conditional) | 链接加载 + 条件存储 — **现代 RISC** 常用；x86 用 `LOCK` 前缀 cmpxchg 等 |

**LL/SC 要点：** SC 失败则重试 — 实现 CAS、锁、无锁栈/队列的基础。

---

### 自旋锁 (Spin Locks)

| 特点 | 说明 |
|------|------|
| 等待时 **忙等** | 不陷入内核 — 适合 **极短临界区** |
| **缓存一致性友好实现** | 在 **本地缓存行** 自旋，减少总线写流量（测试本地副本） |
| 竞争剧烈时 | 仍产生一致性风暴 — 需 **退避** 或 **futex** |

| HFT 视角 |
|----------|
| 热路径：**极短自旋** + `pause`（x86）可接受；长临界区用 **futex/mutex** 但避免进热路径 |
| **无锁队列**（SPSC/MPSC）— 用 CAS/LL-SC，避免锁；见 [16-HFT ch7](../../../21-hft-engineering/chapter-07-无锁数据结构与内存布局.md) |
| 自旋锁 **错误实现**（全局总线写）曾导致整机变慢 — 实现要 **test-test-and-set** 或 **queued lock** |

→ [01-CSAPP Ch12](../../../02-computer-systems/chapter-12-concurrent-programming/)


### 常见陷阱

- 在热路径用 mutex 保护长临界区 — mutex 争用 → 进内核 → **微秒级延迟**；热路径应用极短自旋 + atomic 或无锁结构
- 自旋锁不做 test-test-and-set — 直接原子写 → 每次写触发全总线 invalidate → **总线风暴**；应先读本地副本（test）再 set
- 以为 LL/SC 不会失败 — SC 可能因其他核写入同 cache line 而 **失败** → 需重试循环；且同 line 的无关写也会导致 SC 失败（false sharing on LL/SC）

### 自测题（点击展开）

<details>
<summary>Q1. 自旋锁的「缓存一致性友好实现」是什么？为什么比直接原子写好？</summary>

**test-test-and-set**：先在 **本地缓存行** 读锁状态（test），只在看到锁释放时才原子写（set）。→ 等待期间不发总线写 → 不产生一致性流量。直接原子写 → 每次写都触发 invalidate → 总线风暴。

</details>

<details>
<summary>Q2. LL/SC（Load-Linked/Store-Conditional）是什么？SC 失败时怎么办？</summary>

LL 读一个地址并标记；SC 只在 **LL 后该地址未被其他核修改** 时才写入成功。SC 失败 → 重试 LL/SC 循环。x86 没有 LL/SC，用 `LOCK CMPXCHG`（CAS）实现类似语义。

</details>

<details>
<summary>Q3. HFT 热路径什么时候用自旋锁，什么时候用无锁队列？</summary>

自旋锁：极短临界区（几条指令），配合 `pause` 指令。**不进内核** → 延迟低。无锁队列（SPSC/MPSC）：生产者-消费者场景，用 CAS/atomic 避免 **锁的争用和缓存乒乓**。长临界区用 mutex 但 **不进热路径**。

</details>

<details>
<summary>Q4. x86 的 `pause` 指令在自旋锁中起什么作用？</summary>

`pause`（REP NOP）：1) 提示 CPU 这是自旋等待 → 减少流水线功耗 2) 避免 **memory ordering violation** 在退出循环时的惩罚 3) 给超线程兄弟线程更多执行资源。自旋循环中 `while (!try_lock()) _mm_pause();`。

</details>
---
