# Item 18-19：理解相等与等价的区别

> 第 3 章 关联容器 · Item 18-19 · 下一节：[Item 20 指定比较类型](item20-custom-comparator.md)

## 为什么要学这个（先建立直觉）

C 程序员判断两个值是否"相同"只有一个标准——`==`：

```c
if (a == b) { /* 相同 */ }
```

C++ 的关联容器（`set`/`map`）判断键是否"相同"用的是**等价**（equivalence）而非**相等**（equality）：

```cpp
std::set<int> s = {1, 2, 3};
// set 用 !(a<b) && !(b<a) 判断"等价"——不调用 ==
// 如果 a 和 b 互不小于对方，就认为"等价"（同一个键）
```

这意味着：如果 `operator==` 和 `operator<` 语义不一致，容器认为"等价"的两个对象可能 `==` 为 false——subtle bug 来源。

---

## 这节讲什么

**相等**用 `operator==`；**等价**用比较谓词（默认 `operator<`）判断 `!(a<b) && !(b<a)`。关联容器用等价决定键的唯一性。哈希容器（`unordered_*`）用相等。自定义类型要保证 `<` 与 `==` 语义自洽。

---

## 相等 vs 等价

```cpp
// 相等（equality）：用 operator==
bool equal = (a == b);

// 等价（equivalence）：用比较谓词（默认 operator<）
bool equiv = !(a < b) && !(b < a);

// 关联容器用等价判断键唯一性
std::set<int> s = {1, 2, 3};
s.insert(2);  // 2 与已有 2 等价 → 不插入
s.size();     // 仍然是 3
```

### 不一致的例子

```cpp
// 不区分大小写的字符串比较
struct CaseInsensitiveCmp {
    bool operator()(const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};

std::set<std::string, CaseInsensitiveCmp> s;
s.insert("Hello");
s.insert("HELLO");  // 与 "Hello" 等价（不区分大小写）→ 不插入
s.size();  // 1

// 但 operator== 区分大小写：
std::string("Hello") == std::string("HELLO");  // false！
// "等价"但"不相等"——如果你用 == 查找会找不到
```

---

## 常见错误（新手踩坑）

### 错误 1：自定义类型的 < 和 == 不一致

```cpp
struct Order {
    int id;
    double price;
    bool operator==(const Order& o) const { return id == o.id && price == o.price; }
    bool operator<(const Order& o) const { return id < o.id; }  // 只比较 id
};

std::set<Order> s;
s.insert({1, 100.0});
s.insert({1, 200.0});  // 与 {1, 100.0} 等价（id 相同）→ 不插入！
// 但 {1,100.0} == {1,200.0} 是 false → 逻辑矛盾
```

**修正：** 保证 `<` 和 `==` 语义自洽。如果 `<` 只比 id，`==` 也应该只比 id。

### 错误 2：用 == 在 set 中查找

```cpp
std::set<std::string, CaseInsensitiveCmp> s = {"Hello"};
// s.find("HELLO") 会找到 "Hello"（用等价）
auto it = s.find("HELLO");  // ✅ 找到了
// 但 "Hello" == "HELLO" 是 false
```

**修正：** 关联容器的 `find`/`count`/`contains` 用等价，不是 `==`。理解这一点避免困惑。

### 错误 3：以为 unordered_set 也用等价

```cpp
std::set<int> s;        // 用等价（operator<）
std::unordered_set<int> us;  // 用相等（operator==）！
// 两者判断键唯一性的标准不同
```

**修正：** 有序容器用等价（`<`），无序容器用相等（`==`）。自定义类型要同时提供一致的 `<` 和 `==`。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 判同标准 | `==` 一种 | 相等 `==` + 等价 `<` | 容器设计 |
| 有序容器 | `qsort` + `bsearch` | `set`/`map` 用 `<` | 不需要 `==` |
| 哈希容器 | 无 | `unordered_*` 用 `==` | 哈希冲突判断 |
| 自定义类型 | 手写 `cmp` | 保持 `<` 和 `==` 自洽 | 避免矛盾 |

