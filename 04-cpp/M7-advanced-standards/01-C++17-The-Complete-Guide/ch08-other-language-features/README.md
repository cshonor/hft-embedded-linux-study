# 第 8 章 其他语言特性

**Other Language Features**

## 本章讲什么

C++17 的一些小但实用的语言改进：`__has_include`、嵌套 namespace 简写、`noexcept` 成为类型系统的一部分、UTF-8 字符字面量、十六进制浮点字面量。

## 要点

### 嵌套 namespace 简写

```cpp
// C++14
namespace A { namespace B { namespace C {
    void foo();
}}}

// C++17
namespace A::B::C {
    void foo();
}
```

### `__has_include`

```cpp
#if __has_include(<optional>)
    #include <optional>
    #define HAS_OPTIONAL 1
#elif __has_include(<experimental/optional>)
    #include <experimental/optional>
#else
    #define HAS_OPTIONAL 0
#endif
```

预处理期检测头文件是否存在，用于跨版本/跨平台兼容。

### `noexcept` 进入类型系统

```cpp
void f() noexcept;        // C++17：noexcept 是类型的一部分
void (*p1)() noexcept = f;  // OK
void (*p2)() = f;           // C++17 错误！noexcept 函数不能赋给非 noexcept 指针
```

C++14 里 `noexcept` 只是规范不算类型，C++17 让函数指针的 `noexcept` 成为类型的一部分——`void(*)()` 和 `void(*)() noexcept` 是不同类型。

影响：模板推导、函数指针赋值要考虑 noexcept。`std::function` 仍不区分（C++17 没改）。

### UTF-8 字符字面量

```cpp
char c = 'A';           // char
char c2 = u8'A';        // C++17：UTF-8 字符字面量（char 类型）
const char* s = u8"你好";  // UTF-8 字符串字面量（const char*，C++17 起明确）
```

C++17 的 `u8'A'` 是 `char`（C++20 改为 `char8_t`）。

### 十六进制浮点字面量

```cpp
double d = 0x1.8p1;   // 1.5 × 2^1 = 3.0
// p 表示 2 的幂（不是 e，e 是十进制指数）
```

精确指定浮点位模式，避免十进制转换的精度损失。

### 结构化绑定中 `[[maybe_unused]]`

C++17 对未使用的绑定变量仍不支持 `[[maybe_unused]]`（C++26 才支持），但可用 `std::ignore` 或 `_` 惯例。

## HFT 关联

- **嵌套 namespace**：`namespace hft::feed::l2 { ... }` 替代三层嵌套，代码更清晰。
- **`__has_include` 做编译器适配**：`#if __has_include(<version>)` 检测 C++20 头，做版本降级。
- **`noexcept` 类型化**：HFT 模板库用 `noexcept` 约束回调类型，编译期保证不抛异常（热路径不能异常）。
- **十六进制浮点**：精确指定策略参数的浮点位，避免 `0.1` 的十进制转换误差。
- **UTF-8 字面量**：日志/监控字符串用 `u8""` 明确编码，跨平台不乱码。

## 自测题

1. `namespace A::B::C` 等价于什么旧写法？
2. `__has_include` 在什么阶段求值？解决什么问题？
3. C++17 让 `noexcept` 进入类型系统，对函数指针赋值有什么影响？
4. 十六进制浮点 `0x1.8p1` 的值是多少？`p` 表示什么？
5. HFT 热路径为什么关心 `noexcept` 成为类型的一部分？

## 代码自测

### Q1: 嵌套命名空间
```cpp
// C++14
namespace A { namespace B { namespace C { void f() {} } } }

// C++17
namespace A::B::C { void f() {} }
```
> 还有哪些 C++17 小特性简化了日常写法？

<details>
<summary>答案与复习指引</summary>

C++17 小特性：
1. **嵌套命名空间**：`namespace A::B::C { ... }`
2. **`static_assert` 无消息**：`static_assert(sizeof(int) == 4);`
3. **`noexcept` 成为类型系统一部分**：`void(*)() noexcept` 和 `void(*)()` 是不同类型
4. **`bool` 不能再 implicit narrowing**：`int x = true;` → 需显式转换（某些上下文）
5. **十六进制浮点字面量**：`0x1.8p+1` = 3.0
6. **UTF-8 字符字面量**：`u8'x'`（C++17 起，C++20 改为 char8_t）

**复习：** → [其他语言特性](./README.md)
</details>
