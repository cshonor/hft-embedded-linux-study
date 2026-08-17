# 5.4 unordered 容器与自定义哈希
> 第 5 章 关联容器 · 第 4 节 · 上一节：[5.3 哈希表开链法](03-hashtable-open-chaining.md) · 下一节：[第 6 章 算法](../ch06-algorithms/README.md)

## 为什么要学这个（先建立直觉）

C 里要用哈希表，你得引入 `uthash` 或手写——没有标准库支持：

```c
// C: 没有标准哈希表
// 要么用 uthash（侵入式宏），要么手写
// glibc 的 hsearch 不支持删除，功能极其有限
```

C++11 把 SGI 的 `hash_set`/`hash_map` 标准化为 `unordered_set`/`unordered_map`：

```cpp
#include <unordered_map>
std::unordered_map<int, std::string> m;
m[1] = "one";        // O(1) 平均
m.find(1);            // O(1) 平均
// 支持自定义类型，只需提供 hash + equality
```

`unordered_*` 是 `hash_*` 的标准化版本，API 与 `set`/`map` 几乎一致，但底层是哈希表而非红黑树。

## 这节讲什么

C++11 的四个 `unordered_*` 容器是 SGI `hash_*` 的标准版，底层都是开链哈希表。区别在于**键值是否合一**和**是否允许重复**。

### 四种 unordered 容器

| 容器 | 键值 | 重复 | 底层 | 等价 |
|------|------|------|------|------|
| `unordered_set<K>` | 合一 | 否 | hashtable | `hash_set` |
| `unordered_multiset<K>` | 合一 | 是 | hashtable | `hash_multiset` |
| `unordered_map<K,V>` | 分离 | 否 | hashtable | `hash_map` |
| `unordered_multimap<K,V>` | 分离 | 是 | hashtable | `hash_multimap` |

### 模板参数

```cpp
template<
    class Key,
    class Hash = std::hash<Key>,        // 哈希函数
    class KeyEqual = std::equal_to<Key>, // 键相等判断
    class Allocator = std::allocator<Key>
> class unordered_set;

template<
    class Key, class T,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>
> class unordered_map;
```

### std::hash 内置特化

```cpp
// 标准库已为以下类型提供 std::hash 特化：
std::hash<int>{}(42);          // 整数
std::hash<double>{}(3.14);     // 浮点
std::hash<std::string>{}("hi"); // 字符串
std::hash<void*>{}(ptr);       // 指针

// 但没有为自定义类型提供
struct Point { int x, y; };
std::hash<Point>{}(p);  // 编译错误！需要自己特化
```

### 自定义类型的哈希

```cpp
// 方法 1: 传入自定义 Hash 仿函数
struct PointHash {
    size_t operator()(const Point& p) const noexcept {
        size_t h1 = std::hash<int>{}(p.x);
        size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);  // 组合哈希
    }
};

struct PointEqual {
    bool operator()(const Point& a, const Point& b) const noexcept {
        return a.x == b.x && a.y == b.y;
    }
};

std::unordered_map<Point, int, PointHash, PointEqual> m;

// 方法 2: 特化 std::hash（放 namespace std 里）
namespace std {
    template<>
    struct hash<Point> {
        size_t operator()(const Point& p) const noexcept {
            return hash<int>{}(p.x) ^ (hash<int>{}(p.y) << 1);
        }
    };
}
std::unordered_map<Point, int> m;  // 现在可以直接用
```

### 与有序容器的对比

| 特性 | `map`/`set` | `unordered_map`/`unordered_set` |
|------|------------|-------------------------------|
| 底层 | 红黑树 | 哈希表 |
| 查找 | O(log n) | O(1) 平均, O(n) 最坏 |
| 有序遍历 | 是 | 否 |
| 范围查询 | O(log n + k) | 不支持 |
| 迭代器失效 | erase 只失效被删元素 | rehash 全失效 |
| 内存 | 每节点 3 指针 | 桶数组 + 链表节点 |

