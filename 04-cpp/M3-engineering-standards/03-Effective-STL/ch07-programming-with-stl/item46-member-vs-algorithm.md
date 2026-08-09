# Item 46：区分算法与同名成员函数

> 第 7 章 使用 STL 编程 · Item 46 · 上一节：[Item 45 typedef 简化](item45-typedef-simplify.md) · 下一节：[Item 47 不依赖实现假设](item47-no-implementation-assumptions.md)

## 为什么要学这个（先建立直觉）

在 C 里，没有"容器成员函数"和"通用算法"之分——数组没有成员函数，`qsort`/`bsearch` 是唯一的算法。C++ STL 有同名但不同的两套接口。

```c
/* C: 只有函数，没有成员函数 */
int arr[] = {1, 2, 3, 4, 5};
// 排序：qsort
// 查找：bsearch 或手写循环
// 没有选择——只有一种方式
```

```cpp
// C++: 算法版 vs 成员版
std::set<int> s = {1, 2, 3, 4, 5};

// 成员函数：O(log n) —— 利用红黑树结构
auto it1 = s.find(3);  // 快！

// 算法版：O(n) —— 线性扫描，忽略树结构
auto it2 = std::find(s.begin(), s.end(), 3);  // 慢！
```

**直觉**：关联容器和 list 有专门的成员函数，比同名通用算法更高效。误用算法版会从 O(log n) 退化到 O(n)。

## 这节讲什么

### 成员函数 vs 算法对照表

| 算法（`<algorithm>`） | 成员函数 | 成员更优的原因 | 复杂度对比 |
|----------------------|----------|----------------|-----------|
| `std::find` | `set::find` / `map::find` | 成员版利用树结构 | O(n) vs O(log n) |
| `std::count` | `set::count` / `map::count` | 同上 | O(n) vs O(log n) |
| `std::lower_bound` | `set::lower_bound` | 同上 | O(n) vs O(log n) |
| `std::remove` | `list::remove` | 成员版真删除（不需 erase） | O(n) + erase vs O(n) |
| `std::sort` | `list::sort` | 算法版需要随机访问迭代器，list 没有 | 不可用 vs O(n log n) |
| `std::unique` | `list::unique` | 成员版真删除 | O(n) + erase vs O(n) |
| `std::merge` | `list::merge` | 成员版搬节点不拷贝 | O(n) 拷贝 vs O(n) 搬 |

### 关联容器：必须用成员函数

```cpp
std::map<int, std::string> m = {{1,"a"}, {2,"b"}, {3,"c"}};

// ✅ 正确：成员函数 O(log n)
auto it = m.find(2);

// ❌ 错误：算法 O(n)，完全没用树结构
auto it = std::find(m.begin(), m.end(), 2);  // 编译能过但性能极差
// 而且 find 的比较逻辑不对——它比较的是 pair<const int, string>
// 需要写自定义比较器才编译过，但即使过了也是 O(n)

// 无序容器同理
std::unordered_map<int, std::string> um;
auto it = um.find(2);  // ✅ O(1)
// std::find(um.begin(), um.end(), ...)  // ❌ O(n)
```

### list：sort/remove/unique 必须用成员

```cpp
std::list<int> lst = {3, 1, 4, 1, 5, 9, 2, 6};

// ❌ std::sort 需要 RandomAccessIterator，list 只有 BidirectionalIterator
// std::sort(lst.begin(), lst.end());  // 编译错误！

// ✅ 成员函数：归并排序 O(n log n)
lst.sort();  // {1, 1, 2, 3, 4, 5, 6, 9}

// remove
lst.remove(1);  // 真删除所有 1，O(n)
// vs std::remove + erase：两步，更繁琐
lst.erase(std::remove(lst.begin(), lst.end(), 1), lst.end());
```

### vector/deque：用算法

```cpp
std::vector<int> v = {3, 1, 4, 1, 5};
// vector 没有成员 sort/find/remove —— 只能用算法
std::sort(v.begin(), v.end());
auto it = std::find(v.begin(), v.end(), 4);
v.erase(std::remove(v.begin(), v.end(), 1), v.end());
```

## 常见错误（新手踩坑）

### 错误 1：关联容器用 std::find

```cpp
std::set<int> s = {1, 2, 3, 4, 5};
auto it = std::find(s.begin(), s.end(), 3);  // O(n)！应该用 s.find(3)
```

