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