## 常见错误（新手踩坑）

### 错误 1：rehash 后迭代器全部失效

```cpp
// ❌ unordered_map rehash 导致全量迭代器失效
std::unordered_map<int, int> m;
for (int i = 0; i < 100; ++i) m[i] = i;
auto it = m.begin();
m.reserve(1000);  // rehash！it 失效
// std::cout << it->first;  // 未定义行为
```

与 `map` 不同（`map` 的迭代器只在 erase 被删元素时失效），`unordered_map` 的 rehash 会让所有迭代器失效。

### 错误 2：用 unordered_map 做范围查询

```cpp
// ❌ unordered_map 不支持范围查询
std::unordered_map<int, int> m;
// 想找所有 key >= 10 的元素
auto it = m.lower_bound(10);  // unordered_map 没有 lower_bound！
// 只能遍历全部 O(n)
```

有序容器才有 `lower_bound`/`upper_bound`。需要范围查询用 `map`。

### 错误 3：自定义类型只写 Hash 不写 Equal

```cpp
// ❌ 忘记 KeyEqual，默认用 std::equal_to，对自定义类型不工作
struct Point { int x, y; };
struct PointHash { size_t operator()(const Point& p) const { return p.x ^ p.y; } };
std::unordered_map<Point, int, PointHash> m;
// 隐式使用 std::equal_to<Point>，但 Point 没定义 operator==
// 编译错误或行为错误
```

需要同时提供 Hash 和 Equal（或为类型定义 `operator==`）。

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 无标准哈希表 | `unordered_*` 标准化 | C++11 内置 |
| `uthash` 侵入式 | 非侵入式模板 | C++ 类型安全 |
| 手写哈希函数 | `std::hash` 内置基本类型 | C++ 开箱即用 |
| 手动管理桶 | `reserve` + 自动 rehash | C++ 自动化 |

## HFT 关联

- **启动 reserve**：`unordered_map` 初始化时 `reserve` 最大预估元素数，避免热路径 rehash 延迟尖峰
- **整数键用 identity 哈希**：订单 ID 是整数，`std::hash<int>` 通常返回整数本身（零计算），最快
- **开放寻址替代**：STL 开链法在高负载因子下链表变长 cache 不友好；HFT 用 `rte_hash`（开放寻址 + 线性探测）更 cache 友好
- **有序 vs 哈希选择**：需要范围查询（价格档位）用 `map`，纯键查找（订单 ID → 订单）用 `unordered_map`

## 代码自测

### Q1: unordered_map 和 map 什么时候该用哪个？

```cpp
// 场景 A: 订单 ID → 订单对象（纯查找）
std::unordered_map<OrderId, Order> order_map;

// 场景 B: 价格 → 订单列表（需要有序遍历）
std::map<Price, std::vector<Order>> price_levels;
```
> 选择依据是什么？

<details>
<summary>答案与复习指引</summary>

**选择依据**：

| 需求 | 选择 | 原因 |
|------|------|------|
| 纯键查找，不需要有序 | `unordered_map` | O(1) 平均，比 O(log n) 快 |
| 需要有序遍历或范围查询 | `map` | 红黑树有序 |
| 需要最小/最大键 | `map` | `begin()`/`rbegin()` O(1) |
| 自定义类型无自然序 | `unordered_map` | 不需要 `operator<` |
| 内存受限 | 看情况 | `map` 每节点更大，但 `unordered_map` 桶有额外开销 |

**HFT**：订单 ID 查找用 `unordered_map`（O(1)），价格档位用 `map`（有序 + 范围查询）。

**复习：** → [与有序容器对比](./04-unordered-containers.md)
</details>

### Q2: 自定义类型的哈希怎么写？

```cpp
struct OrderKey {
    uint32_t instrument_id;
    uint32_t sequence;
    // 怎么哈希？
};
```
> 组合多个字段的哈希有什么原则？

