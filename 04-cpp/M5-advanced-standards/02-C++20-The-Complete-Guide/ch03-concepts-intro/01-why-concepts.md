# 为什么需要 Concepts

## C++17 模板的错误灾难

```cpp
// C++17 模板
template <typename T>
void process(T x) {
    x.foo();
    x.bar(42);
    typename T::value_type val = x.get();
}

process(42);
// 错误信息：
// error: request for member 'foo' in 'x', which is of non-class type 'int'
//   x.foo();
//      ^
// note: in instantiation of function template 'process<int>' requested here
//   process(42);
//           ^
// ... 几百行模板实例化栈 ...
```

问题：错误信息指向模板内部，告诉你 `int` 没有 `foo()`，但用户不知道 `T` 需要满足什么要求。

## C++20 Concepts：编译期约束

```cpp
#include <concepts>

// 定义 Concept：T 必须有 foo()、bar(int) 和 value_type
template <typename T>
concept Processable = requires(T x) {
    x.foo();
    x.bar(42);
    typename T::value_type;
    { x.get() } -> std::convertible_to<typename T::value_type>;
};

// 使用 Concept 约束
void process(Processable auto x) {
    x.foo();
    x.bar(42);
    typename T::value_type val = x.get();
}

process(42);
// 错误信息：
// error: constraints not satisfied
//   'int' does not satisfy the concept 'Processable'
// ... 一行说清楚 ...
```

## Concepts 的核心价值

1. **清晰的错误信息**：约束失败一目了然
2. **文档化的接口要求**：Concept 名字就是文档
3. **更好的重载分派**：编译器根据约束选择最佳匹配
4. **IDE 支持**：编辑器能显示 Concept 要求
5. **零运行时开销**：纯编译期检查

## C++17 的替代方案对比

```cpp
// C++17 enable_if：冗长、错误差
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>>
void process(T x) { /* int 版 */ }

// C++20 Concepts：简洁
void process(std::integral auto x) { /* int 版 */ }
void process(std::floating_point auto x) { /* float 版 */ }
```

## 自测题

1. C++17 模板的错误信息有什么问题？
2. Concepts 的核心价值是什么？（列出至少 3 点）
3. Concept 约束失败时错误信息和 C++17 有什么区别？
4. Concepts 有运行时开销吗？
5. C++17 的 `enable_if` 和 C++20 Concepts 在重载分派上有什么区别？