**修复**：`s.find(3)` — O(log n)。

### 错误 2：list 用 std::sort

```cpp
std::list<int> lst = {3, 1, 4};
std::sort(lst.begin(), lst.end());  // 编译错误：需要 RandomAccessIterator
```

**修复**：`lst.sort()`。

### 错误 3：list 用 erase-remove 而非 remove

```cpp
std::list<int> lst = {1, 2, 3, 1, 4};
lst.erase(std::remove(lst.begin(), lst.end(), 1), lst.end());  // 能用但繁琐
// vs
lst.remove(1);  // 一步到位
```

**修复**：list 用 `lst.remove(val)`。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 查找 | `bsearch`（仅已排序数组） | 成员 `find`（O(log n)）或算法 `std::find`（O(n)） |
| 排序 | `qsort`（数组） | `std::sort`（vector）或 `list::sort`（list） |
| 删除 | 手写循环 | erase-remove（vector）或 `list::remove`（list） |
| 选错代价 | 无选择 | 性能退化 O(log n) → O(n) |

## HFT 关联

- **unordered_map 查找必须用成员 find**：`m.find(k)` O(1) vs `std::find` O(n)，热路径性能悬崖
- **list 排序用成员 sort**：如果用链表做订单队列（少见但可能），`lst.sort()` 是唯一选择
- **性能审计**：code review 时检查关联容器是否误用 `std::find`/`std::count`

## 代码自测

### Q1: set 查找

```cpp
std::set<int> s = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
// A:
auto it1 = s.find(5);
// B:
auto it2 = std::find(s.begin(), s.end(), 5);
```
> A 和 B 的复杂度分别是什么？

<details>
<summary>答案</summary>

- **A `s.find(5)`**：O(log n) — 红黑树查找
- **B `std::find(...)`**：O(n) — 线性扫描

对于 10 个元素的 set 差距不大，但对于百万级 set，A 是 20 次比较，B 是百万次。

**规则**：关联容器（set/map/multiset/multimap）和 unordered_* 的查找/计数必须用成员函数。
</details>

### Q2: list sort

```cpp
std::list<int> lst = {3, 1, 4, 1, 5};
std::sort(lst.begin(), lst.end());  // 结果？
```

<details>
<summary>答案</summary>

**编译错误**。`std::sort` 要求 RandomAccessIterator，`std::list` 只提供 BidirectionalIterator。

**修复**：`lst.sort()` — 成员函数用归并排序，O(n log n)。

```cpp
lst.sort();  // 正确
// 降序：lst.sort(std::greater<int>());
```
</details>

### Q3: unordered_map count

```cpp
std::unordered_map<int, std::string> m = {{1,"a"}, {2,"b"}};
// 统计键 2 出现几次
size_t c1 = m.count(2);                        // A
size_t c2 = std::count(m.begin(), m.end(), 2); // B（可能编译失败）
```
> A 和 B 的复杂度？B 能编译吗？

<details>
<summary>答案</summary>

- **A `m.count(2)`**：O(1) 平均 — 哈希查找
- **B `std::count(...)`**：O(n) — 而且很可能**编译失败**

B 编译失败的原因：`m` 的元素类型是 `pair<const int, string>`，`std::count` 比较的是 pair 和 int（2），类型不匹配。

即使写自定义比较器让它编译过，复杂度仍是 O(n)。

**规则**：unordered 容器用成员函数。
</details>

### Q4: list remove

```cpp
std::list<int> lst = {1, 2, 3, 1, 4, 1, 5};
// 删除所有 1
// A:
lst.remove(1);
// B:
lst.erase(std::remove(lst.begin(), lst.end(), 1), lst.end());
```
> A 和 B 哪个更好？为什么？

<details>
<summary>答案</summary>

**A 更好**。

- **A `lst.remove(1)`**：一步完成，直接删除匹配节点，O(n)
- **B erase-remove**：两步——先 `remove` 搬移（但不释放节点），再 `erase` 删除。功能相同但更繁琐

`list::remove` 是专门为链表设计的——直接摘除节点，不需要搬移元素（链表元素不连续存储）。

**规则**：list 有同名成员函数时优先用成员版。
</details>

## 参考与延伸

- 上一节：[Item 45 typedef 简化](item45-typedef-simplify.md)
- 下一节：[Item 47 不依赖实现假设](item47-no-implementation-assumptions.md)
