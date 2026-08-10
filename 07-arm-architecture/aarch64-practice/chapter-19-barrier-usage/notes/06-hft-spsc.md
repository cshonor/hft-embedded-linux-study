# §19.6 HFT 中的屏障使用

> **来源：** [Ch19 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

HFT 的 SPSC（Single Producer Single Consumer）无锁队列使用 release/acquire 内存序配对，编译为 STLR/LDAR，比 mutex 快 10-100 倍。本节给出完整实现代码、内存序选择分析和性能对比。

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

### 内存序选择详解

| 操作 | 内存序 | ARM 指令 | 原因 |
|------|--------|----------|------|
| `write_idx.store` | release | STLR | 保证 buffer 写在 index 更新前可见 |
| `write_idx.load` (consumer) | acquire | LDAR | 保证读 index 后再读 buffer |
| `read_idx.load` (producer) | relaxed | LDR | 只需原子性，不需要顺序 |
| `read_idx.store` | relaxed | STR | 只需原子性 |
| `write_idx.load` (producer) | relaxed | LDR | 只需原子性 |

### 为什么 read_idx 用 relaxed？

```
消费者写 read_idx → 生产者读 read_idx

生产者通过 write_idx.store(release) + 消费者 write_idx.load(acquire)
  → 消费者看到生产者的 buffer 写入

消费者写 read_idx → 但生产者只读 read_idx 判断队列是否满
  → 如果满，生产者等待；如果不满，继续 push
  → read_idx 的可见性不影响数据正确性（只影响流量控制）
  → relaxed 足够
```

### 性能对比

| 方案 | 延迟 | 说明 | 适用场景 |
|------|------|------|---------|
| mutex | ~1000-5000ns | 系统调用 + 上下文切换 | 通用 |
| spinlock | ~50-200ns | 忙等 + DMB | 短临界区 |
| CAS 循环 | ~20-100ns | LDXR/STXR 循环 | 低竞争 MPSC |
| SPSC (relaxed/acquire/release) | ~5-20ns | STLR/LDAR | HFT 首选 |
| SPSC + LSE | ~3-10ns | STLXR/LDAXR | ARMv8.1+ |

> **HFT 核心**：SPSC 队列用 release/acquire 配对，编译为 STLR/LDAR，比 mutex 快 10-100 倍。

### 多核扩展

```cpp
// MPSC（多生产者单消费者）需要 CAS
template<typename T, size_t N>
class MPSCQueue {
    T buffer[N];
    std::atomic<size_t> head{0};  // 生产者竞争
    size_t tail{0};               // 消费者独占

    bool push(const T& val) {
        size_t h = head.load(std::memory_order_relaxed);
        size_t next = (h + 1) % N;
        // CAS：竞争 head
        while (!head.compare_exchange_weak(h, next,
                    std::memory_order_acq_rel)) {
            h = head.load(std::memory_order_relaxed);
            next = (h + 1) % N;
            if (next == tail) return false;  // 满
        }
        buffer[h] = val;
        // 需要 release 保证 buffer 写在 head 更新后可见
        // CAS 已自带 acq_rel
        return true;
    }
};
// CAS 在高竞争时性能下降，SPSC 无竞争更好
```

## HFT 关联

这是 HFT 无锁编程的核心应用。SPSC 队列用于交易线程和 I/O 线程之间的通信——交易线程 push 订单，I/O 线程 pop 并发送到交易所。

### HFT 设计要点

1. `buffer[w % N]` 用 relaxed 写（不需要屏障），只有 `write_idx.store` 用 release
2. 消费者读 `write_idx` 用 acquire（保证读 index 后再读 buffer）
3. `read_idx` 全用 relaxed（生产者和消费者各自只读写自己的 index，通过 acquire/release 间接看到对方进度）

这个设计在 A76 上 push+pop 总延迟约 10-20ns。

### HFT 实际应用

```cpp
// 交易线程（生产者）
void trading_thread() {
    while (running) {
        Order ord = get_order_signal();
        spsc_queue.push(ord);  // ~5-10ns (STLR)
    }
}

// I/O 线程（消费者）
void io_thread() {
    while (running) {
        Order ord;
        if (spsc_queue.pop(ord)) {  // ~5-10ns (LDAR)
            send_to_exchange(ord);
        } else {
            cpu_relax();  // 空队列，短暂等待
        }
    }
}

// 绑核避免进程切换
cpu_set_t cs;
CPU_ZERO(&cs);
CPU_SET(0, &cs);  // 交易线程在核 0
pthread_setaffinity_np(trading_thread, sizeof(cs), &cs);
CPU_ZERO(&cs);
CPU_SET(1, &cs);  // I/O 线程在核 1
pthread_setaffinity_np(io_thread, sizeof(cs), &cs);
```

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

4. **SPSC 队列为什么比 MPSC（多生产者）快？**

<details>
<summary>答案</summary>

SPSC 无竞争——生产者只有一个，消费者只有一个，不需要 CAS：
- SPSC push：`buffer[w] = val` + `STLR write_idx`（无重试，~5-10ns）
- MPSC push：`CAS(head)` 循环（高竞争时反复失败重试，~20-100ns）

MPSC 需要多个生产者竞争同一个 head 指针，CAS 在高竞争时性能急剧下降（livelock）。SPSC 完全避免了竞争，是最快的无锁通信模式。
</details>

## 参考与延伸

- [§19.2 消息传递](02-message-passing.md) — SPSC 的理论基础
- [Ch18 §18.4 Acquire/Release](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — LDAR/STLR 详解
- [Ch20 §20.2 原子操作实现模式](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md) — LDXR/STXR 基础
