# 8.2 UTF-8 字面量与 noexcept

> 第 8 章 其他语言特性 · 上一节：[8.1 表达式求值顺序](01-表达式求值顺序.md) · 下一节：[8.3 结构化异常处理与 init 语句](03-结构化异常处理与init语句.md)

## 这节讲什么

C++17 对 UTF-8 字面量和 `noexcept` 做了小幅改进：`u8` 前缀的字符字面量、`noexcept` 成为类型系统的一部分。

## UTF-8 字面量

### C++17 的变化

```cpp
// C++14：u8 字符串字面量
const char* s = u8"hello";        // UTF-8 编码的字符串

// C++17 新增：u8 字符字面量
char c = u8'A';                    // UTF-8 编码的单个字符
// 注意：ASCII 字符的 UTF-8 编码就是 ASCII，所以 u8'A' == 'A'
```

### C++20 的变化预告

```cpp
// C++20：u8 字符串字面量的类型变了
// C++17: const char*
const char* s17 = u8"hello";

// C++20: const char8_t*（新类型 char8_t）
const char8_t* s20 = u8"hello";   // C++20
// const char* s = u8"hello";     // C++20: 编译错误！类型不匹配
```

### 实际用法

```cpp
// 确保源文件中的非 ASCII 字符串用 UTF-8 编码
const char* msg = u8"交易完成";  // UTF-8 编码
const char* err = u8"错误：余额不足";

// 跨平台一致：Windows 默认可能是 GBK，u8 保证 UTF-8
```

## noexcept 作为类型的一部分

### C++14 的问题

```cpp
// C++14：noexcept 不是类型的一部分
void (*fp1)(int) noexcept;
void (*fp2)(int);

// fp1 和 fp2 的类型相同！noexcept 只是修饰
fp1 = fp2;  // C++14: OK（丢失 noexcept 信息）
```

### C++17 的变化

```cpp
// C++17：noexcept 是函数类型的一部分
void f(int) noexcept;
void g(int);

// f 和 g 的类型不同
void (*fp1)(int) noexcept = f;   // OK
void (*fp2)(int) = g;            // OK

// fp1 = fp2;  // C++17: 编译错误！不能把非 noexcept 赋给 noexcept
fp2 = fp1;     // C++17: OK（noexcept 可以退化成非 noexcept）
```

### 规则

- `noexcept` 函数指针可以赋值给非 `noexcept` 函数指针（安全退化）
- 非 `noexcept` 函数指针**不能**赋值给 `noexcept` 函数指针（编译错误）

```cpp
void may_throw(int);
void no_throw(int) noexcept;

void (*p1)(int) = may_throw;
void (*p2)(int) noexcept = no_throw;

p1 = p2;   // OK: noexcept → 非 noexcept
// p2 = p1; // 编译错误: 非 noexcept → noexcept
```

### 对模板和重载的影响

```cpp
// C++17：条件 noexcept 更精确
template<typename T>
void process(T x) noexcept(noexcept(T::process(x))) {
    T::process(x);
}

// std::function 的签名也受影响
std::function<void(int)> f1;
std::function<void(int) noexcept> f2;  // C++17: 不同类型
```

### type_traits 中的 noexcept

```cpp
// 检查函数是否 noexcept
template<typename T>
constexpr bool is_noexcept = noexcept(std::declval<T>()());

// 用法
auto lambda_noexcept = []() noexcept {};
auto lambda_throwing = []() {};

static_assert(is_noexcept<decltype(lambda_noexcept)>);
static_assert(!is_noexcept<decltype(lambda_throwing)>);
```

## HFT 关联

```cpp
// 标记热路径函数为 noexcept
// 1. 编译器可以省略异常处理代码 → 更快
// 2. 明确表示"不抛异常"的契约
[[gnu::hot]] void on_tick(const Tick& t) noexcept {
    // 热路径不抛异常
    process(t);
}

// 函数指针类型安全
using Callback = void(*)(const Tick&, void*) noexcept;

// 回调表只接受 noexcept 函数
void register_callback(Callback cb, void* ctx);

// 非 noexcept 函数不能注册
void bad_callback(const Tick&, void*);
// register_callback(bad_callback, nullptr);  // 编译错误！
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| `u8'A'` 字符字面量 | ❌ | ✅ |
| `u8"..."` 类型 | `const char*` | `const char*`（C++20 变 `char8_t*`） |
| `noexcept` 是类型一部分 | ❌ | ✅ |
| noexcept → 非 noexcept 赋值 | ✅ | ✅ |
| 非 noexcept → noexcept 赋值 | ✅ | ❌（编译错误） |

---

← [上一节](01-表达式求值顺序.md) · [下一节 →](03-结构化异常处理与init语句.md)
