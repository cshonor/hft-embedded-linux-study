# Item 31：了解迭代器分类与可用算法

> 第 4 章 迭代器 · Item 31 · 上一节：[Item 30 iostreambuf_iterator](item30-iostreambuf-vs-istream.md)

## 为什么要学这个（先建立直觉）

C 程序员的指针只有一种——随机访问：

```c
int arr[100];
// arr + 3   ← 随机访问，O(1)
// arr++     ← 前进一步
// arr--     ← 后退一步
// qsort(arr, n, sizeof(int), cmp);  // 需要随机访问
```

C++ 的迭代器分五种分类，从弱到强：

| 分类 | 能力 | 典型容器 |
|------|------|----------|
| 输入迭代器 | 只读、单遍、`++` | `istream_iterator` |
| 输出迭代器 | 只写、单遍、`++` | `back_inserter` |
| 前向迭代器 | 读写、多遍、`++` | `forward_list`、`unordered_*` |
| 双向迭代器 | + `--` | `list`、`map`/`set` |
| 随机访问迭代器 | + `[]`、`+`/`-` | `vector`、`deque`、`array` |

算法对迭代器分类有要求：`sort` 要随机访问，`reverse` 要双向。选错编译失败或运行错误。

---

## 这节讲什么

五种迭代器分类决定了可用算法。`sort` 要求随机访问（`list` 不能用 `std::sort`），`reverse` 要求双向。迭代器分类是 STL 算法的约束系统。

---

## 迭代器分类与算法约束

```cpp
// std::sort 要求随机访问迭代器
std::vector<int> v = {3, 1, 4};
std::sort(v.begin(), v.end());  // ✅ vector::iterator 是随机访问

std::list<int> l = {3, 1, 4};
// std::sort(l.begin(), l.end());  // ❌ list::iterator 是双向，不支持 +n
l.sort();  // ✅ list 的成员 sort（归并排序）

// std::reverse 要求双向迭代器
std::reverse(v.begin(), v.end());  // ✅ vector 有双向
std::reverse(l.begin(), l.end());  // ✅ list 也有双向

// std::find 只要求输入迭代器——最弱的分类
auto it = std::find(v.begin(), v.end(), 3);  // ✅ 任何容器都能用
```

### 为什么 list 不能用 std::sort？

```cpp
// std::sort 的实现需要随机访问：
// 1. 快排：选 pivot → 需要随机访问中间元素
// 2. 堆排序：需要下标计算 → 随机访问
// 3. 插入排序：需要 it - begin() → 随机访问

// list::iterator 只有 ++ 和 --，不支持 +n 或 []
// 所以 std::sort(l.begin(), l.end()) 编译失败
// list::sort() 用归并排序（只需要 ++ 和 --）
```

---

## 常见错误（新手踩坑）

### 错误 1：对 list 用 std::sort

```cpp
std::list<int> l = {3, 1, 4};
std::sort(l.begin(), l.end());  // ❌ 编译错误
```

**修正：** `l.sort();`（成员函数，归并排序）

### 错误 2：对 forward_list 用 reverse

```cpp
std::forward_list<int> fl = {1, 2, 3};
// std::reverse(fl.begin(), fl.end());  // ❌ forward_list 是前向迭代器，不支持 --
```

**修正：** `fl.reverse();`（成员函数）

### 错误 3：混淆成员函数和算法

```cpp
std::set<int> s = {3, 1, 4};
std::find(s.begin(), s.end(), 3);  // ⚠️ O(n) 线性查找
s.find(3);  // ✅ O(log n) 利用红黑树结构
```

**修正：** 关联容器用成员 `find`（O(log n)），不要用 `std::find`（O(n)）。

---

## 新手要点（和 C 的区别）

