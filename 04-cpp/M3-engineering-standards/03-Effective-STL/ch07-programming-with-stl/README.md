# 第 7 章 使用 STL 编程

**Programming with the STL** — Items 44–50

## 本章讲什么

前 6 章讲"用什么"，本章讲"怎么用得顺"——头文件、`typedef`、成员函数与算法的取舍、错误信息解读。这些是工程实践细节，决定了你用 STL 时的开发效率。

---

## 各 Item 要点

### Item 44：包含正确的头文件

| 用途 | 头文件 |
|------|--------|
| `vector`/`list`/`deque` | `<vector>`/`<list>`/`<deque>` |
| `map`/`set` | `<map>`/`<set>` |
| `unordered_*` | `<unordered_map>`/`<unordered_set>` |
| `sort`/`find`/`copy` 等算法 | `<algorithm>` |
| `numeric` 算法（`accumulate`） | `<numeric>` |
| `function`/`bind` | `<functional>` |
| `string` | `<string>` |

漏含头文件在某些实现上"碰巧能编译"（间接包含），换编译器就报错——显式包含更可移植。

### Item 45：用 `typedef` 简化冗长类型

```cpp
typedef std::map<int, std::unique_ptr<Widget>, PtrCmp> WidgetMap;
WidgetMap m;  // 比 std::map<int,...> 简洁
```

C++11 起 `using` 别名更优（见 Effective Modern C++ Item 9）。`typedef` 让容器类型一处定义、多处复用，换容器只改一处。

### Item 46：区分算法与同名成员函数

有些容器有与算法同名的**成员函数**，成员版更高效（利用容器内部结构）：

| 算法 | 成员函数 | 为什么成员更优 |
|------|----------|----------------|
| `find` | `set::find`/`map::find` | 成员版 O(log n)，算法版 O(n) |
| `count` | `set::count`/`map::count` | 同上 |
| `remove` | `list::remove` | 成员版真删除（链表无需 erase） |
| `sort` | `list::sort` | 成员版归并，算法版不适用链表 |

**规则**：关联容器查找/计数用成员函数；`list` 的删除/排序用成员函数。误用算法版会从 O(log n) 退化到 O(n)。

### Item 47：避免直接修改算法的源码假设

不要依赖算法的具体实现行为（如 `sort` 用的是快排还是内省排序是未指定的）。只依赖标准保证的接口契约——复杂度、副作用、迭代器要求。

### Item 48：`#include` 路径大小写

`<algorithm>` 全小写；`<String>` 在大小写敏感的文件系统（Linux）上找不到。统一用标准小写。

### Item 49：学会解读 STL 错误信息

STL 模板错误信息动辄数百行。读法：**从最内层类型开始**，找第一个不属于标准库的模板实例化点——那通常是你代码出问题的地方。`static_assert` 能在错误信息里植入自定义消息，是定位利器。

### Item 50：熟悉 STL 参考资源

cppreference.com 是最权威的在线参考（含 C++17/20 更新）。读标准库源码（libstdc++/libc++）能深入理解实现。

---

## HFT 关联

- **成员函数 vs 算法**：`unordered_map` 查找必须用 `m.find(k)`（O(1)），误用 `std::find(m.begin(), m.end(), ...)` 是 O(n)——热路径性能悬崖。
- **`using`/`typedef` 固定容器类型**：策略配置容器一处 `using` 定义，换实现（如 `map`→`unordered_map`）只改一处。
- **`static_assert` 植入诊断**：模板化策略接口用 `static_assert` 约束概念，错误信息直接指向误用点，比读 STL 千行报错高效。

---

## 自测题

1. `std::find(m.begin(), m.end(), k)` 和 `m.find(k)` 在 `std::map` 上复杂度分别是什么？
2. `list::remove` 和 `std::remove` 有什么本质区别？
3. 为什么不要依赖 `std::sort` 的具体排序算法（快排/内省）？
4. 解读 STL 模板错误信息时，应该从哪里开始找问题？
5. `#include <String>` 在 Linux 上为什么可能编译失败？

## 代码自测

### Q1: equal_range 用法
```cpp
std::vector<int> v = {1, 2, 2, 2, 3, 4, 5};  // 已排序
auto [lo, hi] = std::equal_range(v.begin(), v.end(), 2);
std::cout << (hi - lo);  // 输出多少？
```
> `equal_range` 返回什么？`hi - lo` 代表什么？

<details>
<summary>答案与复习指引</summary>

`hi - lo` = **3**（值为 2 的元素个数）。

`equal_range` 返回一个迭代器对 `[lo, hi)`：
- `lo` = `lower_bound`（第一个 ≥ 2 的位置）
- `hi` = `upper_bound`（第一个 > 2 的位置）
- `[lo, hi)` 区间内所有元素都等于 2

```
v: [1, 2, 2, 2, 3, 4, 5]
       ^        ^
       lo       hi
```

**用途**：在有序序列中找某值的所有出现。比 `find`（O(n)）高效（O(log n)）。

**复习：** → [equal_range](./README.md)
</details>

### Q2: accumulate vs 普通循环
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// A: accumulate
int sum1 = std::accumulate(v.begin(), v.end(), 0);

// B: 手写循环
int sum2 = 0;
for (auto x : v) sum2 += x;
```
> 两者的结果相同吗？accumulate 的初始值 0 的类型为什么重要？

<details>
<summary>答案与复习指引</summary>

**结果相同**（都是 15）。但初始值类型很重要：

```cpp
std::vector<double> d = {1.5, 2.5, 3.5};
// A: 初始值 0 (int)
auto r1 = std::accumulate(d.begin(), d.end(), 0);      // 结果 6 (int!)，丢失小数
// B: 初始值 0.0 (double)
auto r2 = std::accumulate(d.begin(), d.end(), 0.0);    // 结果 7.5 (double)
```

**原因**：`accumulate` 的返回类型和累加类型由初始值决定。初始值是 `int` → 所有加法用 `int` 运算 → 截断小数。

**教训**：STL 算法的类型推导由参数决定，注意初始值类型匹配。

**复习：** → [accumulate](./README.md)
</details>
