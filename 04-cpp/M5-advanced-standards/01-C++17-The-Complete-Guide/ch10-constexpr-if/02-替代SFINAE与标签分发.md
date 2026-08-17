# 10.2 替代 SFINAE 与标签分发

> 第 10 章 编译期 if · 上一节：[10.1 if constexpr 基础](01-if-constexpr基础.md)

## 这节讲什么

`if constexpr` 几乎可以替代所有 C++14 的 SFINAE 和标签分发技巧。本节通过对比 C++14 和 C++17 的写法，展示 `if constexpr` 如何大幅简化泛型代码。

## 替代 SFINAE

### C++14：enable_if

```cpp
// C++14：为整数类型提供特殊实现
template<typename T,
         typename = std::enable_if_t<std::is_integral_v<T>>>
T process(T x) {
    return x * 2;
}

// C++14：为浮点类型提供特殊实现
template<typename T,
         typename = std::enable_if_t<std::is_floating_point_v<T>>>
T process(T x) {
    return x * 2.0;
}

// 问题：两个重载函数签名相同（都是 process(T)）→ SFINAE 去选择
// 写法复杂，容易出错
```

### C++17：if constexpr

```cpp
// C++17：一个函数搞定
template<typename T>
T process(T x) {
    if constexpr (std::is_integral_v<T>) {
        return x * 2;
    } else if constexpr (std::is_floating_point_v<T>) {
        return x * 2.0;
    } else {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
                      "T must be integral or floating point");
    }
}
```

## 替代标签分发

### C++14：标签分发

```cpp
// C++14：标签分发
template<typename T>
void process_impl(T x, std::true_type /*is_integral*/) {
    std::cout << "integral: " << x * 2 << "\n";
}

template<typename T>
void process_impl(T x, std::false_type /*is_integral*/) {
    std::cout << "other: " << x << "\n";
}

template<typename T>
void process(T x) {
    process_impl(x, std::is_integral<T>{});
}
```

### C++17：if constexpr

```cpp
// C++17：直接写
template<typename T>
void process(T x) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integral: " << x * 2 << "\n";
    } else {
        std::cout << "other: " << x << "\n";
    }
}
```

## 替代递归模板

### C++14：递归 + 特化

```cpp
// 打印可变参数：递归展开
template<typename T>
void print(T x) {
    std::cout << x << "\n";
}

template<typename T, typename... Rest>
void print(T first, Rest... rest) {
    std::cout << first << ", ";
    print(rest...);  // 递归
}

// 需要 2 个函数 + 递归
```

### C++17：if constexpr + 折叠表达式

```cpp
// C++17：一个函数 + 折叠表达式
template<typename... Args>
void print(Args... args) {
    ((std::cout << args << ", "), ...);
    std::cout << "\n";
}

// 或者 if constexpr
template<typename T, typename... Rest>
void print(T first, Rest... rest) {
    std::cout << first;
    if constexpr (sizeof...(rest) > 0) {
        std::cout << ", ";
        print(rest...);  // 只在有剩余参数时编译
    } else {
        std::cout << "\n";
    }
}
```

## 编译期序列生成

### C++17：编译期循环展开

```cpp
template<typename F, size_t... Is>
void for_compile_time(F&& f, std::index_sequence<Is...>) {
    (f(std::integral_constant<size_t, Is>{}), ...);  // 折叠表达式
}

template<size_t N, typename F>
void for_compile_time(F&& f) {
    for_compile_time(std::forward<F>(f), std::make_index_sequence<N>{});
}

// 使用：编译期展开 N 次调用
for_compile_time<5>([](auto i) {
    std::cout << "Iteration " << i << "\n";
});
```

## if constexpr 的注意事项

### 1. 条件必须是编译期常量

```cpp
int x = 42;
// if constexpr (x > 0) { ... }  // ❌ x 不是编译期常量

constexpr int N = 10;
if constexpr (N > 5) { ... }  // ✅ N 是编译期常量
```

### 2. false 分支中的语法仍然检查

```cpp
template<typename T>
void f(T x) {
    if constexpr (std::is_integral_v<T>) {
        return x * 2;
    } else {
        // 语法错误仍然会报（即使是 false 分支）
        // return x @ 2;  // ❌ 语法错误，@ 不是运算符
        // 但类型错误不会报：
        return x.foo();  // 如果 T 没有 foo()，且 T 不是整数 → 不编译 → 不报错
    }
}
```

### 3. 返回类型推导的问题

```cpp
template<typename T>
auto f(T x) {
    if constexpr (std::is_integral_v<T>) {
        return x * 2;    // 返回 int
    } else {
        return x * 2.0;  // 返回 double
        // 问题：auto 推导只看第一个 return → 不同类型可能冲突
    }
}

// 解决：用 common_type 或显式指定返回类型
template<typename T>
std::common_type_t<T, double> f(T x) {
    if constexpr (std::is_integral_v<T>) {
        return x * 2;
    } else {
        return x * 2.0;
    }
}
```

## HFT 关联

```cpp
// 根据数据类型选择最优处理路径
template<typename T>
void process_column(T* data, size_t n) {
    if constexpr (std::is_same_v<T, double>) {
        // AVX2 路径
        process_avx2(data, n);
    } else if constexpr (std::is_same_v<T, float>) {
        // AVX2 float 路径
        process_avx2f(data, n);
    } else {
        // 标量路径
        process_scalar(data, n);
    }
}

// 条件编译调试代码
template<typename T>
void debug_print(const T& x) {
    if constexpr (DEBUG_MODE) {
        std::cout << "debug: " << x << "\n";
    }
    // DEBUG_MODE=false 时，这行代码不存在 → 零开销
}
```

## 小结

| C++14 技巧 | C++17 替代 |
|------------|-----------|
| `enable_if` | `if constexpr` |
| 标签分发 | `if constexpr` |
| 递归模板 + 特化 | `if constexpr` |
| 代码行数 | 大幅减少 |

---

← [上一节](01-if-constexpr基础.md) · [本章导读](./README.md)
