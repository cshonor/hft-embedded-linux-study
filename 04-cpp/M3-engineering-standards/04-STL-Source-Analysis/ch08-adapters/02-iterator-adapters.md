# 8.2 迭代器适配器
> 第 8 章 适配器 · 第 2 节 · 上一节：[8.1 容器适配器](01-container-adapters.md) · 下一节：[8.3 函数适配器](03-function-adapters.md)

## 为什么要学这个（先建立直觉）

C 里要"反向遍历数组"或"往容器尾插"，你得手写下标逻辑：

```c
// C: 反向遍历
for (int i = n - 1; i >= 0; --i)
    process(arr[i]);

// C: 手动管理插入位置
int pos = 0;
for (int i = 0; i < n; ++i)
    dst[pos++] = src[i];  // 手动跟踪位置
```

C++ 的迭代器适配器把"反向"/"尾插"/"流"等模式封装为迭代器：

```cpp
// 反向遍历
for (auto it = v.rbegin(); it != v.rend(); ++it)
    process(*it);

// 尾插（自动扩容）
std::copy(src.begin(), src.end(), std::back_inserter(dst));
```

理解迭代器适配器，你才能写出更简洁、更泛型的代码。

## 这节讲什么

迭代器适配器包装普通迭代器，改变其行为——反向、插入、流。

### reverse_iterator

```cpp
// 包装普通迭代器，反向遍历
template<class Iterator>
class reverse_iterator {
protected:
    Iterator current;  // 被包装的迭代器
public:
    reverse_iterator(Iterator x) : current(x) {}

    // * 解引用：访问 current 的前一个元素
    reference operator*() const {
        Iterator tmp = current;
        return *--tmp;  // 先减再解引用
    }
    // ++ 变成 --（反向）
    reverse_iterator& operator++() { --current; return *this; }
    reverse_iterator& operator--() { ++current; return *this; }
};
```

`rbegin()` 返回 `reverse_iterator(end())`，`rend()` 返回 `reverse_iterator(begin())`。

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
// rbegin 指向 5，rend 指向 1 的前面
for (auto it = v.rbegin(); it != v.rend(); ++it)
    std::cout << *it << " ";  // 5 4 3 2 1
```

### 插入迭代器

```cpp
// back_insert_iterator: *it = x 变成 push_back(x)
template<class Container>
class back_insert_iterator {
protected:
    Container* container;
public:
    back_insert_iterator(Container& c) : container(&c) {}
    back_insert_iterator& operator=(const typename Container::value_type& x) {
        container->push_back(x);  // 赋值变尾插
        return *this;
    }
    // * 和 ++ 是空操作（no-op）
    back_insert_iterator& operator*() { return *this; }
    back_insert_iterator& operator++() { return *this; }
};
```

| 适配器 | `*it = x` 变成 | 要求 |
|--------|---------------|------|
| `back_insert_iterator` | `c.push_back(x)` | 有 `push_back` |
| `front_insert_iterator` | `c.push_front(x)` | 有 `push_front` |
| `insert_iterator` | `c.insert(pos, x)` | 有 `insert` |

```cpp
std::vector<int> src = {1, 2, 3};
std::vector<int> dst;

// back_inserter: 自动 push_back
std::copy(src.begin(), src.end(), std::back_inserter(dst));
// dst = {1, 2, 3}

// front_inserter 需要 push_front（list/deque）
std::list<int> lst;
std::copy(src.begin(), src.end(), std::front_inserter(lst));
// lst = {3, 2, 1}（头插是逆序）

// inserter: 在指定位置插入
std::vector<int> dst2 = {0, 0};
auto it = dst2.begin();
std::copy(src.begin(), src.end(), std::inserter(dst2, it + 1));
// dst2 = {0, 1, 2, 3, 0}
```

### 流迭代器

```cpp
// istream_iterator: 从输入流读取
std::istringstream iss("1 2 3 4 5");
std::istream_iterator<int> begin(iss);
std::istream_iterator<int> end;  // 默认构造 = 流结束

