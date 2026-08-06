# A.6 constexpr

> 附录 A · 上一节：[A.5 可变参数模板](05-variadic.md) · 下一章：[附录 B.1 主流并发库对比](../appendix-b-library-comparison/01-libraries.md)

## 这节讲什么

`constexpr` 让函数和变量在编译期求值——计算移到编译期，运行时零开销。本节讲 `constexpr` 函数、`constexpr` 变量、C++14 的放宽、以及在 HFT 中的价值（编译期计算查表）。

---

## 核心规则（代码+表格）

### `constexpr` 变量

```cpp
// 编译期常量
constexpr int BUFFER_SIZE = 1024;  // 编译期已知
constexpr double PI = 3.14159265358979;

// 用于数组大小、模板参数等需要编译期常量的地方
char buffer[BUFFER_SIZE];  // OK
std::array<int, BUFFER_SIZE> arr;  // OK

// const vs constexpr
const int x = get_runtime_value();  // 运行期常量（第一次赋值后不变）
constexpr int y = 42;               // 编译期常量（编译时已知）
```

### `constexpr` 函数

```cpp
// C++11：constexpr 函数只能有 return 语句
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}
constexpr int f5 = factorial(5);  // 120，编译期计算

// C++14 放宽：可以有循环、局部变量、if 等
constexpr int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
constexpr int fib10 = fibonacci(10);  // 55，编译期计算

// C++14 constexpr 可以有多个 return，但不能有 goto、try-catch
```

### `constexpr` 的双重身份

```cpp
constexpr int square(int x) { return x * x; }

int runtime_n = get_input();
constexpr int compile_time = square(5);   // 编译期求值 → 25
int runtime_result = square(runtime_n);   // 运行期求值（也可以当普通函数用）
// constexpr 函数既能在编译期用，也能在运行期用
```

### 编译期查表（HFT 利器）

```cpp
// 编译期生成查找表，运行时零开销
constexpr int POW2[16] = {
    1, 2, 4, 8, 16, 32, 64, 128,
    256, 512, 1024, 2048, 4096, 8192, 16384, 32768
};
// 运行时：POW2[n] 直接查表，无计算

// C++14：编译期生成更复杂的表
constexpr std::array<int, 256> make_crc_table() {
    std::array<int, 256> table{};
    for (int i = 0; i < 256; ++i) {
        int crc = i;
        for (int j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        table[i] = crc;
    }
    return table;
}
// C++17 constexpr 可以操作 std::array
constexpr auto CRC_TABLE = make_crc_table();  // 编译期生成 256 项 CRC 表
// 运行时：CRC_TABLE[byte] 直接查表，无计算开销
```

### `if constexpr`（C++17）

```cpp
// 编译期 if：条件为 false 的分支不编译
template <typename T>
auto get_value(T t) {
    if constexpr (std::is_pointer_v<T>) {
        return *t;  // 只对指针类型编译
    } else {
        return t;   // 只对非指针类型编译
    }
}
// 不需要 SFINAE 或特化，代码更简洁
```

---

## 新手要点（和 C 的区别）

- **C 的 `const` 不保证编译期已知**：C 的 `const int x = rand();` 是合法的（运行期常量）。C++ 的 `constexpr` 明确保证编译期已知——这是 C++ 的进步。
- **C 用 `#define` 或 `enum` 做编译期常量**：C 程序员可能用 `#define SIZE 1024` 或 `enum { SIZE = 1024 };`——但宏无类型、enum 不能是浮点。C++ 的 `constexpr` 有类型安全。
- **编译期计算是 C 程序员陌生概念**：C 的计算都在运行期。C++ 的 `constexpr` 把计算移到编译期——运行时零开销，但编译时间增加。HFT 中常用于查表。
- **`if constexpr` 是 C++17 的新武器**：C 程序员如果用过 C++ 模板，可能记得 SFINAE 的痛苦。`if constexpr` 让编译期分支简洁——这是 C++ 模板编程的巨大改进。

---

## HFT 关联

- **编译期查表是 HFT 的常用优化**：HFT 中很多计算（如 CRC、哈希、价格档位映射）可以预计算成查找表——`constexpr` 让表在编译期生成，运行时直接查，零计算开销。
- **`constexpr` 消除运行时初始化**：HFT 系统启动时间要短——`constexpr` 常量在编译期就准备好，不需要运行时初始化。
- **`if constexpr` 用于泛型 HFT 代码**：HFT 的通用工具（如序列化、日志）对不同类型有不同处理——`if constexpr` 让一份代码处理多种类型，无运行时分支。
- **编译时间增加**：HFT 项目如果大量用 `constexpr`，编译时间可能显著增加——要权衡。查表用 `constexpr`，简单常量用 `const`。

---

## 自测题

1. `constexpr` 和 `const` 有什么区别？哪个保证编译期已知？
2. C++11 的 `constexpr` 函数有什么限制？C++14 放宽了什么？
3. `constexpr` 函数能在运行期调用吗？
4. `if constexpr`（C++17）和普通 `if` 有什么区别？
5. HFT 中用 `constexpr` 编译期查表有什么好处？

---

## 参考与延伸

- 下一章：[附录 B.1 主流并发库对比](../appendix-b-library-comparison/01-libraries.md)
- 上一节：[A.5 可变参数模板](05-variadic.md)
- 回到：[附录 A](README.md)
