# 第 11 章 折叠表达式

**Fold Expressions**

## 本章讲什么

C++17 让可变参数模板的参数包（`args...`）能用一个二元运算符"折叠"成一个值，替代 C++14 的递归展开模板。

## 要点

### 四种折叠

```cpp
template <typename... Args>
auto sum(Args... args) {
    return (... + args);   // 一元右折叠：(args[0] + (args[1] + (... + args[n])))
}

template <typename... Args>
auto sum_left(Args... args) {
    return (args + ...);   // 一元左折叠：(((args[0] + args[1]) + ...) + args[n])
}

template <typename... Args>
auto sum_init(Args... args) {
    return (0 + ... + args);   // 二元左折叠：((0 + args[0]) + ...) + args[n]
}

template <typename... Args>
auto sum_init_right(Args... args) {
    return (args + ... + 0);   // 二元右折叠
}
```

| 形式 | 语法 | 含义 |
|------|------|------|
| 一元右折叠 | `(... op pack)` | `(p1 op (p2 op (p3 op p4)))` |
| 一元左折叠 | `(pack op ...)` | `(((p1 op p2) op p3) op p4)` |
| 二元右折叠 | `(pack op ... op init)` | `(p1 op (p2 op (init op p4)))` 等价 |
| 二元左折叠 | `(init op ... op pack)` | `(((init op p1) op p2) op p3)` |

### 空包的默认值

一元折叠对空参数包：
- `&&` 折叠空包 → `true`
- `||` 折叠空包 → `false`
- `,` 折叠空包 → `void()`
- 其他运算符折叠空包 → **编译错**（需用二元折叠提供初值）

```cpp
template <typename... Args>
bool all(Args... args) { return (... && args); }   // 空包返回 true

template <typename... Args>
auto sum(Args... args) { return (... + args); }    // 空包编译错！
template <typename... Args>
auto sum_safe(Args... args) { return (0 + ... + args); }  // 空包返回 0
```

### 实用例子

```cpp
// 1. 打印所有参数
template <typename... Args>
void print(Args... args) {
    (std::cout << ... << args);   // 二元左折叠，初值 std::cout
}

// 2. 全部满足条件
template <typename... Args>
bool all_positive(Args... args) {
    return (... && (args > 0));
}

// 3. 逐个调用
template <typename F, typename... Args>
void for_each(F f, Args... args) {
    (f(args), ...);   // 一元左折叠逗号运算符
}

// 4. 检查所有类型
template <typename... Ts>
constexpr bool all_integral = (std::is_integral_v<Ts> && ...);
```

### 运算符可以是任意二元运算符

`+`、`-`、`*`、`/`、`&&`、`||`、`,`、`->*`、`<<`、`>>`、`==`、`<` 等都行。

## HFT 关联

- **编译期参数校验**：`constexpr bool all_numeric = (is_arithmetic_v<Ts> && ...);` 静态断言所有模板参数是数值类型。
- **批量字段序列化**：`(os << args, ...);` 一行展开所有字段写入，替代手写递归。
- **批量注册回调**：`(handlers.push_back(args), ...);` 注册多个回调。
- **编译期求和**：策略参数表的元素总数用 `(... + sizeof(Ts))` 编译期算。
- **日志聚合**：`log(args...)` 一行折叠输出，无运行期递归开销。

## 自测题

1. 四种折叠形式分别是什么？语法怎么写？
2. 一元折叠空包的默认值规则是什么？哪些运算符有默认值？
3. `(std::cout << ... << args)` 是什么折叠？初值是什么？
4. `(f(args), ...)` 用了什么运算符？语义是什么？
5. HFT 用折叠表达式做编译期类型校验的写法是什么？

## 代码自测

### Q1: 可变参数模板的简化
```cpp
// C++14: 递归展开
template<typename T>
auto sum14(T t) { return t; }
template<typename T, typename... Args>
auto sum14(T t, Args... args) { return t + sum14(args...); }

// C++17: fold expression
template<typename... Args>
auto sum17(Args... args) { return (args + ...); }

sum17(1, 2, 3, 4);  // 10
```
> `(args + ...)` 是什么语法？有哪些 fold 形式？

<details>
<summary>答案与复习指引</summary>

**fold expression** 语法：`(pack op ...)` 或 `(... op pack)` 或 `(pack op ... op init)`。

| 形式 | 名称 | 展开 |
|------|------|------|
| `(args + ...)` | 一元右 fold | `((a1 + a2) + a3) + ...` |
| `(... + args)` | 一元左 fold | `a1 + (a2 + (a3 + ...))` |
| `(args + ... + 0)` | 二元右 fold | `((a1 + a2) + a3) + 0` |
| `(0 + ... + args)` | 二元左 fold | `0 + (a1 + (a2 + ...))` |

**二元 fold 的初始值**：用于空包（零参数）的 fallback。`(args + ... + 0)` 空包返回 0；一元 fold 空包是编译错误（`+` 无单位元）。

**用途**`：打印所有参数 `(std::cout << ... << args)`，检查全部为 true `(args && ...)`。

**复习：** → [fold expressions](./README.md)
</details>
