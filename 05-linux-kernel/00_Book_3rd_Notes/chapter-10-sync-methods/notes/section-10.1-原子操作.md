## ① 原子操作 · Atomic Operations

**其他同步机制的基础** — 对共享变量的修改在指令级 **不可分割**：要么全做完，要么完全没做；中间状态对其它 CPU/中断不可见。

#### 为何需要

| 非原子 `++x`（概念） | 风险 |
|---------------------|------|
| 读 → 加 → 写 多步 | 两 CPU 交错 → 丢更新 |
| 编译器拆指令 | 中间值被看到 |

#### 原子整数 · `atomic_t` / `atomic64_t`

| 类型 | 宽度 |
|------|------|
| **`atomic_t`** | 至少 32 位（常用 32） |
| **`atomic64_t`** | 64 位 |

| 用专用类型的原因 | 说明 |
|------------------|------|
| 类型安全 | 原子 API **只接** 这些类型，避免普通 `int` 误用 |
| 防错误优化 | 访问路径不被编译器「拆坏」 |
| 屏蔽架构差异 | x86 `lock` 前缀 / ARM LL/SC 等藏在头文件后 |

```c
atomic_t v = ATOMIC_INIT(0);
atomic_inc(&v);
atomic_dec_and_test(&v);   /* 减到 0 则返回真 — 常见于引用计数 */
atomic_add(3, &v);
atomic_set(&v, 1);
int x = atomic_read(&v);
```

#### 原子位操作

| 对比 | 说明 |
|------|------|
| 原子位 API | 对 **通用内存地址** 某 bit 置位/清除/测试 |
| **`__` 前缀非原子版** | 已在锁保护下可用 — **更快**，勿在无保护路径用 |

#### 能替代锁吗？

| 可以 | 不可以单靠原子 |
|------|----------------|
| 计数器、标志、引用计数 | **多字段一致更新**（要事务或锁） |
| 无锁结构的 **单个** 状态字 | 复杂不变式（常需屏障 + 仔细设计） |

**HFT 对照：** 用户态 `std::atomic` / `__atomic_*` 与内核 `atomic_t` 同一层；热路径 **计数、序号、就绪标志** 优先原子。记得 **内存序**（见 10.10）— 原子读写默认语义随 API/架构而变。

→ [10.2 自旋锁](./section-10.2-自旋锁.md) · [10.10 屏障](./section-10.10-排序和屏障.md) · [02-CSAPP 并发](../../../../02-computer-systems/chapter-12-concurrent-programming/)

### 常见陷阱

1. 把原子操作当万能锁——原子操作只保证单操作原子性，多操作组合仍需锁
2. 混淆 atomic_t 和 refcount_t——refcount_t 防溢出（0 不会变 -1），atomic_t 会
3. 以为 atomic_read() 是原子操作——在 x86 上它只是普通读（volatile），不保证其他 CPU 的写入可见

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** atomic_t 和 refcount_t 的区别？

<details><summary>答案</summary>

atomic_t：纯原子计数器，`atomic_dec(&v)` 可以从 0 变成 -1（UAF 漏洞）。refcount_t：引用计数专用，`refcount_dec()` 在 0 时 WARN + 阻止下溢。6.x 内核中 task_struct 的 usage 已从 atomic_t 改为 refcount_t。安全代码应始终用 refcount_t 管理生命周期。

</details>

**Q2.** `atomic_inc(&v)` 在 x86-64 上实际生成什么指令？

<details><summary>答案</summary>

`lock incl (%rdi)`——LOCK 前缀 + incl 指令。LOCK 前缀锁 cache line（通过 MESI 协议的 Read-Modify-Write 周期），确保原子性。开销：~20-40 cycles（无争用时）。争用时 cache line bouncing，可达数百 cycles。ARM64 上生成 `ldaxr`/`stlxr`（独占加载/存储）循环。

</details>

**Q3.** HFT 用户态如何高效使用原子操作？

<details><summary>答案</summary>

```c
// 无锁 SPSC 队列
std::atomic<size_t> head{0}, tail{0};
// 生产者
head.store(head.load(std::memory_order_relaxed) + 1,
           std::memory_order_release);
// 消费者
size_t h = head.load(std::memory_order_acquire);
if (h > tail.load(std::memory_order_relaxed)) {
    // 有数据
}
// 关键: release/acquire 配对, 避免 seq_cst 的全屏障开销
```

</details>

</details>

---
