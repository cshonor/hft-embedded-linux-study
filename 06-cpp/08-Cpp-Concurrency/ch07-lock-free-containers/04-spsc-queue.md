# 7.4 无锁队列（SPSC）

> 第 7 章 · 上一节：[7.3 ABA 问题](03-aba.md) · 下一节：[7.5 MPMC 的复杂度](05-mpmc.md)

## 这节讲什么

单生产者单消费者（SPSC）队列是 HFT 和实时系统最核心的数据结构。因为只有一个生产者和一个消费者，**无需 CAS、无需 ABA 处理**——用两个原子索引 + 环形缓冲即可，性能极高。本节讲它的实现、内存序、以及为什么它是无锁但不是 wait-free 的边界情况。

---

## 核心规则（代码+表格）

### SPSC 环形队列实现

```cpp
template <typename T, size_t Capacity>
class spsc_queue {
    // Capacity 必须是 2 的幂，用 & 代替 %
    static_assert((Capacity & (Capacity - 1)) == 0, "must be power of 2");
    alignas(64) T buffer[Capacity];
    alignas(64) std::atomic<size_t> write_pos{0};  // 生产者写
    alignas(64) std::atomic<size_t> read_pos{0};   // 消费者读

public:
    // 生产者调用（单线程）
    bool push(const T& value) {
        size_t wp = write_pos.load(std::memory_order_relaxed);
        size_t rp = read_pos.load(std::memory_order_acquire);   // 看消费者的进度
        if (wp - rp >= Capacity) return false;  // 满
        buffer[wp & (Capacity - 1)] = value;
        write_pos.store(wp + 1, std::memory_order_release);     // 发布数据
        return true;
    }

    // 消费者调用（单线程）
    bool pop(T& result) {
        size_t rp = read_pos.load(std::memory_order_relaxed);
        size_t wp = write_pos.load(std::memory_order_acquire);  // 看生产者的进度
        if (rp == wp) return false;  // 空
        result = buffer[rp & (Capacity - 1)];
        read_pos.store(rp + 1, std::memory_order_release);      // 发布消费进度
        return true;
    }
};
```

### 内存序精解

| 操作 | 序 | 为什么 |
|------|-----|--------|
| push: load write_pos | `relaxed` | 生产者独占 write_pos，无需同步 |
| push: load read_pos | `acquire` | 必须看到消费者已处理到哪（同步消费者的 release） |
| push: store write_pos | `release` | 发布新写入的数据给消费者 |
| pop: load read_pos | `relaxed` | 消费者独占 read_pos |
| pop: load write_pos | `acquire` | 必须看到生产者写到哪（同步生产者的 release） |
| pop: store read_pos | `release` | 发布消费进度给生产者（让生产者知道可复用 slot） |

关键：**生产者只写 write_pos、读 read_pos；消费者只写 read_pos、读 write_pos**。各自写的变量对方只读——所以写用 `relaxed`（自己独占），读对方写的变量用 `acquire`，发布给对方用 `release`。

### `alignas(64)` 防 false sharing

```cpp
alignas(64) std::atomic<size_t> write_pos;
alignas(64) std::atomic<size_t> read_pos;
```

- `write_pos` 和 `read_pos` 如果在同一 cache line（64 字节），生产者写 `write_pos` 会让消费者的 cache line 失效——即使消费者只读 `read_pos`。
- `alignas(64)` 强制两者在不同 cache line，消除 false sharing。
- 这是 SPSC 队列性能的关键优化——不做的话性能可能下降 5-10 倍。

### SPSC 为什么不需要 CAS

| 条件 | 效果 |
|------|------|
| 生产者只有一个 | `write_pos` 只有一个写者，无需 CAS |
| 消费者只有一个 | `read_pos` 只有一个写者，无需 CAS |
| 生产者读 read_pos | 只读不写，`acquire` 即可 |
| 消费者读 write_pos | 只读不写，`acquire` 即可 |

每个原子变量都只有一个写者——所以 `store` 就够，不需要 CAS。这是 SPSC 比通用队列简单得多的根本原因。

---

## 新手要点（和 C 的区别）

- **C 里也用环形缓冲做 SPSC**：但 C 程序员常犯的错误是用 `volatile` 而非 `_Atomic`——`volatile` 不保证内存序和可见性（见 5.6 节）。C11 的 `_Atomic` 和 C++ 的 `std::atomic` 才是正确的。
- **`alignas(64)` 是 C 程序员陌生的概念**：C 里用 `__attribute__((aligned(64)))`（GCC）或 `__declspec(align(64))`（MSVC）。C++11 的 `alignas` 统一了。false sharing 是多核性能杀手，C 程序员容易忽略。
- **容量必须是 2 的幂**：为了用 `& (Capacity - 1)` 代替 `% Capacity`——取模指令比位与慢 10-20 倍。C 程序员做环形缓冲也知道这个技巧，但容易忘记加 `static_assert`。
- **`memory_order_acquire/release` 的精确使用**：C 程序员可能用 `memory_order_seq_cst`（最安全但最慢）。SPSC 队列用 acquire/release 足够——在 x86 上编译为普通 `mov`，零额外开销。

---

## HFT 关联

- **SPSC 队列是 HFT 的生命线**：网卡线程（生产者）→ 策略线程（消费者）的行情传递，几乎都用 SPSC 环形队列。它是 HFT 系统中最高频的数据通路。
- **DPDK rte_ring 就是 SPSC/MPMC 环形队列**：DPDK 提供的 `rte_ring` 本质就是这个结构，C 实现。理解 C++ 版本有助于理解 DPDK 源码。
- **`alignas(64)` 在 HFT 中是强制要求**：HFT 系统 cache miss 的代价是 ~100ns（L3）到 ~1000ns（内存），false sharing 导致每次 push/pop 都 cache miss——这在纳秒级系统是不可接受的。
- **零拷贝：存指针而非数据**：HFT 中 SPSC 队列通常存指针（指向 mempool 中的行情包），而非拷贝整个行情结构——push/pop 只是 8 字节指针写入，极快。
- **batch pop 提升吞吐**：消费者一次 pop 多条行情批量处理，减少原子操作的频率——HFT 常用优化。

---

## 自测题

1. SPSC 队列为什么不需要 CAS？根本原因是什么？
2. push 的 `load(read_pos)` 为什么用 `acquire`？store(write_pos) 为什么用 `release`？
3. 为什么 `write_pos` 和 `read_pos` 要用 `alignas(64)` 对齐？不这么做会怎样？
4. 容量为什么必须是 2 的幂？用什么代替取模？
5. 为什么 C 程序员用 `volatile` 做 SPSC 队列是错的？

---

## 参考与延伸

- 下一节：[7.5 MPMC 的复杂度](05-mpmc.md)
- 上一节：[7.3 ABA 问题](03-aba.md)
- 回到：[第 7 章](README.md)
