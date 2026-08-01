## 5.5 同步基础

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

---
