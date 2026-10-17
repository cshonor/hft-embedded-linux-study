# 5.2 set 与 map 的红黑树封装
> 第 5 章 关联容器 · 第 2 节 · 上一节：[5.1 红黑树性质](01-rb-tree-properties.md) · 下一节：[5.3 哈希表开链法](03-hashtable-open-chaining.md)

## 为什么要学这个（先建立直觉）

C 里要实现"键值映射"，你得用数组 + 手写哈希，或者 `tsearch`（不透明、无迭代器）：

```c
// C: 手写键值表或用 tsearch
struct entry { int key; char* value; };
// 红黑树？自己写几百行
// 或者用简单数组 + 线性搜索（O(n)）
```

C++ 的 `set`/`map` 是红黑树的**封装层**——底层一棵 `rb_tree`，外层加一层薄接口：

```cpp
// set = rb_tree（键值合一）
std::set<int> s = {3, 1, 4, 1, 5};
// map = rb_tree<pair<const K, V>>
std::map<int, std::string> m;
m[1] = "one";
```

理解这层封装，你才能理解为什么 `set` 不能存重复值、为什么 `map` 的键是 `const`。

## 这节讲什么

`set`/`map`/`multiset`/`multimap` 都是 `rb_tree` 的封装，区别在于**键值是否合一**和**是否允许重复**。

### RB-tree 的泛型定义

```cpp
// SGI STL 红黑树（简化）
template<class Key, class Value, class KeyOfValue,
         class Compare, class Alloc>
class rb_tree {
    // Key:        键类型
    // Value:      节点存储的类型
    // KeyOfValue: 从 Value 提取 Key 的函数（仿函数）
    // Compare:    键比较函数
};
```

### 四种容器的封装关系

| 容器 | Value 类型 | KeyOfValue | 允许重复 | 键值关系 |
|------|-----------|------------|---------|---------|
| `set<K>` | `K` | `identity<K>` | 否 | 键值合一 |
| `multiset<K>` | `K` | `identity<K>` | 是 | 键值合一 |
| `map<K,V>` | `pair<const K, V>` | `select1st` | 否 | 键值分离 |
| `multimap<K,V>` | `pair<const K, V>` | `select1st` | 是 | 键值分离 |

### set 的封装

```cpp
// set 本质是 rb_tree<T, T, identity<T>, less<T>>
template<class Key, class Compare = less<Key>, class Alloc = allocator<Key>>
class set {
    typedef rb_tree<Key, Key, identity<Key>, Compare, Alloc> rep_type;
    rep_type t;  // 唯一的数据成员
public:
    pair<iterator, bool> insert(const Key& x) { return t.insert_unique(x); }
    iterator find(const Key& x) const { return t.find(x); }
    size_type size() const { return t.size(); }
    // ... 所有接口都是对 t 的转发
};
```

`identity<T>` 返回元素本身作为键——因为 `set` 的元素就是键。

### map 的封装

```cpp
// map 本质是 rb_tree<pair<const K,V>, pair<const K,V>, select1st, less<K>>
template<class Key, class T, class Compare = less<Key>>
class map {
    typedef rb_tree<pair<const Key, T>, pair<const Key, T>,
                    select1st<pair<const Key, T>>, Compare> rep_type;
    rep_type t;
public:
    T& operator[](const Key& k) {
        // 1. lower_bound 查找 k
        // 2. 不存在则 insert(default T)
        // 3. 返回引用
        iterator it = t.lower_bound(k);
        if (it == t.end() || key_comp()(k, it->first))
            it = t.insert_unique(it, value_type(k, T()));
        return (*it).second;
    }
};
```

`select1st` 从 `pair` 中取 `first` 作为键。键是 `const Key` 保证不可修改。

### operator[] 的副作用

```cpp
std::map<int, string> m;
// 只是"查找"，但会创建空字符串！
string& ref = m[42];  // m 现在有 {42: ""}

// 检查是否存在应该用 count 或 find
if (m.count(42)) { ... }     // 不会插入
if (m.find(42) != m.end()) { ... }  // 不会插入
```

## 常见错误（新手踩坑）

### 错误 1：用 operator[] 检查键是否存在

```cpp
// ❌ operator[] 会在键不存在时插入默认值
std::map<int, string> m;
if (m[42].empty()) {  // 误判：m[42] 被创建了！
    cout << "42 不存在";  // 但现在存在了
}
// m.size() 变成了 1
```

用 `find` 或 `count` 检查存在性，用 `operator[]` 获取或设置值。

### 错误 2：multimap 用 operator[]

```cpp
// ❌ multimap 没有 operator[]
std::multimap<int, string> mm;
mm[1] = "a";  // 编译错误！multimap 不能用 []
// 一个键可以有多个值，[] 语义不明确
```

`multimap` 只能用 `insert` 和 `equal_range` 查找。

### 错误 3：以为 set 元素可以修改

```cpp
// ❌ set 的迭代器是 const_iterator
std::set<int> s = {1, 2, 3};
auto it = s.find(2);
*it = 5;  // 编译错误！set 迭代器只读
// 修改值会破坏红黑树性质
```

