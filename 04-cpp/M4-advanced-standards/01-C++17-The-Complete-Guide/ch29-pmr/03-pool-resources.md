# pool_resource 详解

## unsynchronized_pool_resource

```cpp
#include <memory_resource>

std::pmr::pool_options opts{
    .max_blocks_per_chunk = 100,
    .largest_required_pool_block = 1024  // 最大池块大小
};
std::pmr::unsynchronized_pool_resource pool(opts);

std::pmr::vector<int> v(&pool);
for (int i = 0; i < 1000; ++i) {
    v.push_back(i);  // 从池分配
}
// 分配的块归还到池（free list），下次分配可复用
```

**特点**：
- **块分桶**：按大小分桶（如 16, 32, 64, 128...），每桶一个 free list
- **复用**：`deallocate` 把块放回 free list，下次 `allocate` 同大小直接取
- **单线程**：无线程同步开销，最快
- **适合频繁分配释放**：不像 monotonic 只增不减

## synchronized_pool_resource

```cpp
// 线程安全的池
std::pmr::synchronized_pool_resource sync_pool;

// 多线程安全
std::thread t1([&]() {
    std::pmr::vector<int> v(&sync_pool);
    v.resize(100);
});
std::thread t2([&]() {
    std::pmr::vector<int> v(&sync_pool);
    v.resize(100);
});
// 内部有 mutex 保护——安全但有锁开销
```

## 对比

| 特性 | unsynchronized | synchronized |
|------|---------------|-------------|
| 线程安全 | ❌ | ✅（内部 mutex） |
| 性能 | 最快 | 有锁开销 |
| 适用 | 单线程 | 多线程共享 |
| HFT 热路径 | ✅（每线程独立池） | ❌（有锁） |

## 分桶机制

```
池大小桶（示例）：
Bucket 0: [16 bytes]  → free list: [blk] → [blk] → [blk]
Bucket 1: [32 bytes]  → free list: [blk] → [blk]
Bucket 2: [64 bytes]  → free list: [blk]
...

allocate(20):
  → 找 >= 20 的桶 → Bucket 1 (32 bytes)
  → 从 free list 取一个块，或从上游分配新块

deallocate(p, 20):
  → 找对应桶 → 放回 free list
```

## HFT 应用

```cpp
// 每线程独立 unsynchronized_pool（无锁、无竞争）
thread_local std::pmr::unsynchronized_pool_resource tls_pool;

void on_tick(const Tick& tick) {
    std::pmr::vector<Opportunity> opps(&tls_pool);
    // 策略分析...
    // opps 析构，块归还到 tls_pool
    // 下次 on_tick 复用同样的块——零 malloc
}
```

## 自测题

1. `unsynchronized_pool_resource` 和 `synchronized_pool_resource` 的区别？
2. pool 的分桶机制是什么？为什么这样设计？
3. pool 和 monotonic 的区别？什么时候用 pool？
4. HFT 为什么用 `thread_local` + `unsynchronized_pool`？
5. `pool_options` 的 `largest_required_pool_block` 是什么意思？