| 维度 | C 指针 | C++ 迭代器 | 为什么 |
|------|--------|-----------|--------|
| 分类 | 只有随机访问 | 五种分类 | 容器能力不同 |
| 算法约束 | 无 | 迭代器分类决定可用算法 | 编译期检查 |
| 排序 | `qsort`（随机访问） | `std::sort`（随机访问）/ `list::sort()`（双向） | 容器特化 |
| 查找 | `bsearch` / 线性 | `std::find`（输入）/ `set::find`（双向+树结构） | 成员更优 |

**一句话：** C 的指针只有随机访问一种。C++ 的迭代器分五种——算法根据迭代器分类选择实现（如 `copy` 对连续内存特化为 `memmove`），容器提供特化的成员函数（如 `list::sort`、`set::find`）。

---

## HFT 关联

- **随机访问迭代器换 `sort`**：`vector` 的随机访问迭代器让 `std::sort` 高效（内省排序）；`list` 只能用成员 `sort()`（归并），性能差——这也是 HFT 选 `vector` 的原因之一。
- **成员 `find` vs `std::find`**：`unordered_map` 查找必须用 `m.find(k)`（O(1)），误用 `std::find` 是 O(n)——热路径性能悬崖。

---

## 代码自测

### Q1: 迭代器分类
```cpp
std::vector<int> v = {3, 1, 4};
std::list<int> l = {3, 1, 4};

std::sort(v.begin(), v.end());     // A
// std::sort(l.begin(), l.end());  // B
l.sort();                           // C
```
> 为什么 B 编译失败？

<details>
<summary>答案</summary>

`std::sort` 要求**随机访问迭代器**（RandomAccessIterator）。`vector::iterator` 是随机访问（支持 `it + n`），`list::iterator` 只是**双向迭代器**（BidirectionalIterator，只支持 `++`/`--`，不支持 `+n`）。

`list::sort()` 是成员函数，利用链表特性（指针重连）实现归并排序，不需要随机访问。
</details>

### Q2: 五种分类
> 列出五种迭代器分类及其能力。

<details>
<summary>答案</summary>

1. **InputIterator** — 只读，单遍，`++`（`istream_iterator`）
2. **OutputIterator** — 只写，单遍，`++`（`back_inserter`）
3. **ForwardIterator** — 读写，多遍，`++`（`forward_list`、`unordered_*`）
4. **BidirectionalIterator** — + `--`（`list`、`map`、`set`）
5. **RandomAccessIterator** — + `[]`、`+`/`-`（`vector`、`deque`、`array`、原生指针）
</details>

### Q3: 成员 vs 算法
```cpp
std::set<int> s = {1, 2, 3, 4, 5};
// A
auto it1 = std::find(s.begin(), s.end(), 3);
// B
auto it2 = s.find(3);
```
> A 和 B 的复杂度分别是什么？

<detailf>
<summary>答案</summary>

- **A**：O(n)。`std::find` 是线性查找，不利用红黑树结构。
- **B**：O(log n)。`set::find` 是成员函数，利用红黑树的有序性做二分查找。

**教训**：关联容器（set/map/unordered_*）的查找/计数用成员函数，不用 `std::find`/`std::count`。
</details>

### Q4: forward_list 限制
```cpp
std::forward_list<int> fl = {1, 2, 3};
// std::reverse(fl.begin(), fl.end());  // A
fl.reverse();                           // B
```
> A 为什么编译失败？

<detailf>
<summary>答案</summary>

`std::reverse` 要求**双向迭代器**（需要 `--` 向后遍历）。`forward_list::iterator` 是**前向迭代器**（只支持 `++`，不支持 `--`）。

`forward_list::reverse()` 是成员函数，利用单向链表的指针重连实现反转（需要遍历两次或用辅助栈）。

**forward_list 是最弱的标准序列容器**——只有前向迭代器，不支持 `size()`、不支持 `--`、不支持随机访问。但它最省内存（每节点一个指针）。
</details>

---

## 参考与延伸

- 上一节：[Item 30 iostreambuf_iterator](item30-iostreambuf-vs-istream.md)
- 回到：[第 4 章 迭代器](README.md)