std::vector<int> v(begin, end);  // v = {1, 2, 3, 4, 5}

// ostream_iterator: 向输出流写入
std::ostream_iterator<int> out(std::cout, ", ");
std::copy(v.begin(), v.end(), out);  // 输出 "1, 2, 3, 4, 5, "
```

流迭代器让流可以像容器一样参与 STL 算法。

### back_inserter 是最常用的

```cpp
// 最常见的用法：copy + back_inserter
std::vector<int> src = {1, 2, 3, 4, 5};
std::vector<int> dst;

// 方法 A: back_inserter（简单但可能多次扩容）
std::copy(src.begin(), src.end(), std::back_inserter(dst));

// 方法 B: 预分配 + copy（更高效）
dst.resize(src.size());
std::copy(src.begin(), src.end(), dst.begin());

// 方法 C: 直接构造（最高效）
std::vector<int> dst2(src.begin(), src.end());
```

## 常见错误（新手踩坑）

### 错误 1：对 vector 用 front_inserter

```cpp
// ❌ vector 没有 push_front
std::vector<int> v;
std::front_inserter(v);  // 编译错误！
// front_insert_iterator 调用 push_front，vector 没有此方法
// 用 list 或 deque
```

### 错误 2：back_inserter 每元素扩容

```cpp
// ❌ 可能多次扩容（虽然 vector 有扩容因子，但不是最优）
std::vector<int> dst;
std::copy(src.begin(), src.end(), std::back_inserter(dst));
// dst 从 0 开始，push_back 多次扩容

// ✅ 先 reserve 再 back_inserter
dst.reserve(src.size());
std::copy(src.begin(), src.end(), std::back_inserter(dst));
// 或直接 resize + copy
```

### 错误 3：反向迭代器和正向迭代器混用

```cpp
// ❌ rbegin() 和 begin() 类型不同
std::vector<int> v = {1, 2, 3};
auto fwd = v.begin();
auto rev = v.rbegin();
// fwd 和 rev 类型不同，不能直接比较或混用
// std::find(v.begin(), v.rbegin(), 2);  // 编译错误
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写反向下标 | `rbegin`/`rend` | C++ 泛型 |
| 手动跟踪插入位置 | `back_inserter`/`inserter` | C++ 自动 |
| 手动解析流 | `istream_iterator` | C++ 流当容器 |
| 无泛型 | 适配器适配任何容器 | C++ 通用 |

## HFT 关联

- **back_inserter 自动扩容**：方便但 HFT 热路径先 `reserve` 再用——避免 `push_back` 多次扩容
- **reverse_iterator**：回测中反向遍历历史数据（从最新到最旧）用 `rbegin`/`rend` 简洁
- **流迭代器**：回测引擎读 CSV 数据用 `istream_iterator` 让流参与 STL 算法（但热路径用二进制 + `memcpy`）
- **insert_iterator**：在有序 `vector` 中间插入用 `inserter` + `lower_bound`，但热路径避免中间插入（O(n) 移动）

## 代码自测

### Q1: back_inserter 如何工作？

```cpp
std::vector<int> src = {1, 2, 3};
std::vector<int> dst;
std::copy(src.begin(), src.end(), std::back_inserter(dst));
// dst = {1, 2, 3}
```
> back_inserter 把 `*it = x` 变成了什么操作？

<details>
<summary>答案与复习指引</summary>

`back_inserter(dst)` 返回 `back_insert_iterator<vector<int>>`，它的 `operator=` 被重载为调用 `dst.push_back(x)`：

```cpp
back_insert_iterator& operator=(const int& x) {
    container->push_back(x);  // 赋值变尾插
    return *this;
}
// *it 和 ++it 是空操作（no-op），让 copy 的循环能编译
```

**copy 内部循环**：
```cpp
while (first != last) {
    *result = *first;  // → dst.push_back(*first)
    ++first;
    ++result;          // no-op
}
```

**效果**：逐元素 `push_back`，自动扩容。

