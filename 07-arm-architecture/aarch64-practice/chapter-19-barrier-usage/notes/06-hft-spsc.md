# §19.6 HFT 中的屏障使用

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

HFT 的 SPSC（Single Producer Single Consumer）无锁队列使用 release/acquire 内存序配对，编译为 STLR/LDAR，比 mutex 快 10-100 倍。这是 HFT 无锁编程的核心模式。

## 核心要点

### SPSC 无锁队列

```cpp
template<typename T, size_t N>
class SPSCQueue {
    T buffer[N];
    std::atomic<size_t> write_idx{0};
    std::atomic<size_t> read_idx{0};

    void push(const T& val) {
        size_t w = write_idx.load(std::memory_order_relaxed);
        buffer[w % N] = val;
        // release: 保证 buffer 写在 write_idx 更新前可见
        write_idx.store(w + 1, std::memory_order_release);
    }

    bool pop(T& val) {
        size_t r = read_idx.load(std::memory_order_relaxed);
        // acquire: 保证读 write_idx 后再读 buffer
        if (r == write_idx.load(std::memory_order_acquire))
            return false;
        val = buffer[r % N];
        read_idx.store(r + 1, std::memory_order_relaxed);
        return true;
    }
};
```

### 内存序选择

| 操作 | 内存序 | ARM 指令 | 原因 |
|------|--------|----------|------|
| write_idx.store | release | STLR | 保证 buffer 写在 index 更新前可见 |
| write_idx.load (consumer) | acquire | LDAR | 保证读 index 后再读 buffer |
| read_idx.load (producer) | relaxed | LDR | 只需原子性，不需要顺序 |
| read_idx.store | relaxed | STR | 只需原子性 |

### 性能对比

| 方案 | 延迟 | 说明 |
|------|------|------|
| mutex | ~1000-5000ns | 系统调用 + 上下文切换 |
| spinlock | ~50-200ns | 忙等 + DMB |
| SPSC (relaxed/acquire/release) | ~5-20ns | STLR/LDAR |

> **HFT 核心**：SPSC 队列用 release/acquire 配对，编译为 STLR/LDAR，比 mutex 快 10-100 倍。

## HFT 关联

这是 HFT 无锁编程的核心应用。SPSC 队列用于交易线程和 I/O 线程之间的通信——交易线程 push 订单，I/O 线程 pop 并发送到交易所。关键设计点：1) `buffer[w % N]` 用 relaxed 写（不需要屏障），只有 `write_idx.store` 用 release（保证 buffer 写在 index 前）；2) 消费者读 `write_idx` 用 acquire（保证读 index 后再读 buffer）；3) `read_idx` 全用 relaxed（生产者和消费者各自只读写自己的 index，不需要跨核可见性保证——对方通过 acquire/release 间接看到）。这个设计在 A76 上 push+pop 总延迟约 10-20ns。

## 自测题

1. **SPSC 队列中 `write_idx.store` 为什么用 release 而不是 seq_cst？**

<details>
<summary>答案</summary>

`release` 足够保证 `buffer[w % N] = val` 写在 `write_idx.store(w+1)` 之前对消费者可见。`seq_cst`（全序）更强但**不必要**——SPSC 只有一个生产者和一个消费者，不需要全局全序。`release` 编译为 STLR（~5ns），`seq_cst` 编译为 STR + DMB（~10ns），多一倍开销。选最弱的足够内存序是 HFT 性能优化的关键。
</details>

2. **消费者读 `write_idx` 为什么用 acquire？用 relaxed 会怎样？**

<details>
<summary>答案</summary>

`acquire` 保证读 `write_idx` 后再读 `buffer[r % N]`——确保看到生产者写的值。用 `relaxed` 的话，CPU 可能先读 buffer 再读 write_idx（Load-Load 重排），结果读 write_idx 看到新值（r+1）但 buffer 读到旧值。acquire 编译为 LDAR，阻止后续 Load 重排到此 Load 前。这是 SPSC 队列正确性的关键。
</details>

3. **`read_idx` 的 load/store 为什么都用 relaxed？不需要屏障吗？**

<details>
<summary>答案</summary>

`read_idx` 只被消费者写、生产者读。消费者写 `read_idx` 后，生产者通过自己的 `write_idx.store(release)` + 消费者的 `write_idx.load(acquire)` 间接看到消费者的进度——不需要 `read_idx` 自带屏障。`read_idx` 只需要原子性（不被撕裂），不需要顺序保证，所以用 `relaxed` 足够。如果用 acquire/release 反而多余，增加开销。
</details>

## 参考与延伸

- [§19.2 消息传递](02-message-passing.md) — SPSC 的理论基础
- [Ch18 §18.4 Acquire/Release](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — LDAR/STLR 详解
- [Ch20 §20.2 原子操作实现模式](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md) — LDXR/STXR 基础