<details>
<summary>答案与复习指引</summary>

```cpp
struct OrderKeyHash {
    size_t operator()(const OrderKey& k) const noexcept {
        // 组合哈希：逐字段 XOR + 偏移
        size_t h = std::hash<uint32_t>{}(k.instrument_id);
        h ^= std::hash<uint32_t>{}(k.sequence) + 0x9e3779b9
             + (h << 6) + (h >> 2);  // boost::hash_combine 公式
        return h;
    }
};

struct OrderKeyEqual {
    bool operator()(const OrderKey& a, const OrderKey& b) const noexcept {
        return a.instrument_id == b.instrument_id
            && a.sequence == b.sequence;
    }
};
```

**原则**：
1. 不要简单 XOR（`x ^ y` 对 `(a,b)` 和 `(b,a)` 返回相同哈希）
2. 用 `hash_combine` 公式引入位移和魔法常数
3. 必须同时定义 Equal

**HFT**：如果 `sequence` 全局唯一，可以只用 `sequence` 做哈希键。

**复习：** → [自定义类型的哈希](./04-unordered-containers.md)
</details>

### Q3: 下面的代码哪里有性能陷阱？

```cpp
std::unordered_map<int, Order> orders;
while (running) {
    orders[new_id] = order;   // 插入
    // ... 处理 ...
    orders.erase(filled_id);  // 删除
}
```
> 什么情况下会出现延迟尖峰？

<details>
<summary>答案与复习指引</summary>

**延迟尖峰来源**：插入时触发 rehash。

如果 `orders` 从小到大增长，每超过负载因子阈值就 rehash 一次，O(n) 搬移所有元素。

```cpp
// 修复：启动时预分配
orders.reserve(MAX_ORDERS);  // 永不在热路径 rehash
```

**其他陷阱**：
- `erase` 不会缩容（桶数不减少），长期运行内存只增不减
- 链表节点堆分配（每 insert 一次 `new`），cache 不友好

**HFT**：reserve + 自定义 allocator（内存池分配节点）消除热路径分配。

**复习：** → [rehash 延迟尖峰](./03-hashtable-open-chaining.md)
</details>

### Q4: unordered_map 的迭代器什么时候失效？

```cpp
std::unordered_map<int, int> m;
for (int i = 0; i < 10; ++i) m[i] = i;
auto it1 = m.find(3);
auto it2 = m.find(5);

m.erase(3);    // 删除元素 3
// it1 仍然有效吗？

m[100] = 100;  // 可能触发 rehash
// it2 仍然有效吗？
```
> erase 和 insert 分别对迭代器有什么影响？

<details>
<summary>答案与复习指引</summary>

| 操作 | 失效范围 |
|------|---------|
| `erase(3)` | 仅 `it1`（被删元素的迭代器）失效，`it2` 有效 |
| `insert`（不 rehash） | 无迭代器失效 |
| `insert`（触发 rehash） | **所有**迭代器、引用、指针失效 |
| `reserve` / `rehash` | **所有**迭代器失效 |

所以：
- erase 后 `it1` 失效，`it2` 仍有效
- insert 如果触发 rehash，`it2` 也失效

**安全模式**：
```cpp
// 遍历中删除（C++20）
for (auto it = m.begin(); it != m.end(); ) {
    if (should_erase(it->first))
        it = m.erase(it);  // erase 返回下一个迭代器
    else
        ++it;
}
```

**vs map**：`map` 的 insert 永不失效其他迭代器（只有 erase 失效被删元素），更安全。

**复习：** → [迭代器失效规则](./04-unordered-containers.md)
</details>

## 参考与延伸

- 上一节：[5.3 哈希表开链法](03-hashtable-open-chaining.md)
- 下一节：[第 6 章 算法](../ch06-algorithms/README.md)
- 参考：cppreference `unordered_map`
