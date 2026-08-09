# Item 15：尽可能用 constexpr

> 第 3 章 移步现代 C++ · Item 15 · 上一节：[Item 14 noexcept](item14-noexcept.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `#define` 或 `const` 定义常量：

```c
#define PI 3.14159          // 无类型，预处理替换
#define MAX_ORDERS 10000
static const int BUF_SIZE = 4096;  // 有类型，但可能不是编译期常量

int arr[PI];         // 编译失败！PI 不是整数常量表达式
int arr[MAX_ORDERS]; // OK（#define 是字面量）
int arr[BUF_SIZE];   // 可能 OK，取决于编译器
```

C 的 `#define` 是编译期常量但无类型安全。C 的 `const` 有类型但不保证编译期求值。C++11 的 `constexpr` 把两者合一——既有类型安全，又保证编译期求值：

```cpp
constexpr double PI = 3.14159;
constexpr int MAX_ORDERS = 10000;

int arr[MAX_ORDERS];   // OK！constexpr 是编译期常量
static_assert(PI > 3.0, "PI should be > 3");  // 编译期断言
```

`constexpr` 不仅能修饰变量，还能修饰函数——让函数在编译期求值：

```cpp
constexpr int square(int x) { return x * x; }
constexpr int sz = square(10);  // 编译期算出 100，运行时零开销
```

---

## 这节讲什么

`constexpr` 表示"编译期可求值"。`constexpr` 对象是编译期常量；`constexpr` 函数在编译期能求值时就编译期求值，否则退化为运行时。C++14 起 `constexpr` 函数能力大增。

---

## 核心用法

### constexpr 变量

```cpp
constexpr int MAX_ORDERS = 10000;     // 编译期常量
constexpr double PI = 3.14159;

// 可用于：
int arr[MAX_ORDERS];                   // 数组大小
static_assert(MAX_ORDERS > 0);         // 编译期断言
template<int N> struct Buffer {};      // 模板参数
Buffer<MAX_ORDERS> buf;                // OK
```

### constexpr 函数

```cpp
// C++11 constexpr 函数：只能一行 return
constexpr int square(int x) { return x * x; }
constexpr int sz = square(10);     // sz = 100，编译期确定

// C++14 起 constexpr 函数可用 if/循环/局部变量
constexpr int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; ++i) result *= i;
    return result;
}
constexpr int f5 = factorial(5);   // 编译期算出 120

// 传入运行时值 → 退化为普通函数（运行时执行）
int runtime_val = std::atoi(argv[1]);
int result = factorial(runtime_val);  // 运行时执行，和非 constexpr 函数一样
```

### const vs constexpr

```cpp
int x = 42;
const int cx = x;         // const 但不是编译期常量（值在运行时确定）
constexpr int ce = 42;    // constexpr，编译期常量

// const 表示"不可修改"（但值可能运行时确定）
// constexpr 表示"编译期确定"（蕴含 const）
// constexpr 一定是 const，const 不一定是 constexpr

int arr1[cx];   // 可能编译失败（cx 不是编译期常量）
int arr2[ce];   // OK（ce 是编译期常量）
```

`constexpr` 对象 → 编译期常量，可用于模板参数、`static_assert`、数组大小。
`constexpr` 函数 → 至少有一个实参集能在编译期求值；传入运行时值则退化为普通函数。

---

## 常见错误（新手踩坑）

**错误 1：以为 const 等于 constexpr**
```cpp
const int size = get_size();  // 运行时确定，不是编译期常量
int arr[size];                // 编译失败！
```
**修正：** 用 `constexpr` 或确保常量在编译期可求值。

**错误 2：C++11 constexpr 函数写了多行**
```cpp
// C++11：只能一行 return
constexpr int abs_val(int x) {
    if (x < 0) return -x;  // C++11 编译失败！
    return x;
}
// C++14：OK
```
**修正：** C++11 用 `return x < 0 ? -x : x;`，或升级到 C++14。

**错误 3：constexpr 函数调了非 constexpr 函数**
```cpp
constexpr int get_val() {
    return std::rand();  // rand() 不是 constexpr！
}
// 编译失败
```
**修正：** `constexpr` 函数只能调其他 `constexpr` 函数。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 常量 | `#define` 或 `const` | `constexpr` | 类型安全 + 编译期求值 |
| 编译期计算 | `#define`（仅替换） | `constexpr` 函数 | 可写复杂逻辑 |
| 数组大小 | `#define MAX 100` | `constexpr int MAX = 100;` | 有类型 |
| 编译期断言 | 不适用（C 无） | `static_assert(cond, msg)` | 编译期检查 |

**一句话总结：** C 程序员记住——`constexpr` 是 `#define` + `const` 的合体：有类型安全，保证编译期求值，还能修饰函数。`const` 表示"不可改"，`constexpr` 表示"编译期确定"。

---

## HFT 关联

- **编译期查表**：协议字段偏移、校验和表、费率表用 `constexpr` 编译期算好，运行时零开销。
- **`static_assert` 编译期校验**：`static_assert(sizeof(Order) == 64);` 确保订单结构体大小符合 cache 行对齐要求。
- **模板参数**：`constexpr` 常量可做模板参数——`Buffer<MAX_ORDERS>` 在编译期确定大小，编译器生成最优代码。

---

## 自测题

1. `const` 和 `constexpr` 的区别是什么？`constexpr` 蕴含 `const` 吗？
2. C++14 的 `constexpr` 函数比 C++11 强在哪里？
3. `constexpr` 函数传入运行时值会怎样？
4. 为什么 HFT 喜欢用 `constexpr` 做查表？
5. 下面代码能编译吗？
```cpp
int x = 42;
const int cx = x;
constexpr int ce = x;
int arr1[cx];
int arr2[ce];
```

---

## 参考与延伸

- 下一节：[Item 16 const 线程安全](item16-const-thread-safety.md)
- 回到：[第 3 章 移步现代 C++](README.md)