**HFT**：先 `reserve` 再 `back_inserter` 避免多次扩容。或用 `dst.insert(dst.end(), src.begin(), src.end())` 区间版。

**复习：** → [插入迭代器](./02-iterator-adapters.md)
</details>

### Q2: reverse_iterator 的 `*` 返回什么？

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto rit = v.rbegin();  // 包装 end()
std::cout << *rit;  // ?
```
> rbegin() 包装的是哪个迭代器？解引用为什么返回 5 而不是越界？

<details>
<summary>答案与复习指引</summary>

**输出 5**。

`rbegin()` = `reverse_iterator(end())`。`end()` 指向最后一个元素**之后**的位置。

**解引用逻辑**：`reverse_iterator::operator*()` 不是直接解引用 `current`，而是先减一再解引用：

```cpp
reference operator*() const {
    Iterator tmp = current;
    return *--tmp;  // 先减，返回前一个元素
}
```

所以 `rbegin()`（包装 `end()`）解引用返回 `*(end() - 1)` = 最后一个元素 = 5。

**对称设计**：
- `begin()` 指向第一个元素，`end()` 指向最后一个之后
- `rbegin()` 指向最后一个元素，`rend()` 指向第一个之前
- `[begin, end)` = `[rbegin, rend)` 的逆序

**复习：** → [reverse_iterator](./02-iterator-adapters.md)
</details>

### Q3: front_inserter 为什么对 vector 编译失败？

```cpp
std::vector<int> v;
auto it = std::front_inserter(v);  // 编译错误
// std::list<int> l;
// auto it = std::front_inserter(l);  // OK
```
> front_insert_iterator 调用什么方法？vector 为什么不满足？

<details>
<summary>答案与复习指引</summary>

`front_insert_iterator::operator=` 调用 `container->push_front(x)`。

`vector` 没有 `push_front`——头部插入需要移动所有元素 O(n)，STL 不提供这个低效操作。

**满足 `push_front` 的容器**：`deque`、`list`、`forward_list`。

**如果需要在 vector 头部插入**：
```cpp
v.insert(v.begin(), x);  // O(n)，但不推荐
// 或用 deque 替代（如果频繁头插）
```

**HFT**：频繁头插用 `deque` 或 `list`，不用 `vector`。

**复习：** → [插入迭代器](./02-iterator-adapters.md)
</details>

### Q4: istream_iterator 如何让流参与算法？

```cpp
std::istringstream iss("3 1 4 1 5 9 2 6");
std::istream_iterator<int> begin(iss);
std::istream_iterator<int> end;

std::vector<int> v(begin, end);
std::sort(v.begin(), v.end());
// v = {1, 1, 2, 3, 4, 5, 6, 9}
```
> istream_iterator 的"结束"迭代器是什么？如何判断流结束？

<details>
<summary>答案与复习指引</summary>

**默认构造的 `istream_iterator<int>()`** 是"流结束"标记（类似 `end()`）。

**工作原理**：
- `istream_iterator<int>(iss)` 从流读取第一个 `int`，缓存
- `++it` 读取下一个 `int`
- 当流到达 EOF 或遇到非 `int` 输入，迭代器变成默认构造状态（等于 `end`）
- `begin == end` 时停止

**让流参与算法**：
```cpp
// 从 cin 读取整数直到 EOF，求和
auto sum = std::accumulate(
    std::istream_iterator<int>(std::cin),
    std::istream_iterator<int>(),
    0);
```

**HFT**：回测引擎读 CSV 用 `istream_iterator` 很方便，但热路径用二进制格式 + `memcpy`（快 10x+）。

**复习：** → [流迭代器](./02-iterator-adapters.md)
</details>

## 参考与延伸

- 上一节：[8.1 容器适配器](01-container-adapters.md)
- 下一节：[8.3 函数适配器](03-function-adapters.md)
- 源码参考：`bits/stl_iterator.h`（`reverse_iterator`、`back_insert_iterator` 等）
