# Item 23：降序 `set` / `map`

> 第 3 章 关联容器 · Item 23 · 上一节：[Item 22 operator[] vs insert](item22-operator-vs-insert.md) · 下一节：[Item 24 insert 效率](item24-insert-efficiency.md)

## 为什么要学这个（先建立直觉）

C 程序员控制排序方向通过比较函数：

```c
// 升序
int cmp_asc(const void* a, const void* b) { return *(int*)a - *(int*)b; }
// 降序
int cmp_desc(const void* a, const void* b) { return *(int*)b - *(int*)a; }
qsort(arr, n, sizeof(int), cmp_desc);
```

C++ 的 `set`/`map` 默认升序（`std::less<T>`），换成 `std::greater<T>` 就是降序：

```cpp
std::set<int> asc = {3, 1, 4, 1, 5};                    // {1, 3, 4, 5}
std::set<int, std::greater<int>> desc = {3, 1, 4, 1, 5}; // {5, 4, 3, 1}
```

---

## 这节讲什么

用 `greater<K>` 作比较类型得到降序容器。查找/遍历顺序随之反转。

---

## 降序容器

```cpp
// 降序 set
std::set<int, std::greater<int>> desc = {3, 1, 4, 1, 5};
// 遍历：5, 4, 3, 1（降序）

// 降序 map
std::map<int, std::string, std::greater<int>> m = {{1, "a"}, {3, "c"}, {2, "b"}};
// 遍历：{3, "c"}, {2, "b"}, {1, "a"}（按 key 降序）

// 用 typedef/using 简化
using DescSet = std::set<int, std::greater<int>>;
DescSet s = {3, 1, 4};
```

---

## 常见错误（新手踩坑）

### 错误 1：升序 set 和降序 set 类型不同

```cpp
std::set<int> a = {1, 2, 3};
std::set<int, std::greater<int>> b = {3, 2, 1};
// a = b;  // ❌ 编译错误——类型不同
```

**修正：** 逐元素拷贝，或统一比较类型。

### 错误 2：降序容器的迭代器方向

```cpp
std::set<int, std::greater<int>> s = {1, 2, 3, 4, 5};
// begin() → 5（最大的）
// ++begin() → 4
// ...
// end() → 过去末尾
// 遍历是降序：5, 4, 3, 2, 1
```

**注意：** `begin()` 指向最大值，不是最小值。如果代码假设升序遍历会有逻辑错误。

### 错误 3：lower_bound/upper_bound 在降序容器中的语义

```cpp
std::set<int, std::greater<int>> s = {5, 3, 1};
auto it = s.lower_bound(3);  // 第一个"不大于"3 的位置 → 指向 3
// 注意：lower_bound 的语义是"第一个不满足 comp(elem, val) 的元素"
// comp = greater → "第一个不满足 elem > val 的元素" = 第一个 ≤ val 的元素
```

**修正：** 降序容器的 `lower_bound`/`upper_bound` 语义反转——仔细查阅文档。

---

## 新手要点（和 C 的区别）

| 维度 | C `qsort` | C++ `set<K, Cmp>` | 为什么 |
|------|-----------|-------------------|--------|
| 排序方向 | 比较函数 | 比较类型 | 编译期固定 |
| 切换方向 | 改函数 | 改模板参数 | 类型不同 |
| 遍历方向 | 按 qsort 结果 | 按比较类型 | 升序 or 降序 |

**一句话：** C 的 `qsort` 换比较函数就能改方向。C++ 的 `set` 换比较类型（`greater` vs `less`）改方向——但不同比较类型 = 不同容器类型，不能互相赋值。

---

## HFT 关联

- **降序 set 存卖单价格**：订单簿卖单按价格降序（最高价优先），`set<Price, greater<Price>>` 天然降序遍历。
- **升序 set 存买单价格**：买单按价格升序（最高价在末尾），`set<Price, less<Price>>` 或默认。或者用 `rbegin()` 反向遍历。

---

## 代码自测

### Q1: 降序遍历
```cpp
std::set<int, std::greater<int>> s = {1, 3, 5, 2, 4};
for (auto x : s) std::cout << x << ' ';
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `5 4 3 2 1`。`std::greater<int>` 让 set 按降序排列，`begin()` 指向最大值。
</details>

### Q2: 类型不兼容
```cpp
std::set<int> a = {1, 2, 3};
std::set<int, std::greater<int>> b = {3, 2, 1};
// a = b;  // 编译错误？
```

<details>
<summary>答案</summary>

**是编译错误**。`set<int>` 和 `set<int, greater<int>>` 是不同的类型（第二个模板参数不同）。不能直接赋值。

**修正：** 逐元素拷贝或统一比较类型。
</details>

### Q3: 订单簿场景
```cpp
// 买单：价格越高优先级越高 → 用降序 set 存价格
std::set<double, std::greater<double>> bids;
bids.insert(100.5);
bids.insert(99.0);
bids.insert(101.2);
// 最优买价是？
```

<details>
<summary>答案</summary>

最优买价 = `*bids.begin()` = 101.2（最高价）。降序 set 的 `begin()` 指向最大值（最高买价）。

这是订单簿的经典用法：卖单用升序 set（`begin()` = 最低卖价），买单用降序 set（`begin()` = 最高买价）。
</details>

### Q4: lower_bound 语义
```cpp
std::set<int, std::greater<int>> s = {5, 3, 1};
auto it = s.lower_bound(3);
// it 指向哪个元素？
```

<detailf>
<summary>答案</summary>

`it` 指向 **3**。

降序容器的 `lower_bound(3)` 语义是"第一个不满足 `greater(elem, 3)` 的元素" = "第一个 `elem ≤ 3` 的元素"。降序遍历 `5, 3, 1` 中，第一个 ≤3 的是 3。

**对比升序**：升序 `set<int>` 的 `lower_bound(3)` 是"第一个 ≥3 的元素" = 3。

语义反转，但结果可能相同——关键是理解底层比较逻辑。
</details>

---

## 参考与延伸

- 上一节：[Item 22 operator[] vs insert](item22-operator-vs-insert.md)
- 下一节：[Item 24 insert 效率](item24-insert-efficiency.md)
- 回到：[第 3 章 关联容器](README.md)