`set` 的迭代器返回 `const` 引用。要"修改"只能 erase + insert。

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写 BST 或用数组线性搜索 | `set`/`map` 内置红黑树 | C++ 零手写 |
| `void*` 无类型安全 | 模板编译期检查 | C 不安全 |
| 无有序遍历 | 中序遍历有序 | `map` 遍历即排序 |
| 手动管理节点内存 | allocator 自动管理 | C++ RAII |

## HFT 关联

- **订单簿 `map<Price, OrderList>`**：价格档位用 `map` 保持有序，`lower_bound` 快速定位最优买卖价
- **`operator[]` 副作用**：热路径检查键存在性用 `find` 而非 `operator[]`，避免意外插入
- **节点内存**：`map` 每个节点单独堆分配，cache 不友好——HFT 常用 `flat_map`（连续存储 + 排序）替代

## 代码自测

### Q1: set 和 map 的底层 rb_tree 有什么区别？

```cpp
// set
rb_tree<int, int, identity<int>, less<int>>
// map
rb_tree<pair<const int, string>, pair<const int, string>,
        select1st<pair<const int, string>>, less<int>>
```
> Value 类型和 KeyOfValue 有什么不同？为什么 map 的键是 const？

<details>
<summary>答案与复习指引</summary>

**Value 类型**：
- `set`：Value = Key（键值合一），存的就是键本身
- `map`：Value = `pair<const Key, T>`（键值分离），存键值对

**KeyOfValue**：
- `set`：`identity<T>` 返回元素本身
- `map`：`select1st` 从 pair 取 first

**const 键**：红黑树靠键维持有序。如果键可修改，树的结构会被破坏（修改后可能不在正确位置）。所以 `map` 的键是 `const Key`，值可改但键不可改。

**复习：** → [set/map 封装](./02-set-map-as-rb-tree.md)
</details>

### Q2: operator[] 和 insert 的行为差异？

```cpp
std::map<int, int> m;
m[1] = 10;           // 如果 1 不存在，先 insert {1, 0}，再赋值 second=10
m.insert({1, 20});   // 如果 1 已存在，什么都不做（返回 pair<it, false>）

auto [it, ok] = m.insert({2, 30});
// ok = true 如果插入成功，false 如果键已存在
```
> operator[] 和 insert 在键已存在时行为有什么不同？

<details>
<summary>答案与复习指引</summary>

| 操作 | 键不存在 | 键已存在 |
|------|---------|---------|
| `operator[]` | 插入默认值，返回引用 | 返回已有值的引用 |
| `insert` | 插入，返回 `{it, true}` | 不修改，返回 `{existing_it, false}` |

**operator[] 覆盖已有值**：
```cpp
m[1] = 10;  // 插入 {1, 10}
m[1] = 20;  // 1 已存在，返回引用，赋值 20 → 覆盖
```

**insert 不覆盖**：
```cpp
m.insert({1, 10});
m.insert({1, 20});  // 1 已存在，返回 {it, false}，值仍为 10
```

**HFT**：要"不存在才插入"用 `insert` 或 `try_emplace`（C++17），要"获取或创建"用 `operator[]`。

**复习：** → [operator[] 副作用](./02-set-map-as-rb-tree.md)
</details>

### Q3: 下面的代码输出什么？

```cpp
std::map<int, int> m;
m[1] = 100;
m[2] = 200;
m[3] = 300;

int sum = 0;
for (auto it = m.lower_bound(2); it != m.end(); ++it)
    sum += it->second;
std::cout << sum;  // ?
```
> lower_bound(2) 返回什么？遍历到哪里结束？

<details>
<summary>答案与复习指引</summary>

**输出 500**。

`lower_bound(2)` 返回第一个键 ≥ 2 的迭代器，即指向 `{2, 200}`。

遍历到 end()，经过 `{2, 200}` 和 `{3, 300}`，sum = 200 + 300 = 500。

红黑树是有序的，`lower_bound` 利用树结构 O(log n) 定位，然后顺序遍历即可获取所有键 ≥ 2 的元素。

**HFT**：这就是范围查询——"所有价格 ≥ 2 的订单总量"，`lower_bound` 是核心。

**复习：** → [lower_bound](../ch06-algorithms/02-binary-search-algorithms.md)
</details>

### Q4: 为什么 set 没有 operator[]？

```cpp
std::set<int> s = {1, 2, 3};
s[0];  // 编译错误
```
> set 和 map 的元素模型有什么本质区别？

<details>
<summary>答案与复习指引</summary>

**set 的元素就是键本身**（键值合一），没有"值"可以通过 `[]` 获取或设置。

`operator[]` 的语义是"用键取值"——set 没有独立的值，所以没有 `[]`。

如果需要"存在性检查"，用 `count` 或 `find`：
```cpp
if (s.count(2)) { ... }  // O(log n)
if (s.find(2) != s.end()) { ... }  // O(log n)
```

**复习：** → [set 封装](./02-set-map-as-rb-tree.md)
</details>

## 参考与延伸

- 上一节：[5.1 红黑树性质](01-rb-tree-properties.md)
- 下一节：[5.3 哈希表开链法](03-hashtable-open-chaining.md)
- 源码参考：`bits/stl_set.h`、`bits/stl_map.h`（GCC libstdc++）
