# Item 22：`operator[]` vs `insert` 的取舍

> 第 3 章 关联容器 · Item 22 · 上一节：[Item 21 map 键是 const](item21-map-key-is-const.md) · 下一节：[Item 23 降序 set/map](item23-descending-set-map.md)

## 为什么要学这个（先建立直觉）

C 程序员用数组下标访问：

```c
arr[0] = 42;  // 直接赋值，如果越界 → UB
```

C++ 的 `map::operator[]` 有一个隐藏行为——**键不存在时自动插入默认值**：

```cpp
std::map<int, std::string> m;
m[1] = "hello";  // 键 1 不存在 → 默认构造 string → 赋值 "hello"
m[1] = "world";  // 键 1 已存在 → 赋值 "world"

std::string s = m[99];  // 键 99 不存在 → 默认构造空 string → 返回！
// m 现在有了 {99, ""} 条目！
```

如果你只想查找不想插入，用 `[]` 是错误的——它有副作用。

---

## 这节讲什么

`m[k]`：键不存在则默认构造值并插入，返回引用。`m.insert({k,v})`：不默认构造已有键的值。仅查找不插入应 `find`，避免 `[]` 的默认构造副作用。C++17 `try_emplace` 和 C++20 `contains` 是更安全的替代。

---

## operator[] vs insert vs find

```cpp
std::map<int, Widget> m;

// operator[]：取值或默认构造 + 插入
Widget& w = m[1];  // 键 1 不存在 → 默认构造 Widget → 插入 → 返回引用
// 如果 Widget 默认构造代价高 → 不必要的开销

// insert：插入新键（不构造已有键的值）
m.insert({1, Widget(args)});  // 如果键已存在 → 不插入，不修改已有值

// find：仅查找，不插入
auto it = m.find(1);
if (it != m.end()) {
    auto& w = it->second;  // ✅ 纯查找，无副作用
}

// C++17 try_emplace：键不存在才构造
m.try_emplace(1, args...);  // 如果键已存在 → 不构造，不修改

// C++20 contains：只判断存在性
if (m.contains(1)) { /* 键存在 */ }
```

---

## 常见错误（新手踩坑）

### 错误 1：用 [] 查找导致意外插入

```cpp
std::map<std::string, int> m;
if (m["AAPL"] == 0) {  // 键不存在 → 插入 {"AAPL", 0}！
    // 永远进入这里——因为 [] 刚插入了 0
}
// m.size() 变成 1，即使你只想查找
```

**修正：** `if (m.find("AAPL") != m.end())` 或 `if (m.contains("AAPL"))`。

### 错误 2：[] 的默认构造代价

```cpp
std::map<int, BigObject> m;
m[1];  // 键 1 不存在 → 默认构造 BigObject（可能很重）
// 你可能只想看看键 1 在不在
```

**修正：** `m.find(1)` 或 `m.contains(1)`。

### 错误 3：insert 不更新已有键的值

```cpp
std::map<int, std::string> m = {{1, "old"}};
m.insert({1, "new"});  // 键 1 已存在 → 不插入，不修改！
// m[1] 仍然是 "old"
```

**修正：** `m[1] = "new";` 或 `m.insert_or_assign(1, "new");`（C++17）。

---

## 新手要点（和 C 的区别）

| 维度 | C 数组 | C++ map | 为什么 |
|------|--------|---------|--------|
| 下标访问 | `arr[i]` 直接 | `m[k]` 可能插入 | map 是动态的 |
| 查找 | 线性扫描 | `find(k)` / `contains(k)` | O(log n) |
| 插入 | 无（固定大小） | `insert({k,v})` / `try_emplace` | 动态结构 |
| 副作用 | 无 | `[]` 有插入副作用 | 设计选择 |

**一句话：** C 的数组下标是纯访问（越界才 UB）。C++ 的 `map::operator[]` 有"取值或插入"的双重语义——查找时用 `find`/`contains`，插入时用 `insert`/`try_emplace`，避免 `[]` 的意外副作用。

---

## HFT 关联

- **`[]` 的默认构造副作用**：`m[orderId].qty` 在订单不存在时默认构造一个空订单——可能导致脏数据。查找用 `find`，插入用 `insert`/`try_emplace`。
- **`contains` 快速判断**：C++20 `m.contains(k)` 比 `find(k) != end()` 更简洁，热路径上判断订单是否存在用 `contains`。
- **`try_emplace` 条件构造**：C++17 `try_emplace(k, args...)` 只在键不存在时构造——避免不必要的默认构造开销。

---

## 代码自测

### Q1: operator[] 副作用
```cpp
std::map<std::string, int> m;
int x = m["AAPL"];
std::cout << m.size() << ' ' << x;
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `1 0`。`m["AAPL"]` 在键不存在时默认构造 `int()` = 0 并插入。m 多了一个 `{"AAPL", 0}` 条目，`x` = 0。

**问题**：你可能只想查找，但 `[]` 意外修改了容器。
</details>

### Q2: find vs operator[]
```cpp
std::map<std::string, int> m = {{"AAPL", 150}};

// A: 查找 "GOOG"（不存在）
int a = m["GOOG"];           // A
// B: 查找 "GOOG"（不存在）
auto it = m.find("GOOG");    // B
```
> A 和 B 后 m.size() 分别是多少？

<detailf>
<summary>答案</summary>

- **A 后**：m.size() = 2。`m["GOOG"]` 插入了 `{"GOOG", 0}`。
- **B 后**：m.size() = 1。`find` 不修改容器，只是查找。

**教训**：查找用 `find`/`contains`，不要用 `[]`。
</details>

### Q3: insert 不更新
```cpp
std::map<int, std::string> m = {{1, "old"}, {2, "keep"}};
m.insert({1, "new"});  // A
m[2] = "changed";      // B
```
> A 和 B 后 m[1] 和 m[2] 分别是什么？

<detailf>
<summary>答案</summary>

- **A 后**：m[1] = "old"。`insert` 在键已存在时不插入也不修改。
- **B 后**：m[2] = "changed"。`operator[]` 返回引用，可以修改已有值。

**区别**：`insert` = "只在键不存在时插入"。`[]` = "取值或默认构造 + 返回引用（可修改）"。

要更新已有键：`m[k] = v` 或 `m.insert_or_assign(k, v)`（C++17）。
</details>

### Q4: try_emplace
```cpp
std::map<int, std::string> m = {{1, "existing"}};
m.try_emplace(1, "new value");      // A: 键已存在
m.try_emplace(2, "brand new");      // B: 键不存在
```
> A 和 B 后 m 的内容？

<detailf>
<summary>答案</summary>

- **A**：m[1] 仍然是 "existing"。`try_emplace` 在键已存在时不构造、不修改。
- **B**：m[2] = "brand new"。键不存在 → 构造并插入。

`try_emplace` 的优势：不构造已有键的值。如果构造参数代价高（如 `try_emplace(k, expensive_to_construct)`），`try_emplace` 比 `[]` 更高效。
</details>

---

## 参考与延伸

- 上一节：[Item 21 map 键是 const](item21-map-key-is-const.md)
- 下一节：[Item 23 降序 set/map](item23-descending-set-map.md)
- 回到：[第 3 章 关联容器](README.md)
