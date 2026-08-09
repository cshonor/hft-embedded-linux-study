# 第 10 章 泛型算法

标准库未在每个容器上定义所有功能，而是提供一组独立于特定容器的**泛型算法**，用于对序列（通常是一段迭代器范围）进行操作。

## 小节

- [基础算法与重排](./10.1-基础算法与重排.md)
- [定制操作与 Lambda 表达式](./10.2-定制操作与Lambda表达式.md)
- [再探迭代器](./10.3-再探迭代器.md)
- [特定容器算法](./10.4-特定容器算法.md)


## 章节摘要

泛型算法（不绑定容器、操作迭代器范围）：`find`/`count`/`accumulate`/`fill`/`sort`/`unique`/`copy`/`transform` 等。定制操作（lambda 表达式、谓词、函数对象）、再探迭代器（插入迭代器/流迭代器/反向迭代器）。

### 和 C 的区别

| C | C++ |
|---|-----|
| `qsort` + 函数指针 | `std::sort` + lambda（类型安全+可内联） |
| `bsearch` | `std::binary_search`/`lower_bound` |
| 手写循环 | `std::for_each`/`std::transform`/`std::copy` |
| 无迭代器抽象 | 迭代器统一所有容器 |

## 章节自测

### Q1: sort + lambda

```cpp
std::vector<std::string> words = {"banana", "apple", "cherry"};
std::sort(words.begin(), words.end(),
    [](const std::string &a, const std::string &b) {
        return a.size() < b.size();
    });
// words 现在是什么顺序？
```

> words 排序后是什么？lambda 的作用是什么？

<details>
<summary>答案与复习指引</summary>

**words = `{"apple", "banana", "cherry"}`** — 按长度排序（5, 6, 6）

**lambda 的作用：** 自定义排序准则——`a.size() < b.size()` 表示"长度小的排前面"。

**和 C 的区别：** C 的 `qsort` 需要函数指针，无法内联，且类型不安全（`void*` + 手动 cast）。C++ lambda 可内联，编译器优化效果好。

**复习：** → [定制操作与 Lambda 表达式](./10.2-定制操作与Lambda表达式.md)
</details>

### Q2: accumulate

```cpp
std::vector<int> nums = {1, 2, 3, 4, 5};
auto sum = std::accumulate(nums.begin(), nums.end(), 0);
auto product = std::accumulate(nums.begin(), nums.end(), 1, std::multiplies<int>());
// sum 和 product 分别是多少？
```

> sum 和 product 分别是多少？第三个参数 0 和 1 分别是什么意思？

<details>
<summary>答案与复习指引</summary>

**sum = 15**（1+2+3+4+5），**product = 120**（1×2×3×4×5）

**第三个参数：** 初始值（accumulator 的起点）。`0` 用于求和，`1` 用于求积。

**注意类型陷阱：** `accumulate` 的返回类型由第三个参数决定。`std::accumulate(v.begin(), v.end(), 0)` 如果 `v` 是 `vector<double>`，结果会是 `int`（截断）！要用 `0.0` 才能得到 `double`。

**复习：** → [基础算法与重排](./10.1-基础算法与重排.md)
</details>

### Q3: find_if + 谓词

```cpp
std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
if (it != v.end())
    std::cout << *it;
```

> 输出是什么？`find_if` 的返回值是什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `9`（第一个大于 5 的元素）

**返回值：** 指向第一个满足谓词的元素的迭代器。如果都不满足，返回 `v.end()`。

**谓词：** 返回 `bool` 的可调用对象（lambda/函数指针/函数对象）。

**复习：** → [定制操作与 Lambda 表达式](./10.2-定制操作与Lambda表达式.md)
</details>

### Q4: back_inserter

```cpp
std::vector<int> src = {1, 2, 3};
std::vector<int> dest;
std::copy(src.begin(), src.end(), std::back_inserter(dest));
// dest 现在是什么？如果用 dest.begin() 替代 back_inserter 呢？
```

> dest 是什么？用 `dest.begin()` 会怎样？

<details>
<summary>答案与复习指引</summary>

**dest = `{1, 2, 3}`** — `back_inserter` 创建插入迭代器，每次赋值自动调用 `push_back`。

**用 `dest.begin()`：** UB！`dest` 是空的，`begin()` 等于 `end()`，`copy` 向空容器的 `begin()` 写入是未定义行为。

**插入迭代器类型：**
- `back_inserter` → `push_back`
- `front_inserter` → `push_front`
- `inserter` → `insert`（指定位置）

**复习：** → [再探迭代器](./10.3-再探迭代器.md)
</details>
