# C++17 弃用的特性

## 弃用 vs 移除

- **移除**：编译失败，代码必须修改
- **弃用**：编译警告（`-Wdeprecated`），仍可用但建议迁移

## C++17 弃用的特性

| 特性 | 替代 | 原因 |
|------|------|------|
| `std::result_of` | `std::invoke_result` | 语法怪异，有已知问题 |
| `std::is_literal_type` | `is_trivially_copyable` 等 | 过于宽泛，不实用 |
| `std::raw_storage_iterator` | `uninitialized_copy` 等 | 不安全 |
| `std::get_temporary_buffer` | `aligned_alloc` 等 | 难用且无优势 |
| `std::iterator<>` 基类 | 直接定义 typedef | 设计有缺陷 |
| `<ccomplex>`/`<cstdalign>` 等 | `<complex>`/`<cstddef>` 等 | C 兼容头不需要 |
| `std::pointer_to_binary_function` | `std::function`/lambda | 冗余 |

## [[deprecated]] 属性

```cpp
// C++14 引入，C++17 常用

// 函数级
[[deprecated("use new_func() instead")]]
void old_func();

// 类级
[[deprecated]]
class OldClass {};

// 枚举值
enum Color {
    RED [[deprecated("use CRIMSON")]] = 1,
    CRIMSON = 1,
    GREEN = 2,
};

// 模板
template <typename T>
[[deprecated("use NewTemplate instead")]]
class OldTemplate {};
```

## result_of → invoke_result

```cpp
// C++11 result_of（C++17 弃用）
template <typename F, typename... Args>
using R1 = typename std::result_of<F(Args...)>::type;

// C++17 invoke_result
template <typename F, typename... Args>
using R2 = std::invoke_result_t<F, Args...>;

// result_of 的问题：
// 1. 语法 F(Args...) 像函数类型，不直观
// 2. 对成员函数指针推导有问题
// 3. 与 std::invoke 不对齐
```

## std::iterator 弃用

```cpp
// C++17 前：继承 std::iterator
class MyIter : public std::iterator<std::forward_iterator_tag, int> {
    // 自动获得 typedef：value_type, difference_type, pointer, reference, iterator_category
};

// C++17：直接定义 typedef
class MyIter {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int*;
    using reference = int&;
    // ...
};
```

## 迁移建议

```cpp
// 1. 编译时开启弃用警告
// g++ -std=c++17 -Wdeprecated

// 2. 用 [[deprecated]] 标记内部旧 API
[[deprecated("use process_tick_v2 instead")]]
void process_tick(const Tick&);

// 3. 渐进迁移：先标记、再替换
// 4. CI 中 -Werror=deprecated-declarations 强制迁移
```

## 自测题

1. 弃用和移除的区别是什么？
2. `std::result_of` 为什么被弃用？用什么替代？
3. `std::iterator` 基类为什么弃用？替代写法是什么？
4. `[[deprecated]]` 可以用在哪些地方？能带消息吗？
5. 如何在 CI 中强制处理弃用警告？

<details>
<summary>参考答案</summary>

1. **弃用（deprecated）**：仍然合法、仍被标准支持，只是明确"不推荐继续使用"，通常伴随编译警告，并**预告未来可能移除**——代码还能编译，迁移可以排期。
**移除（removed）**：已经不在标准里，使用它属于编译错误（或该名字被彻底删除/复用）——必须改代码才能过编译。
简言之：弃用是"警告 + 留出迁移窗口"，移除是"直接失效"。
2. 弃用原因有三点：
   1. 语法 `result_of<F(Args...)>` 用"函数类型拼接"表达，读起来像函数声明，不直观；
   2. 在参数是引用/右值引用、`F` 是成员函数指针等场景下推导规则几经修补，语义不够可靠；
   3. 与 `std::invoke` 的 INVOKE 语义**不对齐**，容易与真正的行为脱节。
替代：用 `std::invoke_result<F, Args...>`（惯用别名 `std::invoke_result_t<F, Args...>`），参数分开传、语义与 `std::invoke` 一致。`result_of` 在 C++20 中被移除。
3. 弃用原因：它的模板参数顺序（`iterator<Category, T, Distance, Pointer, Reference>`）极易写错，且靠继承拿 typedef 会带来不必要的耦合、无法表达 C++20 之后更细的迭代器概念。
替代写法：在迭代器类里**直接定义**五个成员 typedef：
```cpp
class MyIter {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = int;
    using difference_type   = std::ptrdiff_t;
    using pointer           = int*;
    using reference         = int&;
};
```
同时也可以不定义 `pointer` / `reference`（它们可推导），只要 `iterator_category`、`value_type`、`difference_type` 齐备即可被 `iterator_traits` 识别。
4. 可用于**类/结构体、类型别名（typedef）、变量、非静态数据成员、函数、枚举与枚举项、模板特化，以及命名空间**——凡是使用被标记名字的地方，编译器都会给出弃用警告。
**可以带消息**：
```cpp
[[deprecated("use process_tick_v2 instead")]]
void process_tick(const Tick&);
```
消息会显示在警告里，非常适合做渐进式迁移（先标记、给调用方明确指引，再分批替换）。
5. 关键是**把警告升级为错误**并纳入流水线：
   - GCC/Clang：`-Wdeprecated-declarations -Werror=deprecated-declarations`（或全局 `-Werror` 配合白名单）；
   - MSVC：`/W4` 让 C4996 可见，再用 `/we4996` 把它变成错误。
渐进流程：先在代码里用 `[[deprecated("...")]]` 标记旧 API → 开警告让所有调用点可见 → 分批替换 → 最后在 CI 打开「警告即错误」固化，防止回退。注意：升级为错误要排除第三方头（可用 `-isystem` 或 `#pragma GCC diagnostic` 局部屏蔽），避免被外部库的弃用声明淹没。

</details>
