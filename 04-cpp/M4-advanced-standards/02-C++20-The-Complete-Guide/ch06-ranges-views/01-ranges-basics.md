# Ranges 基础

## 从迭代器到范围

```cpp
#include <ranges>
#include <algorithm>

// C++17：算法要传 begin/end
std::sort(v.begin(), v.end());
std::find(v.begin(), v.end(), 42);
std::transform(v.begin(), v.end(), out.begin(), square);

// C++20：算法接受范围
std::ranges::sort(v);
std::ranges::find(v, 42);
std::ranges::transform(v, out.begin(), square);
```

## 范围概念

```cpp
// 范围 = 有 begin() 和 end() 的东西
// 容器（vector, list, map...）
// 数组（int arr[10]）
// string / string_view
// 自定义类型（有 begin/end）

// Concept 层次
std::ranges::range<T>              // 有 begin/end
std::ranges::input_range<T>        // 可读
std::ranges::forward_range<T>      // 可前向遍历
std::ranges::bidirectional_range<T> // 可双向
std::ranges::random_access_range<T> // 随机访问
std::ranges::contiguous_range<T>   // 连续内存
```

## ranges::begin / ranges::end

```cpp
// C++20 统一的 begin/end
std::vector<int> v = {1, 2, 3};
auto b = std::ranges::begin(v);  // v.begin()
auto e = std::ranges::end(v);    // v.end()

// 对数组也行
int arr[] = {1, 2, 3};
auto b2 = std::ranges::begin(arr);  // arr
auto e2 = std::ranges::end(arr);    // arr + 3
```

## 范围算法

```cpp
// 所有 STL 算法都有 ranges 版本
std::ranges::sort(v);
std::ranges::find(v, 42);
std::ranges::copy(v, out.begin());
std::ranges::transform(v, out.begin(), square);
std::ranges::for_each(v, [](int x) { /* ... */ });
std::ranges::accumulate(v, 0);  // C++23

// 投影（Projection）
struct Person { std::string name; int age; };
std::vector<Person> people;

// 按 age 排序（投影：提取 age 做比较）
std::ranges::sort(people, {}, &Person::age);
// {} = 默认比较（<），&Person::age = 投影

// 按名字长度查找
auto it = std::ranges::find(people, 5,
    [](const Person& p) { return p.name.size(); });
```

## HFT 应用

```cpp
// 简化算法调用
std::vector<Tick> ticks;
std::ranges::sort(ticks, {}, &Tick::timestamp);  // 按时间排序
auto it = std::ranges::find(ticks, target_sym, &Tick::sym_id);

// 投影让代码更清晰
auto best_bid = std::ranges::max_element(orders,
    std::less{}, &Order::price);
```

## 自测题

1. C++20 ranges 算法和 C++17 算法的调用方式有什么区别？
2. 什么是范围？范围 Concept 的层次是什么？
3. `ranges::begin` 和 `v.begin()` 的区别？
4. 投影（Projection）是什么？`std::ranges::sort(v, {}, &T::field)` 的三个参数分别是什么？
5. HFT 中如何用投影简化排序和查找？
