# 13.1 auto 作非类型模板参数

> 第 13 章 auto 作模板参数

## 这节讲什么

C++17 允许 `auto` 作为非类型模板参数（NTTP）的占位符——编译器从实参推导参数类型。C++14 只允许 `int`、`bool` 等特定类型作 NTTP，C++17 放宽到任意 literal type。

## C++14 的限制

```cpp
// C++14：NTTP 必须是具体类型
template<int N> struct Array { int data[N]; };
template<bool B> struct EnableIf {};
template<typename T, T V> struct Constant { static constexpr T value = V; };

// 不能用 auto
// template<auto N> struct Array { ... };  // C++14: 不允许
```

## C++17 的 auto NTTP

```cpp
// C++17：auto 推导 NTTP 类型
template<auto N>
struct Constant {
    static constexpr auto value = N;
};

Constant<42> c1;          // N=int, value=42
Constant<3.14> c2;        // N=double, value=3.14
Constant<'A'> c3;         // N=char, value='A'
Constant<true> c4;        // N=bool, value=true
```

### 不同类型的实参生成不同的模板实例

```cpp
Constant<42> a;       // Constant<int>
Constant<42U> b;      // Constant<unsigned>
Constant<42L> c;      // Constant<long>
// a, b, c 是三种不同的类型！
```

## 实际用法

### 1. 通用常量

```cpp
template<auto Value>
struct Const {
    static constexpr auto value = Value;
};

using MaxSize = Const<65536>;
using Pi = Const<3.14159265358979>;
using ExchangeName = Const<'N'>;  // char
```

### 2. 类型安全的枚举值

```cpp
enum class Color { Red, Green, Blue };

template<auto C>
struct ColorTrait {
    static constexpr Color color = C;
    static const char* name() {
        if constexpr (C == Color::Red) return "Red";
        else if constexpr (C == Color::Green) return "Green";
        else return "Blue";
    }
};

ColorTrait<Color::Red>::name();   // "Red"
ColorTrait<Color::Blue>::name();  // "Blue"
```

### 3. 函数指针作 NTTP

```cpp
// C++17：函数指针可以作 NTTP
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

template<auto Op>
int compute(int a, int b) {
    return Op(a, b);
}

compute<add>(3, 4);  // 7
compute<sub>(3, 4);  // -1
```

### 4. 配合 if constexpr

```cpp
template<auto Mode>
void process(int* data, size_t n) {
    if constexpr (Mode == "sort") {
        std::sort(data, data + n);
    } else if constexpr (Mode == "reverse") {
        std::reverse(data, data + n);
    }
}
// 注意：字符串字面量不能直接作 NTTP，这里只是示意
// 实际用枚举或 StringLiteral
```

## auto NTTP 的类型限制

NTTP 的类型必须是 **literal type**：
- 标量类型（int, double, 指针, 枚举等）
- 引用类型
- 有 constexpr 构造函数的类类型（如 `std::string_view` 在 C++20）

```cpp
// ✅ 标量类型
template<auto N> struct A {};
A<42> a1;
A<3.14> a2;
A<&global_var> a3;

// ✅ 枚举
enum class Mode { Fast, Slow };
A<Mode::Fast> a4;

// ✅ 函数指针
A<&some_function> a5;

// ❌ 非字面量类型
// A<std::string("hello")> a6;  // 编译错误：string 不是 literal type（C++17）
// A<std::vector<int>{}> a7;     // 编译错误
```

## decltype(auto) NTTP

```cpp
// C++17：也可以用 decltype(auto)
template<decltype(auto) N>
struct D {
    static constexpr auto value = N;
};

int x = 42;
D<x> d1;  // N=int, value=42（注意：x 必须是编译期常量）
```

## HFT 关联

```cpp
// 编译期绑定配置
template<auto BufSize>
class RingBuffer {
    alignas(64) char buf_[BufSize];
    // ...
};

RingBuffer<65536> rb1;     // 64KB 缓冲区
RingBuffer<131072> rb2;    // 128KB 缓冲区

// 编译期绑定函数指针（策略分发）
template<auto Matcher>
class OrderMatcher {
public:
    bool match(const Order& a, const Order& b) {
        return Matcher(a, b);  // 编译期绑定，无间接调用
    }
};

bool price_match(const Order&, const Order&);
bool fifo_match(const Order&, const Order&);

OrderMatcher<price_match> pm;
OrderMatcher<fifo_match> fm;
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| `int` NTTP | ✅ | ✅ |
| `auto` NTTP | ❌ | ✅ |
| 函数指针 NTTP | ✅ | ✅ |
| 类类型 NTTP | 有限 | 放宽 |
| 类型推导 | 需显式 | auto 自动推导 |

---

← [本章导读](./README.md)
