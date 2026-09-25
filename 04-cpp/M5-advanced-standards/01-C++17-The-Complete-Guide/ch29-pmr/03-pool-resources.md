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

<details>
<summary>参考答案</summary>

1. 两者池结构（分桶 + 空闲链表）完全相同，差别只在**线程安全**：
   - `unsynchronized_pool_resource`：**不加锁**，只能被单个线程使用；多个线程同时访问同一个实例是**未定义行为**。因为没有同步开销，速度最快。
   - `synchronized_pool_resource`：**内部有锁**，可被多个线程共享，代价是每次分配/释放都有同步开销（竞争时会急剧放大）。
规则：每个线程一个对象就用 `unsynchronized`，跨线程共享一个池才用 `synchronized`。
2. 分桶（binning）：池内部按**块大小**维护若干「桶」，每个桶对应一种固定块大小，桶内是一条**空闲链表（free list）**；内存按 chunk 向上游批量申请后切分进桶。
分配时找「不小于请求大小的最小桶」，从 free list 取一块（没有就向上游申请新 chunk）；释放时按块大小放回对应桶的 free list。
这样设计是为了：让分配/释放都是 O(1)（无需搜索空闲区）、把碎片限制在块大小粒度内（同类大小的分配反复复用同一批块）、并让同尺寸的分配落在相邻地址上（局部性好）。
3. **pool 支持单块回收**（`deallocate` 把块放回 free list 供后续复用），因此内存可以在长期运行中保持在一个稳态水位；**monotonic 只增不减**（deallocate 是空操作），只适合「一批分配、整体释放」。
代价是 pool 有分桶/链表的管理开销，比 monotonic 略慢。
选用：请求作用域、批处理、一次性构建后用完就丢 → monotonic；长生命周期、频繁分配释放、内存水位要可控 → pool。
4. 两点：**无锁 + 无跨线程竞争**。HFT 通常是「绑核 + 每线程独占数据」的模型，资源本来就不需要跨线程共享；用 `thread_local` 的 `unsynchronized_pool` 就没有 mutex、也不会出现多线程争抢同一条空闲链表导致的 cache line 乒乓。
```cpp
thread_local std::pmr::unsynchronized_pool_resource tls_pool;
void on_tick(const Tick& tick) {
    std::pmr::vector<Opportunity> opps(&tls_pool);
    // opps 析构 → 块归还到本线程的池 → 下一笔 tick 直接复用
}
```
结果是稳态后几乎零 malloc、延迟分布稳定。注意：从该池分配的对象不应跨线程传递或由另一线程释放（资源属于本线程）。
5. 它是 `std::pmr::pool_options` 的一个字段，含义是「**由池直接管理（分桶复用）的最大块大小**」：
   - 请求大小 **不超过** 该值时，由池的某个桶分配，释放后进入 free list 复用；
   - 请求大小 **超过** 该值时，直接转交给上游资源（upstream），不进池。
另一个字段 `max_blocks_per_chunk` 表示向上游一次申请多少个块（即 chunk 粒度）。两者都填 0 表示「由实现自选默认值」。把它设成热路径上最常见的分配尺寸，可以最大化复用率。

</details>
