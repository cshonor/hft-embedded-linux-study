# Item 25：哈希容器（`unordered_*`）的选择

> 第 3 章 关联容器 · Item 25 · 上一节：[Item 24 insert 效率](item24-insert-efficiency.md)

## 为什么要学这个（先建立直觉）

C 程序员没有标准哈希表——要么手写，要么用第三方（如 uthash）：

```c
// 手写哈希表：桶数组 + 链表
struct Entry { int key; int value; struct Entry* next; };
struct Entry* table[BUCKET_SIZE];
// 查找：hash(key) % BUCKET_SIZE → 遍历链表
```

C++11 引入 `unordered_map`/`unordered_set`，均摊 O(1) 查找：

```cpp
std::unordered_map<int, std::string> m;
m[1] = "hello";  // O(1) 均摊
m.find(1);       // O(1) 均摊
```

但哈希表不是银弹——cache 不友好（桶链表指针追逐），rehash 时有尖峰。

---

## 这节讲什么

`unordered_map`/`unordered_set` 基于哈希表，均摊 O(1) 查找。关键选择：哈希函数、相等判断（用 `==` 而非 `<`）、桶数与负载因子。预 `reserve` 避免 rehash 尖峰。

---

## 哈希容器核心

```cpp
std::unordered_map<std::string, int> m;
m["AAPL"] = 150;
m["GOOG"] = 140;

// 查找 O(1) 均摊
auto it = m.find("AAPL");

// 负载因子 = size / bucket_count
std::cout << m.load_factor() << ' ' << m.max_load_factor();
// 默认 max_load_factor = 1.0，超过就 rehash

// 预分配桶避免 rehash
m.reserve(10000);  // 自动计算需要的桶数
```

### 自定义类型的哈希

```cpp
struct OrderKey {
    int symbol_id;
    int exchange_id;
    bool operator==(const OrderKey& o) const {
        return symbol_id == o.symbol_id && exchange_id == o.exchange_id;
    }
};

namespace std {
    template<> struct hash<OrderKey> {
        size_t operator()(const OrderKey& k) const {
            return hash<int>()(k.symbol_id) ^ (hash<int>()(k.exchange_id) << 1);
        }
    };
}

std::unordered_map<OrderKey, Order> orders;
```

### unordered_map vs map

| 特性 | `map` | `unordered_map` |
|------|-------|----------------|
| 底层结构 | 红黑树 | 哈希表 |
| 查找 | O(log n) | O(1) 均摊 |
| 最坏查找 | O(log n) | O(n)（哈希冲突） |
| 有序性 | ✅ | ❌ |
| 迭代器失效 | 插入不失效 | rehash 全部失效 |
| 用 `==` 还是 `<` | `<`（等价） | `==`（相等） |

---

## 常见错误（新手踩坑）

### 错误 1：自定义类型忘了提供 hash 或 ==

```cpp
struct MyKey { int x, y; };
std::unordered_map<MyKey, int> m;  // 编译错误：没有 hash<MyKey> 和 operator==
```

**修正：** 提供 `operator==` 和特化 `std::hash<MyKey>`。

### 错误 2：热路径 rehash 尖峰

```cpp
std::unordered_map<int, Order> orders;
for (auto& tick : incoming_ticks)
    orders[tick.id] = tick;  // 可能触发 rehash → 延迟尖峰
```

**修正：** `orders.reserve(MAX_ORDERS);` 预分配桶。

### 错误 3：哈希函数质量差导致冲突

```cpp
struct BadHash {
    size_t operator()(int x) const { return x % 10; }  // 只有 10 个桶！
};
std::unordered_set<int, BadHash> s;
for (int i = 0; i < 1000; ++i) s.insert(i);
// 所有元素集中在 10 个桶 → 查找退化到 O(n)
```

**修正：** 用 `std::hash<T>` 默认实现，或写质量好的哈希函数（均匀分布）。

---

## 新手要点（和 C 的区别）

