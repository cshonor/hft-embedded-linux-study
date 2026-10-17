# 8.3 其他杂项语言特性

> 第 8 章 其他语言特性 · 上一节：[8.2 UTF-8 字面量与 noexcept](02-utf8字面量与noexcept.md)

## 这节讲什么

C++17 还有一些小的语言特性改进：`noexcept` 在类型系统中的变化、`__has_include` 预处理、带 init 的 namespace 枚举、十六进制浮点数字面量等。本节汇总这些小特性。

## __has_include

检查某个头文件是否存在（预处理期）：

```cpp
#if __has_include(<optional>)
    #include <optional>
    using std::optional;
#elif __has_include(<experimental/optional>)
    #include <experimental/optional>
    using std::experimental::optional;
#else
    // 自己实现或报错
    #error "No optional header available"
#endif
```

### 跨版本兼容

```cpp
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;       // C++17
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;  // C++14 实验性
#endif
```

## 十六进制浮点数字面量

```cpp
// C++17：十六进制浮点数
double a = 0x1.8p+1;    // 1.5 × 2^1 = 3.0
double b = 0xA.8p+4;    // 10.5 × 2^4 = 168.0
double c = 0x1p-1;      // 1.0 × 2^-1 = 0.5

// 格式：0x Mantissa . Fraction p Exponent
// p 表示 2 的幂（类似 e 表示 10 的幂）
```

### 为什么需要

```cpp
// 十进制浮点数：可能精度损失
double x = 0.1;  // 0.1 在二进制中是无限循环，存储的是近似值

// 十六进制浮点数：精确
double y = 0x1.999999999999ap-4;  // 精确表示 0.1 的 IEEE 754 双精度值

// 调试/校验时有用：打印十六进制浮点数可以精确比较
```

## 带初始化的 enum class

```cpp
// C++17：enum class 可以指定底层类型并初始化
enum class Status : int { OK = 0, WARN = 1, ERROR = 2 };

// 从整数显式转换
Status s = static_cast<Status>(0);  // OK

// C++17：可以用列表初始化
Status s2{0};  // C++17: OK（列表初始化，不窄化）
// Status s3 = 0;  // 编译错误：不能隐式转换
```

## namespace 嵌套简化

```cpp
// C++14：嵌套命名空间要逐层写
namespace A {
    namespace B {
        namespace C {
            void f();
        }
    }
}

// C++17：嵌套命名空间一行搞定
namespace A::B::C {
    void f();
}
```

## using 属性命名空间

（已在第 7 章第 3 节讲解，此处略）

## 结构化绑定与 bit-field

C++17 不支持位域的结构化绑定（已在第 1 章第 3 节讲解），但 C++17 增加了对 `[[maybe_unused]]` 在结构化绑定中的支持预告（实际 C++26 才支持）。

## static_assert 无消息

```cpp
// C++14：必须写消息
static_assert(sizeof(int) == 4, "int must be 4 bytes");

// C++17：可以省略消息
static_assert(sizeof(int) == 4);  // 编译器自动生成消息
```

## auto 推导为非指针

```cpp
// C++14：auto 推导可能意外退化为指针
int x = 42;
const int& rx = x;
auto a = rx;  // a 的类型是 int（丢失 const 和引用）

// C++17：decltype(auto) 保留引用和 const
decltype(auto) b = rx;  // b 的类型是 const int&
```

## 折叠表达式预告

折叠表达式是 C++17 的重要特性，但属于模板特性，在第 11 章详细讲解。这里只提一句：它让可变参数模板的展开变得简洁。

```cpp
// C++17 折叠表达式
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // 折叠表达式
}

sum(1, 2, 3, 4);  // 10
```

## HFT 关联

```cpp
// __has_include：跨版本兼容
#if __has_include(<string_view>)
    #include <string_view>
    using StringView = std::string_view;
#else
    using StringView = std::string;  // 降级
#endif

// namespace 嵌套简化
namespace hft::core::matching {
    void match_order();
}

// static_assert 无消息
template<typename T>
void process(T x) {
    static_assert(sizeof(T) <= 16);  // 简洁
    // ...
}

// 十六进制浮点数：精确常量
constexpr double LOG2_E = 0x1.71547652b82fep+0;  // log2(e) 的精确值
```

## 小结

| 特性 | 说明 |
|------|------|
| `__has_include` | 预处理期检查头文件 |
| 十六进制浮点数 | `0x1.8p+1` 格式 |
| `namespace A::B::C` | 嵌套命名空间简化 |
| `static_assert(cond)` | 无消息断言 |
| `decltype(auto)` | 保留引用和 const |

---

← [上一节](02-utf8字面量与noexcept.md) · [本章导读](./README.md)
