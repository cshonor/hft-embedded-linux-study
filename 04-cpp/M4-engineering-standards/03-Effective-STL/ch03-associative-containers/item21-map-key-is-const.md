# Item 21：`map`/`set` 的键是 `const`

> 第 3 章 关联容器 · Item 21 · 上一节：[Item 20 指定比较类型](item20-custom-comparator.md) · 下一节：[Item 22 operator[] vs insert](item22-operator-vs-insert.md)

## 为什么要学这个（先建立直觉）

C 程序员修改数组元素很简单：

```c
struct { int key; int value; } arr[100];
arr[0].key = 42;  // 直接改
```

C++ 的 `map<K,V>` 的键是 `const`——不能直接修改：

```cpp
std::map<int, std::string> m = {{1, "one"}};
// m[1] 的键是 1，不能改成 2
// auto& pair = *m.begin();
// pair.first = 2;  // ❌ 编译错误——键是 const
```

因为修改键会破坏红黑树的有序性——树是根据键的值来组织的，改键 = 破坏不变量。

---

## 这节讲什么

`map<K,V>::value_type` 是 `pair<const K, V>`——键不可修改。想改键只能删旧插新。这是为了维护容器的有序/哈希不变量。

---

## 键的 const 性

```cpp
std::map<int, std::string> m = {{1, "one"}, {2, "two"}};

// value_type 是 pair<const int, string>
for (auto& [key, value] : m) {
    // key = 99;  // ❌ 编译错误——key 是 const
    value = "modified";  // ✅ value 可修改
}

// 想把键从 1 改成 3？
// 不能直接改 → 删旧插新
auto node = m.extract(1);  // C++17：提取节点
node.key() = 3;            // 修改键
m.insert(std::move(node)); // 重新插入
```

### C++17 extract + insert

```cpp
// 旧做法：删除 + 插入（需要重新构造 value）
m.erase(1);
m.insert({3, "one"});

// C++17：extract + 改键 + insert（零拷贝移动）
auto handle = m.extract(1);  // 提取节点，不析构 value
handle.key() = 3;            // 就地改键
m.insert(std::move(handle)); // 重新插入（移动 value，不拷贝）
```

---

## 常见错误（新手踩坑）

### 错误 1：试图通过迭代器修改键

```cpp
std::map<int, std::string> m = {{1, "a"}};
auto it = m.find(1);
// it->first = 2;  // ❌ 编译错误
```

**修正：** 删旧插新，或用 C++17 `extract`。

### 错误 2：假设 set 的元素可修改

```cpp
std::set<int> s = {1, 2, 3};
auto it = s.find(2);
// *it = 99;  // ❌ 编译错误——set 的元素就是键，全是 const
```

**修正：** `s.erase(it); s.insert(99);`

### 错误 3：在 multimap 中改值影响其他元素

```cpp
std::multimap<int, std::string> mm = {{1, "a"}, {1, "b"}};
auto it = mm.find(1);
it->second = "modified";  // ✅ 只改这一个
// 但另一个 {1, "b"} 不受影响
```

**注意：** 值可修改，键不可。multimap 中同一键的多个元素是独立的。

---

## 新手要点（和 C 的区别）

| 维度 | C 数组 | C++ map/set | 为什么 |
|------|--------|-------------|--------|
| 改键 | 直接赋值 | 删旧插新 | 维护有序性 |
| 元素类型 | `struct` | `pair<const K, V>` | 键保护 |
| 性能 | O(1) | O(log n) 删+插 | 红黑树重平衡 |

**一句话：** C 数组的元素随便改。C++ 关联容器的键是 `const`——改键 = 破坏树/哈希不变量。想改键只能删旧插新（C++17 `extract` 可零拷贝改键）。

---

## HFT 关联

- **订单 ID 不可变**：`map<OrderId, Order>` 的 OrderId 是 const——订单 ID 是身份标识，不应改变。需要"改键"时通常是删除旧订单 + 插入新订单。
- **`extract` 零拷贝改键**：C++17 的 `extract` 让改键不拷贝 value——对大 value（如订单对象含多个字段）很高效。

---

## 代码自测

### Q1: 键是 const
```cpp
std::map<int, std::string> m = {{1, "one"}};
auto& ref = *m.begin();
// ref.first = 2;   // A
ref.second = "two";  // B
```
> A 和 B 分别会怎样？

<details>
<summary>答案</summary>

- **A**：编译错误。`map::value_type` 是 `pair<const int, string>`，`first`（键）是 `const`，不可修改。
- **B**：✅ 成功。`second`（值）不是 const，可以修改。
</details>

### Q2: 改键的正确方式
```cpp
std::map<int, std::string> m = {{1, "one"}};
// 把键从 1 改成 3，值不变
// 旧做法
m.erase(1);
m.insert({3, "one"});
// C++17 做法
auto h = m.extract(1);
h.key() = 3;
m.insert(std::move(h));
```
> 旧做法和 C++17 做法有什么区别？

<details>
<summary>答案</summary>

- **旧做法**：删除 {1, "one"}（析构 value）→ 构造 {3, "one"}（拷贝 value）。如果 value 是大对象，拷贝代价高。
- **C++17 做法**：`extract` 提取节点（不析构 value）→ 改键 → `insert` 移动节点（不拷贝 value）。零拷贝。

C++17 `extract` 对大 value 更高效。
</details>

### Q3: set 元素全 const
```cpp
std::set<int> s = {1, 2, 3};
auto it = s.find(2);
*it = 99;  // 编译错误？
```

<details>
<summary>答案</summary>

**是编译错误**。`set<int>` 的元素类型是 `const int`——set 中只有键没有值，键是 const。修改元素 = 修改键 = 破坏有序性。

**修正：** `s.erase(it); s.insert(99);`
</details>

### Q4: multimap 值可改
```cpp
std::multimap<int, std::string> mm = {{1, "a"}, {1, "b"}, {2, "c"}};
auto range = mm.equal_range(1);
for (auto it = range.first; it != range.second; ++it)
    it->second = "x";
// mm 现在的内容？
```

<detailf>
<summary>答案</summary>

mm = {{1, "x"}, {1, "x"}, {2, "c"}}。键 1 的两个元素的值都被改成 "x"。键 2 不受影响。

multimap 中值（`second`）可修改，键（`first`）不可。
</details>

---

## 参考与延伸

- 上一节：[Item 20 指定比较类型](item20-custom-comparator.md)
- 下一节：[Item 22 operator[] vs insert](item22-operator-vs-insert.md)
- 回到：[第 3 章 关联容器](README.md)
