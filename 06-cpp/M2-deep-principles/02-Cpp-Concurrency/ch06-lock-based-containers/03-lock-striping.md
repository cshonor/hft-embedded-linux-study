# 6.3 锁分段哈希表

> 第 6 章 · 上一节：[6.2 线程安全队列：细粒度锁](02-fine-grained-queue.md) · 下一节：[6.4 设计准则](04-design-guidelines.md)

## 这节讲什么

哈希表天然可以按桶（bucket）分区——不同桶的操作互不干扰。**锁分段（lock striping）**用 N 把锁保护 N 组桶，不同桶组的操作可以完全并行。这是 `ConcurrentHashMap`（Java）和 Intel TBB `concurrent_hash_map` 的核心思想。

---

## 核心规则（代码+表格）

### 单锁哈希表的瓶颈

```cpp
// 单 mutex 保护整个哈希表：所有操作互斥
template <typename K, typename V>
class naive_map {
    std::unordered_map<K, V> data;
    mutable std::mutex m;
public:
    V get(const K& k) { lock_guard lk(m); return data[k]; }
    void set(const K& k, const V& v) { lock_guard lk(m); data[k] = v; }
};
// 100 个线程访问 100 个不同 key，也要排队
```

### 锁分段实现

```cpp
template <typename K, typename V, size_t NumShards = 16>
class striped_map {
    struct shard {
        std::unordered_map<K, V> data;
        mutable std::mutex m;
    };
    std::array<shard, NumShards> shards;

    size_t shard_index(const K& k) const {
        return std::hash<K>{}(k) % NumShards;
    }

public:
    V get(const K& k) const {
        auto& s = shards[shard_index(k)];
        std::lock_guard<std::mutex> lk(s.m);
        auto it = s.data.find(k);
        return it != s.data.end() ? it->second : V{};
    }

    void set(const K& k, const V& v) {
        auto& s = shards[shard_index(k)];
        std::lock_guard<std::mutex> lk(s.m);
        s.data[k] = v;
    }

    // 跨分片的操作需锁住所有分片
    void clear() {
        for (auto& s : shards) {
            std::lock_guard<std::mutex> lk(s.m);
            s.data.clear();
        }
    }
};
```

### 分段数选择

| 分段数 | 并行度 | 内存开销 | 适用 |
|--------|--------|----------|------|
| 1 | 低（等于单锁） | 1 把锁 | 线程少、数据量小 |
| = 核数 | 中 | N 把锁 | 通用推荐 |
| = 桶数 | 高（每桶一锁） | 大 | 超高并发、桶少 |
| >> 桶数 | 无意义浪费 | 大 | 错误 |

### 跨分片操作的锁序

如果需要原子地操作多个 key（如 `swap(k1, k2)`），必须**按固定顺序**锁住多个分片，避免死锁：

```cpp
void swap_values(const K& k1, const K& k2) {
    auto i1 = shard_index(k1);
    auto i2 = shard_index(k2);
    if (i1 == i2) {  // 同一分片，锁一次
        std::lock_guard<std::mutex> lk(shards[i1].m);
        std::swap(shards[i1].data[k1], shards[i1].data[k2]);
    } else {
        // 按索引顺序加锁，避免死锁
        auto& first = i1 < i2 ? shards[i1] : shards[i2];
        auto& second = i1 < i2 ? shards[i2] : shards[i1];
        std::lock_guard<std::mutex> lk1(first.m);
        std::lock_guard<std::mutex> lk2(second.m);
        std::swap(shards[i1].data[k1], shards[i2].data[k2]);
    }
}
// C++17 更简洁：std::scoped_lock lock(shards[i1].m, shards[i2].m);
```

---

## 新手要点（和 C 的区别）

- **C 里通常手写哈希表 + 单锁**：C 程序员很少做分段，因为手写分段哈希表代码量大。C++ 用 `std::array<shard, N>` + `std::mutex` 很简洁。
- **分段数等于核数是经验值**：太少则锁竞争重，太多则浪费内存且 cache 不友好。C 程序员可能习惯一把锁走天下——多核时代这是瓶颈。
- **`std::hash<K>` 是标准库自带的**：C 里要自己写哈希函数。C++ 对内置类型和 `std::string` 有特化，自定义类型可特化 `std::hash`。
- **跨分片操作容易忘记锁序**：C 程序员可能觉得"我分别锁两个分片就行"——但如果两个线程同时 `swap(k1, k2)` 和 `swap(k2, k1)`，不按固定顺序就会死锁。

---

## HFT 关联

- **行情快照表用锁分段**：HFT 维护全市场股票快照，多线程读写。按股票代码 hash 分段（如 64 段），不同股票的更新完全并行。
- **分段数对齐 NUMA**：在 NUMA 架构下，分段数可以等于 NUMA 节点数 × 核数，让锁尽量在本地节点。但 DPDK 通常绑核+网卡队列一一对应，NUMA 感知由部署配置保证。
- **`concurrent_hash_map` 而非自造**：生产环境通常用 Intel TBB 的 `concurrent_hash_map` 而非手写——它已优化了分段数、细粒度读锁、cache 行对齐。这里的手写是为了理解原理。
- **读多写少用 `shared_mutex`**：如果读远多于写，每个分片的锁换成 `std::shared_mutex`（C++17），读用 `shared_lock`、写用 `unique_lock`。

---

## 自测题

1. 锁分段哈希表为什么能比单锁哈希表有更高的并发吞吐？
2. 分段数应该选多少？为什么不能远大于桶数？
3. 跨两个分片的原子操作（如 swap 两个 key 的值）如何避免死锁？
4. `clear()` 需要锁住所有分片，加锁顺序重要吗？为什么？
5. 读多写少的场景，分片的锁应该换成什么类型？

---

## 参考与延伸

- 下一节：[6.4 设计准则](04-design-guidelines.md)
- 上一节：[6.2 线程安全队列：细粒度锁](02-fine-grained-queue.md)
- 回到：[第 6 章](README.md)
