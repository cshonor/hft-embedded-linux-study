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

<details>
<summary>参考答案</summary>

1. 它**一步**去掉引用以及顶层的 `const` / `volatile`：`remove_cvref_t<const int&>` 得到 `int`。
C++17 需要两步嵌套：
```cpp
using Clean = std::remove_cv_t<std::remove_reference_t<T>>;   // C++17
using Clean2 = std::remove_cvref_t<T>;                        // C++20
```
注意它只处理**顶层**：`remove_cvref_t<volatile int*>` 仍然是 `volatile int*`（指针本身没 cv，指针**指向**的 `volatile` 不会被去掉）；`remove_cvref_t<const int* const&>` 得到 `const int*`。
2. 它判断"**当前这次求值是否处于常量求值（编译期）语境**"，是 `constexpr` 函数里走不同实现的标准手段：
```cpp
constexpr int smart_calc(int x) {
    if (std::is_constant_evaluated()) {
        return x * 2;                  // 编译期：只能用 constexpr 允许的写法
    } else {
        return fast_multiply(x, 2);    // 运行期：可用 intrinsics / SIMD / 查表
    }
}
```
用途：编译期路径保证可移植且可常量求值，运行期路径用最快的机器指令，从而"一份代码、两种最优实现"。
注意：它**不是** `if constexpr`——两个分支都会被编译（只是运行期只执行其中一个）；也不要用它判断"是否会被优化成常量"。
3. `std::is_bounded_array_v<int[10]>` 为 **true**（有界数组）；`std::is_unbounded_array_v<int[]>` 为 **true**（无界数组）。
两者互斥：`is_bounded_array_v<int[]>` 为 `false`，`is_unbounded_array_v<int[10]>` 为 `false`。
实际用途是区分这两个容易混淆的场景——`T[]` 常出现在 `operator new[]`、`std::unique_ptr<T[]>`、以及"用不完整数组类型表达定长数据"的惯用法里，而 `T[N]` 才是能取 `sizeof`/`std::size` 的完整类型。
4. 它检测两个类型是否**布局兼容（layout-compatible）**：两个标准布局类（standard-layout class）拥有相同数量的非静态数据成员，且**按声明顺序对应的成员逐一布局兼容**时结果为 `true`。
```cpp
struct A { int x; double y; };
struct B { int a; double b; };
static_assert(std::is_layout_compatible_v<A, B>);   // true：成员类型按序一致
struct C { double y; int x; };
static_assert(!std::is_layout_compatible_v<A, C>);  // false：顺序不同
```
典型用途：在静态断言里保证"协议结构体 A 与 B 可以安全地按字节互转/复用缓冲"。（注意：布局兼容不等于大小相同——`int` 与 `long` 大小相同也**不**布局兼容。）
5. 它检测两个**成员指针**是否指向两个布局兼容类中"**位置相对应**"的成员：
```cpp
static_assert(std::is_corresponding_member_v<&A::x, &B::a>);   // true
static_assert(!std::is_corresponding_member_v<&A::x, &B::b>);  // false
```
用途：与 `is_layout_compatible` 配合，在编译期把两套结构体（如行情协议的两个版本、内部结构与外部协议结构）的字段**逐一对应**起来——既可以做静态断言防止字段错位，也能据此生成字段级的转换/序列化代码，避免手写偏移带来的错位风险。

</details>
