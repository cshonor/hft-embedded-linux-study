# Item 24：`map::insert` 效率 vs `operator[]`

> 第 3 章 关联容器 · Item 24 · 上一节：[Item 23 降序 set/map](item23-descending-set-map.md) · 下一节：[Item 25 哈希容器选择](item25-unordered-containers.md)

## 为什么要学这个（先建立直觉）

C 程序员没有这个问题——数组赋值就是赋值，没有"插入"概念。

C++ 的 `map` 有两种"写入"方式，效率不同：

```cpp
std::map<int, Widget> m;

// operator[]：键不存在 → 默认构造 + 赋值（两次操作）
m[1] = Widget(args);  // 默认构造 Widget → operator= → 析构临时对象

// insert：键不存在 → 直接构造（一次操作）
m.insert({1, Widget(args)});  // 移动/拷贝 Widget 进 map
```

---

## 这节讲什么

更新已有键的值，`m[k] = v` 与 `m.insert_or_assign(k, v)`（C++17）效率接近。插入新键 `insert`/`emplace` 略优（不构造默认值）。批量插入用 `insert(first, last)` 区间版。

---

## 效率对比

```cpp
std::map<int, Widget> m;

// 场景 1：插入新键
m[1] = Widget(args);  // 默认构造 Widget() → operator= → 析构临时 Widget(args)
m.insert({1, Widget(args)});  // 直接构造/移动 Widget(args) 进 map
m.emplace(1, args);  // 最优：直接在 map 内部构造，无临时对象

// 场景 2：更新已有键
m[1] = Widget(args);  // operator=（直接赋值）
m.insert({1, Widget(args)});  // 不更新！insert 不覆盖已有键
m.insert_or_assign(1, Widget(args));  // C++17：插入或更新

// 场景 3：批量插入
std::vector<std::pair<int, Widget>> src;
m.insert(src.begin(), src.end());  // 区间 insert
```

---

## 常见错误（新手踩坑）

### 错误 1：用 insert 更新（不生效）

```cpp
std::map<int, int> m = {{1, 100}};
m.insert({1, 200});  // 键 1 已存在 → 不插入，不更新
// m[1] 仍然是 100！
```

**修正：** `m[1] = 200;` 或 `m.insert_or_assign(1, 200);`。

### 错误 2：用 operator[] 插入大对象（默认构造浪费）

```cpp
std::map<int, BigObject> m;
m[1] = BigObject(lots_of_args);  // 默认构造 BigObject → 赋值 → 析构临时
// 默认构造 + 赋值 + 析构 = 3 次操作
```

**修正：** `m.emplace(1, lots_of_args);` 一次构造，无临时对象。

### 错误 3：循环单元素 insert

```cpp
for (auto& [k, v] : src) m.insert({k, v});  // 每次单独 insert → 每次搜索红黑树
```

**修正：** `m.insert(src.begin(), src.end());` 区间 insert，更高效。

---

## 新手要点（和 C 的区别）

| 维度 | C 数组 | C++ map | 为什么 |
|------|--------|---------|--------|
| 写入 | `arr[i] = v` | `m[k] = v` / `insert` / `emplace` | 多种方式 |
| 插入新键 | N/A | `emplace` 最优 | 无临时对象 |
| 更新已有 | `arr[i] = v` | `m[k] = v` / `insert_or_assign` | insert 不更新 |
| 批量 | 循环 | `insert(first, last)` | 区间更优 |

**一句话：** C 数组赋值只有一种方式。C++ map 有 `[]`/`insert`/`emplace`/`insert_or_assign` 多种写入方式——插入新键用 `emplace`，更新用 `[]`/`insert_or_assign`，批量用区间 `insert`。

---

## HFT 关联

- **`emplace` 省默认构造**：插入新订单用 `m.emplace(orderId, price, qty)`，直接在 map 内部构造 Order，无临时对象。
- **`insert_or_assign` 更新**：C++17 `insert_or_assign` 比 `[]` 语义更清晰——明确表达"插入或更新"。
- **区间 insert 批量加载**：启动时批量加载符号表用 `m.insert(src.begin(), src.end())`，比循环单 insert 更高效。

---

## 代码自测

### Q1: insert 不更新
```cpp
std::map<int, std::string> m = {{1, "old"}};
m.insert({1, "new"});
std::cout << m[1];
```

<details>
<summary>答案</summary>

输出 `old`。`insert` 在键已存在时不插入也不修改已有值。要更新用 `m[1] = "new"` 或 `m.insert_or_assign(1, "new")`。
</details>

### Q2: emplace vs operator[]
```cpp
struct Big {
    int data[1000];
    Big(int x) { data[0] = x; }
};

std::map<int, Big> m;
// A
m[1] = Big(42);
// B
m.emplace(1, 42);
```
> A 和 B 哪个更高效？

<details>
<summary>答案</summary>

**B 更高效**。
- **A**：默认构造 Big → 移动/拷贝 `Big(42)` → 析构临时对象。3 次操作。
- **B**：直接在 map 内部构造 `Big(42)`。1 次操作。

`emplace` 转发参数给构造函数，省去临时对象和默认构造。
</details>

### Q3: insert_or_assign
```cpp
std::map<int, int> m;
m.insert_or_assign(1, 100);  // A: 键不存在
m.insert_or_assign(1, 200);  // B: 键已存在
```
> A 和 B 后 m[1] 分别是什么？返回什么？

<detailf>
<summary>答案</summary>

- **A 后**：m[1] = 100。返回 `{iterator, true}`（插入成功）。
- **B 后**：m[1] = 200。返回 `{iterator, false}`（已存在，更新值）。

`insert_or_assign`（C++17）明确表达"插入或更新"——比 `operator[]` 语义更清晰，且返回值告诉你是插入还是更新。
</details>

### Q4: 区间 insert
```cpp
std::vector<std::pair<int, int>> src = {{1,10}, {2,20}, {3,30}};
std::map<int, int> m = {{2, 99}};

// A: 循环
for (auto& [k, v] : src) m[k] = v;
// B: 区间 insert
m.insert(src.begin(), src.end());
```
> A 和 B 后 m 的内容有什么不同？

<detailf>
<summary>答案</summary>

- **A 后**：m = {{1,10}, {2,20}, {3,30}}。`[]` 会更新已有键（{2,99} → {2,20}）。
- **B 后**：m = {{1,10}, {2,99}, {3,30}}。`insert` 不更新已有键（{2,99} 保留）。

**区别**：`[]` 覆盖已有值，`insert` 不覆盖。根据场景选择——需要更新用 `[]`/`insert_or_assign`，不需要更新用 `insert`。
</details>

---

## 参考与延伸

- 上一节：[Item 23 降序 set/map](item23-descending-set-map.md)
- 下一节：[Item 25 哈希容器选择](item25-unordered-containers.md)
- 回到：[第 3 章 关联容器](README.md)
