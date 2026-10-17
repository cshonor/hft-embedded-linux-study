# 10.1 if constexpr 基础

> 第 10 章 编译期 if · 下一节：[10.2 替代 SFINAE 与标签分发](02-替代SFINAE与标签分发.md)

## 这节讲什么

`if constexpr` 是 C++17 最重要的特性之一：编译期条件分支。条件为假的分支**不会被编译**——不是运行时跳过，是编译期直接丢弃。这替代了大量复杂的 SFINAE 和标签分发。

## 为什么要学这个（先建立直觉）

C 程序员的条件编译：

```c
// C：用 #ifdef 做条件编译
#ifdef USE_FLOAT
    float process(float x) { return x * 2.0f; }
#else
    double process(double x) { return x * 2.0; }
#endif
```

C++14 模板里的条件分支：

```cpp
// C++14：运行时 if，两个分支都要编译通过
template<typename T>
void process(T x) {
    if (std::is_integral<T>::value) {
        std::cout << x << " is integral\n";
        // 如果 T 是 string，这行也能编译（cout << string 没问题）
    } else {
        std::cout << x << " is not integral\n";
    }
}

// 但如果分支里有类型不兼容的操作：
template<typename T>
void process(T x) {
    if (std::is_integral<T>::value) {
        return x * 2;        // string 类型没有 * 2！编译错误！
    } else {
        return x + " suffix";
    }
}
// 编译器会尝试编译两个分支 → 类型不兼容时报错
```

C++17 `if constexpr`：

```cpp
// C++17：编译期 if，false 分支不编译
template<typename T>
auto process(T x) {
    if constexpr (std::is_integral_v<T>) {
        return x * 2;         // 只有 T 是整数时才编译这行
    } else {
        return x + " suffix"; // 只有 T 是 string 时才编译这行
    }
}

process(42);              // 返回 84
process(std::string("a")); // 返回 "a suffix"
```

## 语法

```cpp
if constexpr (编译期布尔表达式) {
    // 条件为 true 时编译
} else {
    // 条件为 false 时编译
}

// 可以没有 else
if constexpr (cond) { ... }

// 可以链式
if constexpr (cond1) { ... }
else if constexpr (cond2) { ... }
else { ... }
```

## 关键区别：if vs if constexpr

| 特性 | `if` | `if constexpr` |
|------|------|----------------|
| 条件求值 | 运行时 | 编译期 |
| false 分支 | 编译但不执行 | **不编译** |
| 要求分支合法 | 是（都要编译通过） | 否（false 分支可以类型不合法） |
| 生成代码 | 两个分支都有 | 只有 true 分支 |

## 常见用法

### 1. 类型条件分支

```cpp
template<typename T>
auto get_value(T x) {
    if constexpr (std::is_pointer_v<T>) {
        return *x;           // 解引用
    } else {
        return x;            // 直接返回
    }
}
```

### 2. 容器类型判断

```cpp
template<typename T>
void process(T& container) {
    if constexpr (std::is_same_v<typename T::value_type, int>) {
        // 处理 int 容器
        for (auto& x : container) x *= 2;
    } else if constexpr (std::is_same_v<typename T::value_type, std::string>) {
        // 处理 string 容器
        for (auto& x : container) x += "!";
    }
}
```

### 3. 递归终止（替代模板特化）

```cpp
// C++14：递归模板 + 特化终止
template<int N>
struct Factorial {
    static constexpr int v = N * Factorial<N-1>::v;
};
template<>
struct Factorial<0> { static constexpr int v = 1; };

// C++17：if constexpr 一步到位
template<int N>
constexpr int factorial() {
    if constexpr (N == 0) return 1;
    else return N * factorial<N-1>();
}

static_assert(factorial<5>() == 120);
```

### 4. variant 访问

```cpp
template<typename T>
void visit(T&& var) {
    if constexpr (std::is_same_v<std::decay_t<T>, OrderMsg>) {
        handle_order(var);
    } else if constexpr (std::is_same_v<std::decay_t<T>, TradeMsg>) {
        handle_trade(var);
    }
}
```

## HFT 关联

```cpp
// 热路径零开销分支
template<bool SIMD>
void process_batch(const double* data, size_t n) {
    if constexpr (SIMD) {
        // AVX2 路径：只有 SIMD=true 时编译
        for (size_t i = 0; i < n; i += 4) {
            __m256d v = _mm256_load_pd(data + i);
            v = _mm256_mul_pd(v, _mm256_set1_pd(2.0));
            _mm256_store_pd(const_cast<double*>(data + i), v);
        }
    } else {
        // 标量路径：只有 SIMD=false 时编译
        for (size_t i = 0; i < n; ++i) {
            const_cast<double&>(data[i]) *= 2.0;
        }
    }
}

// 编译期选择最优路径
process_batch<true>(data, n);   // AVX2 版本
process_batch<false>(data, n);  // 标量版本
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| 编译期条件 | SFINAE / 标签分发 | `if constexpr` |
| false 分支 | 必须编译通过 | 不编译 |
| 递归终止 | 模板特化 | `if constexpr` |
| 代码可读性 | 差 | 好 |

---

← [本章导读](./README.md) · [下一节 →](02-替代SFINAE与标签分发.md)
