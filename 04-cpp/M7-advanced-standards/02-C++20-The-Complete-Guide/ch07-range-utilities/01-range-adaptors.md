# 范围适配器

## 范围工厂

```cpp
#include <ranges>

// iota：生成递增序列
auto seq = std::views::iota(1, 10);     // 1,2,...,9
auto inf = std::views::iota(0);          // 0,1,2,...（无限序列）

// empty：空范围
auto e = std::views::empty<int>;

// single：单元素范围
auto s = std::views::single(42);  // [42]

// repeat（C++23）：重复元素
// auto r = std::views::repeat(0, 5);  // 0,0,0,0,0
```

## 范围概念

```cpp
// 检查范围类型
template <typename T>
concept IsRange = std::ranges::range<T>;

template <typename T>
concept IsSizedRange = std::ranges::sized_range<T>;  // 有 size()

template <typename T>
concept IsView = std::ranges::view<T>;  // 是视图（轻量、不拥有）

template <typename T>
concept IsCommonRange = std::ranges::common_range<T>;  // begin/end 同类型

// 使用
template <std::ranges::sized_range R>
auto process(R&& r) {
    return r.size();  // 有 size() 保证
}
```

## 视图概念

```cpp
// view：O(1) 可移动/拷贝，不拥有数据
// viewable_range：能安全转成 view 的范围
std::ranges::view<T>           // 是视图
std::ranges::viewable_range<T> // 能变成视图（容器或临时都能）

// 临时容器可以转成 view
auto v = std::vector{1,2,3} | std::views::filter(...);
// 临时 vector 通过 viewable_range 安全地变成 view

// 左值容器直接用
std::vector<int> vec = {1,2,3};
auto v2 = vec | std::views::take(2);
// vec 是左值，view 引用它
```

## 自定义范围适配器

```cpp
// C++20 可以自定义视图
template <std::ranges::view V>
class take_every_n_view : public std::ranges::view_interface<take_every_n_view<V>> {
    V base_;
    std::size_t step_;
public:
    take_every_n_view(V base, std::size_t n) : base_(std::move(base)), step_(n) {}

    auto begin() { return std::ranges::begin(base_); }
    auto end() { return std::ranges::end(base_); }
    // 简化：实际需要跳步迭代器
};

// 范围适配器闭包对象（RACO）
struct take_every_n_fn {
    std::size_t n;
    template <std::ranges::viewable_range R>
    auto operator()(R&& r) const {
        return take_every_n_view(std::views::all(std::forward<R>(r)), n);
    }
};

// 用法
// auto result = v | take_every_n_fn{3};
```

## 自测题

1. `std::views::iota` 生成什么？能生成无限序列吗？
2. `view` 和 `viewable_range` 的区别？
3. 临时容器能直接用管道吗？为什么安全？
4. 如何自定义视图？需要继承什么？
5. `sized_range` 保证什么？

<details>
<summary>参考答案</summary>

1. 它生成 `std::ranges::iota_view`，一个**按需递增的整数序列视图**（不存储任何元素）。
两种形式：`views::iota(first, last)` 是**有界**序列 `[first, last)`；`views::iota(first)` 是**无界（无限）**序列。
可以生成无限序列——它本身永不结束，必须配 `views::take` / `take_while` 之类截断，或者在算法里自己 break，否则遍历不会终止。
2. `std::ranges::view<T>`：T **已经是**视图——满足 `range` + `movable` + `enable_view<T>`（语义上还要求拷贝/移动/析构是 O(1)），即"轻量、惰性、非拥有"。
`std::ranges::viewable_range<T>`：T **可以被安全地转换成**视图——即 `views::all(T)` 合法。它比 `view` 宽：既包括本身就是 view 的类型，也包括左值非 view 范围（包装成 `ref_view`），以及可移动的右值范围（包装成 `owning_view`）。
简单说：`view` 是"已经是视图"，`viewable_range` 是"能变成视图"。
3. 可以，而且安全：`std::vector<int>{1,2,3} | std::views::filter(f)` 是合法的。
原因在于 `views::all` 的派发规则：对**右值**的非 view 范围，`ref_view` 绑定不了（只接受左值），于是会包装成 **`std::ranges::owning_view`**——它把临时容器**移动进来并拥有它**，从而把生命周期延长到视图本身结束。
所以在同一条表达式（或把该视图作为对象保存）中使用是安全的。反过来，对**左值**容器只会得到 `ref_view`（只持有引用），此时视图绝不能超出容器的生命周期。
4. 两种路线：
   1. **组合已有适配器**：把 `views::transform` / `filter` 等串起来，或写一个返回视图的函数。
   2. **自定义视图类型**：写一个满足 `std::ranges::view` 的类，通常**派生自 `std::ranges::view_interface<Derived>`**（CRTP）——它免费提供 `empty()`、`operator bool`、`size()`、`front()`、`back()`、`operator[]` 等成员函数；`view_interface` 本身派生自 `view_base`，因此也满足 `enable_view`。
```cpp
template <std::ranges::view V>
class MyView : public std::ranges::view_interface<MyView<V>> {
    V base_;
public:
    // 自己实现 begin()/end()（以及可选 size()）
};
```
再配一个返回**范围适配器闭包**的对象，就能用 `|` 接入管道（C++23 起可直接用 `std::ranges::range_adaptor_closure`）。
5. 它保证**能在 O(1)（摊销常数时间）内拿到元素个数**：`std::ranges::size(r)` 合法且开销是常数。
```cpp
template <std::ranges::sized_range R>
void f(R&& r) { auto n = std::ranges::size(r); /* O(1) */ }
```
`std::vector` 满足 `sized_range`，`std::forward_list` 不满足（求 size 是 O(n)），`iota_view` 通常满足（长度可算）。拿到它就意味着可以安全地预分配、做并行分块等优化。

</details>
