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

<details>
<summary>参考答案</summary>

1. 主要区别四点：
   1. **传范围而不是一对迭代器**：`std::ranges::sort(v)` 取代 `std::sort(v.begin(), v.end())`，少了"两个迭代器不匹配"的错误来源。
   2. **支持投影（projection）**：可加第三个参数 `&T::field`，先投影再比较。
   3. **它们是函数对象（niebloid）**：`std::ranges::sort` 不是普通函数模板，不受 ADL 干扰，可以整体传给别的算法，且不能靠 ADL 找到。
   4. **由 concepts 约束**：不满足 `sortable` 时给出"哪个 concept 不满足"的清晰报错，且返回值更丰富（如 `ranges::for_each` 返回 `{迭代器, 函数}`）。
2. **范围（range）**就是能用 `ranges::begin` / `ranges::end` 取得迭代器对的东西——数组、有 `begin/end` 成员或自由函数的类型。
层次（按迭代器能力递进）：`input_range` → `forward_range` → `bidirectional_range` → `random_access_range` → `contiguous_range`；此外还有正交的 `sized_range`（O(1) 取 size）、`common_range`（起止迭代器同类型）、`borrowed_range`（迭代器在原范围销毁后仍可用）、`view`、`viewable_range`。
3. `std::ranges::begin(v)` 是**统一的 CPO（niebloid）**：对数组、容器、`std::array`、以及任何通过 ADL 提供 `begin` 的类型都成立（`int arr[3]` 也能用），而 `v.begin()` 只是成员调用，数组根本没有。
另外它是 `std::ranges` 名字空间里的函数对象：不受 ADL 影响、可被当作参数传递，并且只有在类型真的满足 `range` 概念时才有结果——比裸成员调用更严格、更一致。
4. **投影**是在比较/取值之前，先对元素施加一个可调用物（通常是成员指针），"按元素的某个部分"来比较。
`std::ranges::sort(v, {}, &T::field)` 三个参数分别是：
   1. **范围** `v`；
   2. **比较器**，这里 `{}` 表示默认的 `std::ranges::less`（即 `<`）；
   3. **投影** `&T::field`——比较时先取 `elem.field` 再比。
投影也可以换成 lambda（如 `[](const Person& p){ return p.name.size(); }`）。
5. 用投影把"按哪个字段"直接写在调用里，省掉一堆手写比较器：
```cpp
std::ranges::sort(ticks, {}, &Tick::timestamp);                 // 按时间排序
auto it = std::ranges::find(ticks, target_sym, &Tick::sym_id);  // 按合约号查找
auto best_bid = std::ranges::max_element(orders, std::less{}, &Order::price);
```
好处：一行表达意图、避免手写 lambda 出错，且投影在比较前求值一次（而不是每次比较都取成员两次）。

</details>
