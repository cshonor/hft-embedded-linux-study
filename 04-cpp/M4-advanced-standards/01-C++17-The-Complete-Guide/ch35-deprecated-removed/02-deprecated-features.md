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
