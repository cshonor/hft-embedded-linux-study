# 7.5 MPMC 的复杂度

> 第 7 章 · 上一节：[7.4 无锁队列（SPSC）](04-spsc-queue.md) · 下一章：[8.1 并行划分策略](../ch08-designing-concurrent-code/01-partitioning.md)

## 这节讲什么

当多个生产者或多个消费者存在时，SPSC 的"每个变量单写者"前提被打破——必须用 CAS。本节讲 MPMC 无锁队列（Michael-Scott 队列）的实现、CAS 竞争下的性能退化、以及为什么 HFT 中 MPMC 通常改用有锁或 SPSC 拆分。

---

## 核心规则（代码+表格）

### Michael-Scott 无锁 MPMC 队列

```cpp
template <typename T>
class mpsc_queue {  // 多生产者单消费者，简化版
    struct node {
        std::atomic<T*> data{nullptr};
        std::atomic<node*> next{nullptr};
    };
    alignas(64) std::atomic<node*> head;   // 生产者 CAS 竞争
    alignas(64) std::atomic<node*> tail;   // 消费者独占

public:
    mpsc_queue() {
        node* dummy = new node;
        head.store(dummy);
        tail.store(dummy);
    }

    // 多生产者调用
    void push(const T& value) {
        node* n = new node;
        n->data = new T(value);
        node* prev = head.exchange(n, std::memory_order_acq_rel);
        prev->next.store(n, std::memory_order_release);
    }
    // 注：这是 Vyukov MPMC 队列的变体，用 exchange 代替 CAS 减少竞争

    // 单消费者调用
    bool pop(T& result) {
        node* t = tail.load(std::memory_order_relaxed);
        node* next = t->next.load(std::memory_order_acquire);
        if (!next) return false;  // 空
        result = *next->data;
        delete next->data;
        delete t;
        tail.store(next, std::memory_order_release);
        return true;
    }
};
```

### SPSC vs MPMC 复杂度对比

| 维度 | SPSC | MPMC |
|------|------|------|
| 写者数 | 1 | 多 |
| 需要 CAS | 否 | 是（或 exchange） |
| ABA 风险 | 无 | 有（地址重用） |
| 内存回收 | 简单（生产者独占） | 复杂（hazard/epoch） |
| 竞争退化 | 无 | 高并发下 CAS 重试激增 |
| 实现难度 | 低 | 高 |

### CAS 竞争的性能退化

```
N 个生产者同时 push：
  每个线程执行 head.exchange(n)
  x86 exchange = LOCK XCHG（有 LOCK 前缀，总线锁）
  
  2 线程：基本无竞争
  4 线程：~2x 延迟
  8 线程：~4-8x 延迟（cache line bounce）
  16 线程：可能比 mutex 更慢
```

CAS/exchange 在 x86 上有 `LOCK` 前缀，触发 cache line 在核间传递。核数越多，bounce 越严重——这就是为什么 MPMC 无锁在高竞争下可能不如有锁。

### MPMC 的替代方案

| 方案 | 思路 | 适用 |
|------|------|------|
| SPSC 拆分 | 每个生产者一个 SPSC 队列，消费者轮询所有 | 生产者数量固定且少 |
| 有锁队列 | `mutex` + `condition_variable` | 高竞争、通用 |
| 分段队列 | 多个队列 + 负载均衡 | 吞吐优先 |
| 批量提交 | 生产者攒一批再 CAS | 减少竞争频率 |

---

## 新手要点（和 C 的区别）

- **C 程序员可能觉得"无锁一定比有锁快"**：这是最大误区。MPMC 无锁在 4+ 线程竞争下，CAS 的 cache line bounce 开销可能超过 mutex（mutex 竞争时线程睡眠，不占 CPU）。要实测。
- **Michael-Scott 队列是经典论文**：1996 年由 Maged Michael 和 Michael Scott 提出，是无锁 MPMC 队列的标准实现。C 程序员如果做过无锁队列，可能见过它的 C 版本。C++ 版本用 `atomic` 更安全。
- **Vyukov 队列用 exchange 代替 CAS**：Dmitry Vyukov 提出的 MPMC 队列变体，用 `exchange`（无失败）代替 CAS（有失败重试），在多生产者下性能更好。C 里要用 `__sync_lock_test_and_set` 实现类似效果。
- **"多生产者拆成多个 SPSC"是重要思路**：C 程序员可能习惯"一个队列大家共用"，但无锁时代拆成多个 SPSC（每生产者一个）可能更快——因为 SPSC 无竞争。消费者轮询多个队列，看似复杂但性能更优。

---

## HFT 关联

- **HFT 极少用真正的 MPMC 无锁队列**：CAS 竞争在 8+ 核系统上太贵。HFT 通常的做法是"每网卡队列一个 SPSC"——多个网卡队列各自有独立的生产者线程和消费者线程，互不干扰。
- **DPDK 的 `rte_ring` 支持 MPMC 但建议避免**：`rte_ring` 有 `rte_ring_mp_enqueue`（多生产者），用 CAS 实现。DPDK 文档明确建议在高性能场景用 SPSC（`rte_ring_sp_enqueue`），把多生产者需求拆成多个 SPSC 环。
- **策略热切换是 MPMC 场景**：多个策略线程可能同时往风控队列提交订单——这是真正的 MPMC。HFT 中这个路径通常用有锁队列（风控不是最热路径）或 SPSC 拆分。
- **批量提交减少 CAS**：如果必须用 MPMC，生产者攒一批（如 16 条）再一次 `exchange`，CAS 频率降为 1/16——HFT 的常见优化。

---

## 自测题

1. SPSC 队列为什么不需要 CAS？MPMC 为什么必须用 CAS 或 exchange？
2. CAS 在高并发（8+ 线程）下为什么会比 mutex 更慢？cache line bounce 是什么？
3. Vyukov 队列用 `exchange` 代替 CAS 有什么优势？
4. "多生产者拆成多个 SPSC"策略为什么可能比单个 MPMC 无锁队列更快？
5. HFT 系统为什么极少用真正的 MPMC 无锁队列？通常怎么替代？

---

## 参考与延伸

- 下一章：[8.1 并行划分策略](../ch08-designing-concurrent-code/01-partitioning.md)
- 上一节：[7.4 无锁队列（SPSC）](04-spsc-queue.md)
- 回到：[第 7 章](README.md)
