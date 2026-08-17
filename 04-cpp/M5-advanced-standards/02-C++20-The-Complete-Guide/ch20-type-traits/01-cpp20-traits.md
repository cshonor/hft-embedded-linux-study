# C++20 type_traits 新增

## is_bounded_array / is_unbounded_array

```cpp
#include <type_traits>

static_assert(std::is_bounded_array_v<int[10]>);   // true：有大小
static_assert(!std::is_bounded_array_v<int[]>);     // false：无大小
static_assert(!std::is_bounded_array_v<int>);       // false：不是数组

static_assert(std::is_unbounded_array_v<int[]>);    // true
static_assert(!std::is_unbounded_array_v<int[10]>); // false
```

## remove_cvref

```cpp
// C++20：一步去除 const、volatile 和引用
template <typename T>
using CleanType = std::remove_cvref_t<T>;

static_assert(std::is_same_v<CleanType<const int&>, int>);
static_assert(std::is_same_v<CleanType<volatile int*>, volatile int*>);
// 注意：remove_cvref 只去顶层 cv 和引用，不去指针指向的 cv

// C++17 需要两步：
// std::remove_cv_t<std::remove_reference_t<T>>
```

## is_constant_evaluated

```cpp
// C++20：检测当前是否在编译期执行
constexpr int smart_calc(int x) {
    if (std::is_constant_evaluated()) {
        // 编译期路径：不能用非 constexpr 函数
        return x * 2;
    } else {
        // 运行期路径：可以用快速内置函数
        return fast_multiply(x, 2);  // 运行期优化
    }
}

constexpr int a = smart_calc(21);  // 编译期路径
int b = smart_calc(21);            // 运行期路径
```

## is_layout_compatible

```cpp
// C++20：检测两个类型是否布局兼容
struct A { int x; double y; };
struct B { int a; double b; };

static_assert(std::is_layout_compatible_v<A, B>);  // true（相同布局）

struct C { double y; int x; };  // 顺序不同
static_assert(!std::is_layout_compatible_v<A, C>); // false
```

## is_corresponding_member

```cpp
// C++20：检测两个成员指针是否指向对应成员
struct A { int x; double y; };
struct B { int a; double b; };

static_assert(std::is_corresponding_member_v<&A::x, &B::a>);  // true
static_assert(std::is_corresponding_member_v<&A::y, &B::b>);  // true
static_assert(!std::is_corresponding_member_v<&A::x, &B::b>); // false
```

## 自测题

1. `remove_cvref` 一步做了什么？C++17 怎么实现？
2. `is_constant_evaluated()` 做什么？有什么用？
3. `is_bounded_array<int[10]>` 和 `is_unbounded_array<int[]>` 的结果？
4. `is_layout_compatible` 检测什么？
5. `is_corresponding_member` 的用途？
