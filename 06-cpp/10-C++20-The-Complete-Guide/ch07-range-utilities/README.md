# 第 7 章 范围与视图的实用工具

**Utilities for Ranges and Views**

## 本章讲什么

Ranges 的辅助工具：`ranges::begin`/`end`/`size`/`data`、`ranges::distance`、`ranges::empty`、`ranges::advance`、`ranges::next`/`prev`、`views::counted`、`ranges::subrange`、自定义视图适配器。

## 要点

### 范义访问函数

```cpp
std::vector<int> v = {1,2,3};
int arr[5] = {1,2,3,4,5};

std::ranges::begin(v);    // v.begin()
std::ranges::end(v);      // v.end()
std::ranges::size(v);     // v.size()
std::ranges::data(v);     // v.data()
std::ranges::empty(v);    // v.empty()

// 对数组也工作
std::ranges::size(arr);   // 5
std::ranges::data(arr);   // arr
```

`std::ranges::xxx` 比 `std::xxx` 更智能——能找到 ADL 的 `begin`、对数组特化、返回 `sentinel`（不一定和迭代器同类型）。

### `ranges::subrange`：迭代器+哨兵对

```cpp
std::vector<int> v = {1,2,3,4,5};

// subrange：把 [begin, end) 当范围
auto sub = std::ranges::subrange(v.begin() + 1, v.begin() + 4);
for (int x : sub) { /* 2,3,4 */ }

// 用 subrange 切片
auto first3 = std::ranges::subrange(v.begin(), v.begin() + 3);
```

`subrange` 是"迭代器 + 哨兵"的包装，让一对迭代器能当范围用。

### `views::counted`：从指针+数量构造

```cpp
int* p = ...;
size_t n = 100;

auto view = std::views::counted(p, n);   // [p, p+n) 的视图
// 等价 subrange{p, p+n}
```

### 迭代器操作

```cpp
auto it = v.begin();
std::ranges::advance(it, 3);    // 前进 3
auto next2 = std::ranges::next(it, 2);   // 前进 2（返回新迭代器）
auto prev2 = std::ranges::prev(it, 2);   // 后退 2
auto dist = std::ranges::distance(v.begin(), v.end());  // 距离
```

`ranges::advance/next/prev` 比旧 `std::advance` 更安全——能处理哨兵（非随机访问范围也能算距离）。

### 自定义视图适配器

```cpp
// 自定义视图：每隔 N 取一个
struct stride_fn {
    size_t n;
    constexpr stride_fn(size_t n) : n(n) {}
    template <std::ranges::input_range R>
    auto operator()(R&& r) const { /* 实现 */ }
};

// 可管道使用
auto every_other = v | stride_fn(2);
```

C++20 让自定义视图适配器标准化，可像内置视图一样用 `|` 组合。

### `ranges::to`（C++23）

```cpp
// C++23：视图转容器
auto vec = v | views::filter(...) | ranges::to<std::vector>();
```

C++20 没有 `ranges::to`，要把视图转容器要用 `for` 循环或 `copy`：

```cpp
std::vector<int> result;
std::ranges::copy(v | views::filter(f), std::back_inserter(result));
```

## HFT 关联

- **`subrange` 切片行情**：`subrange(buf.begin(), buf.begin() + valid_len)` 处理有效部分，不拷贝。
- **`views::counted` 处理 C 缓冲**：`counted(ptr, n)` 把裸指针+长度当范围，零拷贝遍历。
- **`ranges::size` 统一接口**：模板代码对数组和容器统一用 `ranges::size`，不用 `sizeof(arr)/sizeof(arr[0])`。
- **自定义视图做采样**：`ticks | stride(N)` 每隔 N 个取一个做快速预览。
- **回测切片**：`subrange(historical.begin() + start, historical.begin() + end)` 切回测区间。
- **`ranges::to` 待 C++23**：C++20 项目要把视图转容器仍需手写循环，或等 C++23 升级。

## 自测题

1. `std::ranges::begin` 比 `std::begin` 强在哪？
2. `ranges::subrange` 解决什么问题？
3. `views::counted(ptr, n)` 的用途？
4. `ranges::advance/next/prev` 相比旧 `std::advance` 的改进？
5. C++20 把视图转容器的难点是什么？C++23 如何解决？