| 维度 | C 手写哈希 | C++ unordered_map | 为什么 |
|------|-----------|-------------------|--------|
| 哈希函数 | 手写 | std::hash + 可自定义 | 标准化 |
| 冲突处理 | 手写（链表/开放寻址） | 标准实现（链表法） | 无需关心 |
| 扩容 | 手动 rehash | 自动 rehash | 自动管理 |
| 相等判断 | 手写 cmp | operator== | 与 set 不同 |
| 预分配 | 手动设桶数 | reserve() | 语义清晰 |

**一句话：** C 的哈希表全手写。C++ 的 `unordered_map` 把哈希函数、冲突处理、rehash 都封装了——你只需提供 `hash` 和 `==`，然后 `reserve` 预分配避免热路径 rehash。

---

## HFT 关联

- **`unordered_map` 的 rehash 尖峰**：订单 ID → 订单对象的映射，启动时按峰值 `reserve`，避免热路径 rehash 导致延迟尖峰。
- **cache 不友好**：哈希表桶链表是指针追逐，cache miss 严重。键密集且范围已知时，`vector` 直接下标更快。
- **`vector` 替代 `unordered_map`**：小整数键（如交易所 ID 0-7）用 `vector<Exchange>` 直接下标，比 `unordered_map<int,Exchange>` 快且 cache 友好。

---

## 代码自测

### Q1: unordered_map 基础
```cpp
std::unordered_map<std::string, int> m;
m["AAPL"] = 150;
m["GOOG"] = 140;
m["MSFT"] = 130;
std::cout << m.bucket_count() << ' ' << m.load_factor();
```
> load_factor 大概是多少？

<details>
<summary>答案</summary>

- `bucket_count()` ≥ 3（通常 ≥ 8，因为桶数通常是质数或 2 的幂）。
- `load_factor()` = `size() / bucket_count()` = `3 / bucket_count`，大约 0.375（如果 bucket_count = 8）。
- `max_load_factor()` 默认 1.0，当 `load_factor > 1.0` 时自动 rehash。
</details>

### Q2: 自定义类型哈希
```cpp
struct Pair { int a, b; };
// 需要什么才能用 unordered_map<Pair, int>？
```

<details>
<summary>答案</summary>

需要两样东西：
1. `operator==`（用于哈希冲突时判断相等）
2. `std::hash<Pair>` 特化（用于计算哈希值）

```cpp
struct Pair {
    int a, b;
    bool operator==(const Pair& o) const { return a==o.a && b==o.b; }
};
namespace std {
    template<> struct hash<Pair> {
        size_t operator()(const Pair& p) const {
            return hash<int>()(p.a) ^ (hash<int>()(p.b) << 1);
        }
    };
}
```
</details>

### Q3: rehash 代价
```cpp
std::unordered_map<int, int> m;
for (int i = 0; i < 100000; ++i) m[i] = i;  // A: 无 reserve

std::unordered_map<int, int> m2;
m2.reserve(100000);  // B: 有 reserve
for (int i = 0; i < 100000; ++i) m2[i] = i;
```
> A 发生了多少次 rehash？B 呢？

<detailf>
<summary>答案</summary>

- **A**：约 10-17 次 rehash（桶数增长策略通常是 2x：1→2→4→...→131072）。每次 rehash = 重新哈希所有已有元素 + 分配新桶数组。
- **B**：0 次 rehash。`reserve(100000)` 一次分配足够桶数。

**HFT**：启动时 `reserve` 避免热路径 rehash 尖峰。
</details>

### Q4: vector 替代 unordered_map
```cpp
// 场景：交易所 ID（0-7）→ Exchange 对象
// A
std::unordered_map<int, Exchange> m;
// B
std::vector<Exchange> v(8);
```
> 哪个更适合？为什么？

<detailf>
<summary>答案</summary>

**B 更适合**。只有 8 个交易所，ID 是 0-7 的小整数：
- `v[id]` 直接下标，O(1) 且零哈希计算。
- 连续存储，cache 友好。
- 无 rehash、无桶开销、无哈希冲突。

`unordered_map` 在小数据集上反而更慢——哈希计算 + 桶查找的常数因子大于直接下标。HFT 经验：键密集且范围已知时，`vector` 永远比 `map`/`unordered_map` 快。
</details>

---

## 参考与延伸

- 上一节：[Item 24 insert 效率](item24-insert-efficiency.md)
- 回到：[第 3 章 关联容器](README.md)
