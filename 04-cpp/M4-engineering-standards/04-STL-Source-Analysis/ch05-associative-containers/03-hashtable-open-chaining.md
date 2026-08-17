# 5.3 哈希表的开链法
> 第 5 章 关联容器 · 第 3 节 · 上一节：[5.2 set/map 封装](02-set-map-as-rb-tree.md) · 下一节：[5.4 unordered 容器](04-unordered-containers.md)

## 为什么要学这个（先建立直觉）

C 里哈希表要自己写或用第三方库（如 `uthash`）：

```c
// C: 用 uthash（宏黑魔法，侵入式）
struct order {
    int id;
    uthash_handle hh;  // 侵入式哈希句柄
};
struct order *orders = NULL;
HASH_ADD_INT(orders, id, o);   // 插入
HASH_FIND_INT(orders, &id, o); // 查找
// uthash 用宏实现，调试困难，类型不安全
```

C++ 的 `unordered_map` 内置哈希表，干净且类型安全：

```cpp
std::unordered_map<int, Order> orders;
orders[42] = o;          // 插入 O(1) 平均
auto it = orders.find(42); // 查找 O(1) 平均
```

理解开链法，你才能理解为什么 `unordered_map` 平均 O(1) 但最坏 O(n)、为什么 rehash 会造成延迟尖峰。

## 这节讲什么

SGI STL 哈希表使用**开链法（separate chaining）**处理冲突：每个桶（bucket）是一条链表，哈希到同一桶的元素挂在链上。

### 数据结构

```cpp
// SGI hashtable 的核心结构
template<class Value, class Key, class HashFcn,
         class ExtractKey, class EqualKey, class Alloc>
class hashtable {
    typedef __hashtable_node<Value> node;
    vector<node*, alloc> buckets;  // 桶数组：每个桶指向链表头
    size_type num_elements;         // 元素总数
};

template<class Value>
struct __hashtable_node {
    __hashtable_node* next;  // 链表指针
    Value val;               // 元素值
};
```

### 哈希定位

```cpp
// 哈希函数 → 桶索引
size_type bkt_num(const key_type& key) const {
    return hash(key) % buckets.size();  // 取模定位桶
}

// 查找：定位桶 + 链表遍历
iterator find(const key_type& key) {
    size_type n = bkt_num(key);
    node* first = buckets[n];
    while (first && !equals(key, get_key(first->val)))
        first = first->next;
    return iterator(first, this);
}
```

### 负载因子与 rehash

```cpp
// 负载因子 = 元素数 / 桶数
float load_factor() const { return num_elements / buckets.size(); }

// 超过阈值（默认 1.0）触发 rehash
void resize(size_type num_elements_hint) {
    if (num_elements_hint > buckets.size() * max_load_factor) {
        // 1. 找下一个素数桶数（SGI 用素数减少聚集）
        // 2. 新建桶数组
        // 3. 逐元素重新哈希到新桶
        // 4. 释放旧桶
    }
}
```

### SGI 素数桶数策略

```cpp
// SGI 用 28 个素数（2 → 4294967291）
static const unsigned long prime_list[] = {
    2, 3, 5, 7, 11, 13, 53, 97, 193, 389, 769, 1543,
    3079, 6151, 12289, 24593, 49157, 98317, 196613,
    393241, 786433, 1572869, 3145739, 6291469,
    12582917, 25165843, 50331653, 100663319,
    201326611, 402653189, 805306457, 1610612741,
    3221225473ul, 4294967291ul
};
// 取 >= n 的最小素数作为桶数
```

素数桶数 + 对素数取模 = 减少哈希冲突聚集（对低质量哈希函数更鲁棒）。

## 常见错误（新手踩坑）

### 错误 1：不知道 rehash 导致迭代器失效

```cpp
// ❌ rehash 导致所有迭代器、引用失效
std::unordered_map<int, int> m;
m[1] = 10; m[2] = 20;
auto it = m.find(1);
m[3] = 30;  // 可能触发 rehash
std::cout << it->second;  // 未定义行为！it 可能已失效
```

`unordered_map` 的 rehash 会搬移所有元素到新桶数组，旧的迭代器、指针、引用全部失效。

### 错误 2：以为遍历有序

```cpp
// ❌ unordered_map 遍历顺序不确定
std::unordered_map<int, string> m;
m[3] = "c"; m[1] = "a"; m[2] = "b";
for (auto& [k, v] : m) cout << k;  // 可能是 "312" 或 "213" 等
```

`unordered_map` 遍历顺序取决于哈希函数和桶数，不保证有序。需要有序用 `map`。

### 错误 3：自定义类型没有哈希函数

