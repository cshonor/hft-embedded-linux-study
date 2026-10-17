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

<details>
<summary>参考答案</summary>

1. 问题在于错误**发生在模板内部、且非常晚**：编译器先盲目实例化，然后在深层嵌套（几十层模板栈、标准库实现代码）里报出密密麻麻的错误；错误位置指向库内部而不是用户的调用点；用 `enable_if` 约束失败时，只报一句"没有匹配的函数"（no matching function），完全不说**为什么**不匹配。
2. 核心价值（任选三点展开）：
   1. **错误信息清晰**：直接指出哪个约束的哪一项不满足。
   2. **接口文档化**：`template<std::random_access_range R>` 这样的签名本身就是需求说明。
   3. **更好的重载分派**：约束参与重载消解（subsumption），更严格者胜出。
   4. **可复用、可组合**：约束像类型一样被命名、组合、复用。
   5. **IDE/工具友好**：工具能在调用点就提示约束不满足。
   6. **零运行期开销**：纯编译期检查，不生成任何代码。
3. Concept 失败时，编译器在**调用点**就报"约束不满足"，并说明具体是哪一个 concept 的哪一项要求没过，通常只有几行。
C++17 的 `enable_if` / SFINAE 失败时，报的是"no matching function for call to ..."加一长串被丢弃的候选；若是无约束模板，则报出模板**内部**几十上百行的实例化错误，且指向标准库源码。
差别本质上是"在门口检查（Concept）"和"进去以后才炸（SFINAE/实例化）"。
4. 没有。Concept 与约束完全是**编译期**的：它们只影响重载消解与是否可实例化，不生成任何运行期代码、不占空间、不影响 ABI。
唯一的"成本"是编译时间（约束求值需要编译期计算）。
5. `enable_if` 靠 **SFINAE 增删候选**：重载之间没有"谁更严格"的概念，必须手工把条件写成互斥（如 `enable_if_t<cond1>` 与 `enable_if_t<!cond1>`），写错就二义或全被丢弃；条件塞在函数签名里（额外模板参数 / 返回类型 / 参数类型），可读性差且污染签名。
Concepts 由**约束的偏序（subsumption）**决定胜负：两个都可行的候选中，更受约束（更严格）的那个自动胜出，无需手工写互斥条件；约束写在模板参数列表或 requires 子句里，签名干净、可读、可组合。

</details>
