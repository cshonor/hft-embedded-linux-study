# Item 35：算法通常优于手写循环

> 第 5 章 算法 · Item 35 · 上一节：[Item 34 binary_search/lower_bound](item34-binary-search-lower-bound.md) · 下一节：[Item 36 正确选择算法](item36-choose-right-algorithm.md)

## 为什么要学这个（先建立直觉）

C 程序员习惯手写循环：

```c
// 查找第一个 > 5 的元素
int found = -1;
for (int i = 0; i < n; ++i) {
    if (arr[i] > 5) { found = i; break; }
}

// 拷贝满足条件的元素
int j = 0;
for (int i = 0; i < n; ++i)
    if (arr[i] > 5) dst[j++] = arr[i];
```

C++ 的 STL 算法更清晰：

```cpp
auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
std::copy_if(v.begin(), v.end(), std::back_inserter(dst),
             [](int x) { return x > 5; });
```

---

## 这节讲什么

STL 算法比手写循环更正确（充分测试）、更可读（声明意图）、更高效（针对迭代器分类特化，如 `copy` 对 `memmove` 特化）。

---

## 算法的三大优势

### 1. 正确性

```cpp
// 手写循环：容易 off-by-one
for (int i = 0; i <= n; ++i)  // ❌ 应该是 < 不是 <=
    process(arr[i]);

// 算法：迭代器范围自动正确
std::for_each(v.begin(), v.end(), process);  // ✅
```

### 2. 可读性

```cpp
// 手写循环：意图不明确
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it > 5) results.push_back(*it);

// 算法：直接表达"拷贝满足条件的"
std::copy_if(v.begin(), v.end(), std::back_inserter(results),
             [](int x) { return x > 5; });
```

### 3. 性能

```cpp
// 手写循环：编译器不知道循环意图，难以优化
for (size_t i = 0; i < v.size(); ++i)
    dst[i] = v[i];

// 算法：std::copy 对连续内存特化为 memmove
std::copy(v.begin(), v.end(), dst);  // 可能调用 memmove → SIMD 优化
```

### 例外

```cpp
// 当算法需要复杂的绑定/适配器时，简单循环可能更清晰
// C++14 lambda + 算法的组合已大幅减少这种例外
std::transform(v.begin(), v.end(), std::back_inserter(result),
    std::bind(std::multiplies<int>(), std::placeholders::_1, 2));
// vs
for (auto x : v) result.push_back(x * 2);  // 更清晰
```

---

## 常见错误（新手踩坑）

### 错误 1：用手写循环代替 find_if

```cpp
// 冗长
auto it = v.begin();
for (; it != v.end(); ++it)
    if (*it > 5) break;
// vs
auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
```

**修正：** 用 `find_if`——一行代码，意图清晰。

### 错误 2：手写循环实现 accumulate

```cpp
int sum = 0;
for (auto x : v) sum += x;
// vs
int sum = std::accumulate(v.begin(), v.end(), 0);
```

**修正：** 用 `accumulate`——更清晰且可并行化。

### 错误 3：手写循环实现 transform

```cpp
for (size_t i = 0; i < v.size(); ++i)
    result[i] = v[i] * 2;
// vs
std::transform(v.begin(), v.end(), result.begin(),
               [](int x) { return x * 2; });
```

**修正：** 用 `transform`——更安全且可特化优化。

---

## 新手要点（和 C 的区别）

| 维度 | C 手写循环 | C++ STL 算法 | 为什么 |
|------|-----------|-------------|--------|
| 正确性 | 易 off-by-one | 充分测试 | 标准库 |
| 可读性 | 意图不明确 | 声明式 | 命名表达意图 |
| 性能 | 编译器难优化 | 迭代器分类特化 | memmove/SIMD |
| 可并行化 | 手动 | C++17 并行算法 | execution policy |

**一句话：** C 的手写循环是唯一选择。C++ 的 STL 算法更正确、更可读、更高效——`copy` 可能特化为 `memmove`，`find_if` 比手写循环更直白。

---

## HFT 关联

- **`copy` 的 `memmove` 特化**：连续内存容器间 `copy` 被特化为 `memmove`，HFT 批量拷贝 tick 用 `std::copy` 比手写循环更可能走 SIMD/`memmove`。
- **`accumulate` 初值类型陷阱**：累加成交额用 `0LL`（int64）而非 `0`（int），避免大额溢出。

---

## 代码自测

### Q1: find_if vs 循环
```cpp
std::vector<int> v = {1, 3, 5, 7, 9};

// A: 手写循环
auto it = v.begin();
for (; it != v.end(); ++it) { if (*it > 5) break; }

// B: find_if
auto it2 = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
```
> A 和 B 的结果相同吗？

<details>
<summary>答案</summary>

**相同**，都指向 7（第一个 > 5 的元素）。但 B 更简洁、更清晰、更不易出错——一行代码表达"找到第一个满足条件的"。
</details>

### Q2: copy 特化
```cpp
std::vector<int> src = {1, 2, 3, 4, 5};
std::vector<int> dst(5);

// A: 手写循环
for (size_t i = 0; i < src.size(); ++i) dst[i] = src[i];

// B: std::copy
std::copy(src.begin(), src.end(), dst.begin());
```
> B 为什么可能比 A 快？

<detailf>
<summary>答案</summary>

`std::copy` 对连续内存容器（`vector`/`array`/原生指针）特化为 `memmove`——底层用 SIMD 指令批量拷贝，比逐元素循环快。

手写循环编译器可能也能优化为 `memcpy`，但不保证。`std::copy` 的特化是标准库实现保证的。
</details>

### Q3: accumulate
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
// A
int sum = 0;
for (auto x : v) sum += x;
// B
int sum2 = std::accumulate(v.begin(), v.end(), 0);
```
> A 和 B 结果相同吗？

<detailf>
<summary>答案</summary>

**相同**（都是 15）。但 B 更简洁，且 `accumulate` 可以传自定义操作：

```cpp
// 求积
int product = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());
// = 120
```

**注意**：初始值类型决定结果类型——`0` 是 int，`0LL` 是 long long。HFT 累加成交额用 `0LL` 避免溢出。
</details>

### Q4: 例外场景
```cpp
// 什么时候手写循环比算法更好？
std::vector<int> v = {1, 2, 3, 4, 5};
std::vector<int> result;

// A: 用算法 + 适配器
std::transform(v.begin(), v.end(), std::back_inserter(result),
    std::bind(std::multiplies<int>(), std::placeholders::_1, 2));

// B: 手写循环 + lambda
for (auto x : v) result.push_back(x * 2);
```
> 哪个更清晰？

<detailf>
<summary>答案</summary>

**B 更清晰**。当算法需要复杂的 `bind`/适配器才能表达时，简单循环 + lambda 更易读。

C++14 起，lambda 让大部分算法用法变得简洁：
```cpp
std::transform(v.begin(), v.end(), std::back_inserter(result),
               [](int x) { return x * 2; });  // 现在也清晰了
```

**原则**：优先用算法。当算法比循环更复杂时才用循环。
</details>

---

## 参考与延伸

- 上一节：[Item 34 binary_search/lower_bound](item34-binary-search-lower-bound.md)
- 下一节：[Item 36 正确选择算法](item36-choose-right-algorithm.md)
- 回到：[第 5 章 算法](README.md)