```cpp
// ❌ 自定义类型直接用 unordered_map 编译失败
struct Point { int x, y; };
std::unordered_map<Point, int> m;  // 错误：没有 std::hash<Point> 特化

// 需要自定义哈希函数
struct PointHash {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};
struct PointEq {
    bool operator()(const Point& a, const Point& b) const {
        return a.x == b.x && a.y == b.y;
    }
};
std::unordered_map<Point, int, PointHash, PointEq> m;  // OK
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| `uthash` 侵入式宏 | `unordered_map` 模板 | C++ 非侵入式，类型安全 |
| 手写 rehash | 自动 rehash | C++ 自动管理 |
| 手动指定桶数 | `reserve` 预分配 | C++ 简洁 |
| 无标准哈希函数 | 内置基本类型哈希 | C++ `std::hash<T>` |

## HFT 关联

- **rehash 延迟尖峰**：`unordered_map` 插入触发 rehash 时一次性搬移所有元素，微秒级延迟。HFT 启动时 `reserve` 足够桶数避免热路径 rehash
- **开链 vs 开放寻址**：STL 用开链（链表），高负载因子下链表变长 cache 不友好。HFT 常用开放寻址 + 线性探测（`rte_hash`），cache 行友好
- **自定义哈希**：订单 ID 常为整数，用 `identity` 哈希（直接返回整数）最快；字符串键用 FNV 或 xxHash

## 代码自测

### Q1: 开链法为什么平均 O(1)？

```cpp
// 查找 = 哈希定位 O(1) + 链表遍历 O(桶长)
// 桶长 = 元素数 / 桶数 = 负载因子
```
> 负载因子为 1 时，平均查找几次比较？最坏情况呢？

<details>
<summary>答案与复习指引</summary>

**负载因子 = 1**：平均每个桶 1 个元素，平均查找 1 次比较。

**最坏**：所有元素哈希到同一桶，退化为链表 O(n)。但好的哈希函数 + rehash 保证桶长接近常数。

**rehash 保证**：负载因子超过阈值（默认 1.0）时自动扩桶，保证平均桶长 ≤ max_load_factor。

**HFT**：reserve 后负载因子 < 1，查找接近 O(1)。

**复习：** → [负载因子与 rehash](./03-hashtable-open-chaining.md)
</details>

### Q2: rehash 什么时候发生？

```cpp
std::unordered_map<int, int> m;
m.max_load_factor();  // 默认 1.0
m.load_factor();      // 元素数 / 桶数

for (int i = 0; i < 100; ++i)
    m[i] = i;  // 什么时候 rehash？
```
> rehash 的触发条件是什么？代价多大？

<details>
<summary>答案与复习指引</summary>

**触发条件**：`num_elements > bucket_count * max_load_factor` 时，在下次 insert 时 rehash。

**代价**：O(n)——逐元素重新哈希到新桶。对 n = 100 万的 map，rehash 可能耗时毫秒级。

**避免**：
```cpp
m.reserve(100);  // 预分配足够桶，不触发 rehash
// 或
m.max_load_factor(0.5);  // 降低负载因子阈值（但桶更多）
```

**HFT**：启动时 reserve 最大预估元素数，热路径永不 rehash。

**复习：** → [rehash 机制](./03-hashtable-open-chaining.md)
</details>

### Q3: 下面的自定义哈希有什么问题？

```cpp
struct BadHash {
    size_t operator()(int x) const {
        return 42;  // 所有元素哈希到同一桶
    }
};
std::unordered_map<int, int, BadHash> m;
for (int i = 0; i < 100; ++i) m[i] = i;
```
> 这会导致什么性能问题？

<details>
<summary>答案与复习指引</summary>

**所有元素都哈希到桶 42**，退化为链表。查找 O(n) 而非 O(1)。

100 个元素全部在同一个桶的链表上，`find(50)` 要遍历 50 个节点。

**好的哈希函数**：
- 均匀分布（不同输入尽量不同输出）
- 雪崩效应（输入小变化导致输出大变化）
- 快速计算

整数可用 `std::hash<int>`（通常返回整数本身），字符串用 `std::hash<string>`。

**HFT**：用 `identity` 哈希整数键（零计算），但确保键分布均匀。

**复习：** → [哈希定位](./03-hashtable-open-chaining.md)
</details>

### Q4: SGI 为什么用素数桶数？

```cpp
// SGI: 桶数 = 素数
// hash % prime 比 hash % power_of_2 更抗聚集
```
> 素数桶数如何减少冲突？

<details>
<summary>答案与复习指引</summary>

**素数取模更抗聚集**：

如果桶数是 2 的幂（如 256），哈希函数输出有周期性模式时（如 `hash = 256*k`），`hash % 256 = 0` 全部聚集到桶 0。

素数取模（如 257）打破这种周期性对齐，即使哈希函数质量不高也能较均匀分布。

**C++11 `unordered_map`**：标准不强制素数桶数。libstdc++ 用素数，MSVC 用 2 的幂（但用更好的哈希函数补偿）。

**HFT**：如果用整数键 + identity 哈希 + 2 的幂桶数，且键有周期性（如 `key = 256*k`），会严重聚集。素数桶数更安全。

**复习：** → [素数桶数策略](./03-hashtable-open-chaining.md)
</details>

## 参考与延伸

- 上一节：[5.2 set/map 封装](02-set-map-as-rb-tree.md)
- 下一节：[5.4 unordered 容器](04-unordered-containers.md)
- 源码参考：`bits/hashtable.h`（GCC libstdc++ 的 `__hashtable` 实现）
