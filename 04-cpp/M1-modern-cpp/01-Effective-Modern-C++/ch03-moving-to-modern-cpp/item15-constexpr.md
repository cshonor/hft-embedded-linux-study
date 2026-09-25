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

<details>
<summary>参考答案</summary>

1. `const` 表达"这个对象初始化后不可修改"，它的初值可以在运行期才确定（`const int n = get();` 合法）。`constexpr` 表达"这个值必须是编译期可求值的常量表达式"，可以用于数组长度、模板实参、`case` 标签等要求编译期常量的位置。`constexpr` **蕴含** 顶层 `const`：一个 `constexpr` 变量本身必然是 const 的；反过来不成立——`const` 变量不一定是常量表达式。注意指针上的差异：`constexpr int* p` 等价于 `int* const`（const 作用于指针本身），而不是 `const int*`。

2. C++11 的 `constexpr` 函数函数体被严格限制为"单条 `return` 语句"（不能声明局部变量、不能有 `if`/循环/多条语句），复杂逻辑只能靠递归或三元表达式硬凑；且 `constexpr` 成员函数隐含 `const`。C++14 大幅放宽：允许局部变量、多条语句、`if`、`for`/`while`、多个 `return`，还能修改自己创建的局部对象，写起来接近普通函数；`constexpr` 成员函数也不再隐含 const。这让"编译期查表/循环生成表"变得实际可行。

3. 会在**运行期**求值，`constexpr` 函数退化成一次普通函数调用，返回值不再是常量表达式——因此不能用作数组长度、模板实参、`case` 标签等要求编译期常量的位置（那些地方会编译失败）。也就是说 `constexpr` 是"**有能力**在编译期求值"，而不是"只在编译期求值"；是否编译期求值取决于实参是否为常量表达式以及结果是否被用在需要常量表达式的语境。此外，函数体仍必须满足 constexpr 的语法约束，无论实参是不是常量。

4. 因为 `constexpr` 表在编译期就算好并放进只读数据段，运行时是零计算、零初始化开销（没有静态初始化顺序问题、没有构造/加锁成本），数据布局连续、cache 友好，还能用 `static_assert` 在编译期校验表内容（比如表长、校验和），出错在编译期暴露。对延迟敏感的路径，"查表"这件事被完全移出运行时。

5. 编译不过，卡在第 3 行。`const int cx = x;` 合法（`const` 可以用运行期值初始化）；但 `constexpr int ce = x;` 非法：`x` 是非 const 的局部变量，对它做左值到右值转换不属于常量表达式，编译器无法在编译期确定 `ce`。因此正确写法是 `const int x = 42;`（或 `constexpr int x = 42;`）之后才能 `constexpr int ce = x;`。另外 `int arr1[cx];` 按标准也不合法——`cx` 不是常量表达式，数组长度必须是编译期常量；clang/GCC 会把它当成 VLA（变长数组）扩展接受并给出警告（clang 为 `-Wvla-cxx-extension`，提示"cx 的初始化器不是常量表达式"），属于非标准行为，可移植代码里不该依赖。若 `ce` 能编译，`int arr2[ce];` 是合法的。

</details>

---

## 参考与延伸

- 下一节：[Item 16 const 线程安全](item16-const-thread-safety.md)
- 回到：[第 3 章 移步现代 C++](README.md)