**一句话：** C 只用 `==`（或自定义 `cmp` 函数）。C++ 关联容器用"等价"（`!(a<b) && !(b<a)`）而非"相等"——这是为了只需一个比较运算符就能维护有序结构。但要保证 `<` 和 `==` 语义自洽。

---

## HFT 关联

- **等价 vs 相等 bug**：自定义订单键的 `operator<` 与 `operator==` 不一致，会导致 `set` 里出现"等价但不相等"的重复——订单去重失效。务必自洽。
- **`unordered_map` 用 `==`**：哈希容器的键判断用 `operator==`，与有序容器不同。如果自定义类型只提供了 `<`，哈希容器编译失败。
- **不区分大小写的 symbol 查找**：用自定义比较的 `set<string, CaseInsensitiveCmp>` 实现 symbol 去重，但注意等价与相等的差异。

---

## 代码自测

### Q1: 等价判断
```cpp
std::set<int> s = {1, 2, 3, 4, 5};
s.insert(3);  // 3 已存在
std::cout << s.size();
```
> 输出多少？set 怎么判断 3 已存在？

<details>
<summary>答案</summary>

输出 **5**。`set` 用等价判断：`!(3 < 3) && !(3 < 3)` = `!false && !false` = `true` → 3 与已有 3 等价 → 不插入。

`set` 的 `insert` 不调用 `operator==`，只用 `operator<`。
</details>

### Q2: 不一致类型
```cpp
struct Point {
    int x, y;
    bool operator<(const Point& p) const { return x < p.x; }  // 只比 x
    bool operator==(const Point& p) const { return x == p.x && y == p.y; }  // 比 x 和 y
};
std::set<Point> s;
s.insert({1, 2});
s.insert({1, 3});  // 会插入吗？
```

<details>
<summary>答案</summary>

**不会插入**。`{1,3}` 与 `{1,2}` 等价（`!({1,3} < {1,2})` = `!(1 < 1)` = `true`，`!({1,2} < {1,3})` = `!(1 < 1)` = `true`）→ 等价 → 不插入。

但 `{1,2} == {1,3}` 是 `false`（y 不同）→ "等价但不相等" → 逻辑矛盾。

**修正：** `<` 只比 x 的话，`==` 也应该只比 x。
</details>

### Q3: set vs unordered_set
```cpp
std::set<int> s;           // A: 用等价（<）
std::unordered_set<int> us; // B: 用相等（==）
// 自定义类型需要提供什么？
```

<detailf>
<summary>答案</summary>

- **A（set）**：需要 `operator<`（用于等价判断和排序）。
- **B（unordered_set）**：需要 `operator==`（用于冲突判断）和 `std::hash<T>` 特化（用于哈希）。

如果自定义类型只提供 `<` 而没有 `==` 和 `hash`，能用于 `set` 但不能用于 `unordered_set`。
</details>

### Q4: 大小写不敏感
```cpp
struct CiCmp {
    bool operator()(const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
std::set<std::string, CiCmp> s = {"Hello", "World"};
s.insert("HELLO");  // A
s.insert("hello");  // B
std::cout << s.size();
```

<detailf>
<summary>答案</summary>

输出 **2**。

- **A**：`"HELLO"` 与 `"Hello"` 等价（`strcasecmp` 返回 0，互不小于）→ 不插入。
- **B**：`"hello"` 与 `"Hello"` 等价 → 不插入。

s = {"Hello", "World"}（或等价的其他大小写变体），size = 2。

这就是"等价"的威力——用自定义比较实现不区分大小写的去重。
</details>

---

## 参考与延伸

- 下一节：[Item 20 指定比较类型](item20-custom-comparator.md)
- 回到：[第 3 章 关联容器](README.md)
