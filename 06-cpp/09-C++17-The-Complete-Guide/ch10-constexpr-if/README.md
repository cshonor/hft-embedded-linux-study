# 第 10 章 编译期 if

**Compile-Time if**

## 本章讲什么

`if constexpr (cond)` 让分支在**编译期**决定，未选中的分支不实例化代码。这是替代 SFINAE 和标签分派的利器，让泛型代码大幅简化。

## 要点

### 基本语法

```cpp
template <typename T>
void process(T x) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integer: " << x;
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "float: " << x;
    } else {
        std::cout << "other: " << x;
    }
}
```

### 关键区别：`if constexpr` vs 普通 `if`

```cpp
template <typename T>
void bad(T x) {
    if (std::is_integral_v<T>) {
        return x * 2;        // 浮点 T 时也实例化，编译错（x*2 可能 OK 但其他操作可能不合法）
    } else {
        return x.to_string();  // 整型 T 没有 to_string()，编译错！
    }
}

template <typename T>
void good(T x) {
    if constexpr (std::is_integral_v<T>) {
        return x * 2;            // 只在 T 是整型时实例化
    } else {
        return x.to_string();    // 只在 T 非整型时实例化
    }
}
```

普通 `if` 两个分支都会实例化——如果某分支对某类型不合法就编译错。`if constexpr` 只实例化选中分支，未选中的不检查合法性。

### 替代了什么

```cpp
// C++14 SFINAE：极其啰嗦
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
void process(T x) { /* int 版 */ }

template <typename T,
          typename = std::enable_if_t<!std::is_integral_v<T>>,
          typename = void>  // 避免重复
void process(T x) { /* other 版 */ }

// C++17：一行搞定
template <typename T>
void process(T x) {
    if constexpr (std::is_integral_v<T>) { /* int 版 */ }
    else { /* other 版 */ }
}
```

### 在递归模板中的应用

```cpp
template <typename... Args>
void print_all(Args... args) {
    if constexpr (sizeof...(args) > 0) {
        std::cout << args...[0];   // C++26 下标语法，C++20 可用 std::get
        print_all(args...[1:]);    // 递归处理剩余
    }
}
```

### 注意点

- 条件必须是**编译期常量表达式**（`constexpr bool`）。
- 未选中的分支**仍要语法合法**（括号匹配、token 合法），但不做语义检查（类型合法性不检查）。
- `if constexpr` 不能替代运行期 `if`——条件必须编译期可知。

## HFT 关联

- **热路径零开销分支**：`if constexpr (is_pod_v<T>)` 在编译期选 memcpy 路径或逐元素拷贝路径，运行期无分支预测开销。
- **替代 SFINAE 简化泛型**：策略模板对不同行情类型（L1/L2/Trade）用 `if constexpr` 分派，代码比 SFINAE 清晰 10 倍。
- **类型分派零开销**：`if constexpr (std::is_same_v<T, Tick>)` 替代虚函数分派，编译期决定，无 vtable 间接。
- **配合 `is_aggregate`/`is_trivially_copyable`**：`if constexpr (is_trivially_copyable_v<T>) memcpy(...)` 安全走快路径。
- **调试仍可用**：未选中分支不生成代码，但语法仍检查，编译期错误信息清晰。

## 自测题

1. `if constexpr` 和普通 `if` 的核心区别是什么？未选中分支会实例化吗？
2. `if constexpr` 替代了 C++14 的什么机制？好处是什么？
3. 未选中的分支完全不做任何检查吗？（提示：语法 vs 语义）
4. `if constexpr` 的条件必须满足什么？
5. HFT 用 `if constexpr` 做 POD 分派有什么好处？相比虚函数分派？
