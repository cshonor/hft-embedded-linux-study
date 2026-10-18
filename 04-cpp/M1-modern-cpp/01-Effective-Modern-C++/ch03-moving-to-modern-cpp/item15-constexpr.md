# Item 15：尽可能用 constexpr

> 第 3 章 移步现代 C++ · Item 15 · 上一节：[Item 14 noexcept](item14-noexcept.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `#define` 或 `const` 定义常量：

```c
#define PI 3.14159          // 无类型，预处理替换
static const int BUF_SIZE = 4096;  // 有类型，但可能不是编译期常量

int arr[BUF_SIZE];   // 可能 OK，取决于编译器（标准 C 里这是 VLA）
```

两条路各有缺：`#define` 是编译期常量但**没有类型**，调试器看不见、作用域管不住；C 的 `const` 有类型但**不保证编译期求值**——`const int n = get();` 完全合法，值要等运行时才知道。

C++11 的 `constexpr` 把两者合一：**既有类型安全，又保证编译期求值**。而且它不只修饰变量，还能修饰函数——让函数在编译期跑完，运行时零开销。

最实用的一点：`constexpr` 是"**有能力**编译期求值"，不是"只在编译期求值"。同一个 `constexpr` 函数，传字面量就编译期算，传运行期变量就退化成普通函数调用。这意味着你不需要为编译期和运行期各写一份代码。

---

## 这节讲什么

1. `constexpr` 对象：`const` + 编译期初值，可用于数组维度、`static_assert`、模板参数；
2. `constexpr` 函数：C++11 的"单 return"限制 vs C++14 的放开（两标准分别编译的实测对比）；
3. `constexpr` 函数也能在运行期被调用；
4. `const` 与 `constexpr` 的精确区别，以及指针上的坑；
5. 编译期查表，以及 C++20 `consteval` / `constinit` 的前瞻。

---

## constexpr 对象：const + 编译期初值

```cpp
constexpr int MAX_ORDERS = 10000;     // 编译期常量
constexpr double PI = 3.14159;

int arr[MAX_ORDERS];                   // ① 数组大小
static_assert(MAX_ORDERS > 0);         // ② 编译期断言
template<int N> struct Buffer {};      // ③ 模板参数
Buffer<MAX_ORDERS> buf;                //    OK
```

三个位置都是"必须用编译期常量"的硬约束，是判断一个值到底是不是 `constexpr` 的**试金石**。反过来：如果 `MAX_ORDERS` 声明成 `const int` 但初值来自运行期函数，`int arr[MAX_ORDERS]` 按标准就不合法（clang 会退化成 VLA 扩展并给 `-Wvla-cxx-extension` 告警）。

---

## constexpr 函数：C++11 只能一行 return

C++11 规定 `constexpr` 函数体只能是**一条 `return` 语句**，不能声明局部变量、不能有 `if`/循环、不能有多条语句。想写分支？只能用三元表达式或递归：

```cpp
// C++11 合法写法
constexpr int abs11(int x) { return x < 0 ? -x : x; }
```

C++14 彻底放开：局部变量、多条语句、`if`、`for`/`while`、多个 `return`，还能修改自己创建的局部对象。于是"编译期算阶乘/生成查表"变得和写普通函数一样：

```cpp
constexpr int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; ++i) result *= i;
    return result;
}
static_assert(factorial(5) == 120);   // 编译期算出 120
```

### 两标准分别编译的实测对比

同一份代码，只换 `-std`：

| 编译命令 | 结果 |
|---------|------|
| `clang++ -std=c++14 -Wall` | 编译通过，`static_assert` 成立 |
| `clang++ -std=c++11 -Wall` | **编译失败** |

C++11 下的报错原文（clang 23.1.0）：

```
warning: variable declaration in a constexpr function is a C++14 extension [-Wc++14-extensions]
    5 |     int result = 1;
      |         ^
error: statement not allowed in constexpr function
    6 |     for (int i = 1; i <= n; ++i) result *= i;
      |     ^
1 warning and 1 error generated.
```

注意两者的差别：**局部变量声明只是 warning（clang 把它当 C++14 扩展接受），循环语句是硬 error**。所以在 `-std=c++11` 下写 `constexpr` 函数，你会看到一部分"好像能过"、一部分直接报错——这种半软半硬的状态最容易让人误判。

`if` 语句在 C++11 下也是"扩展"级：加 `-pedantic-errors` 后同样报 error：

```
error: use of this statement in a constexpr function is a C++14 extension [-Werror,-Wc++14-extensions]
error: multiple return statements in constexpr function is a C++14 extension [-Werror,-Wc++14-extensions]
```

**实践结论**：只要项目允许，用 C++14 或更高标准写 `constexpr`。在 C++11 下它只能当"能放进常量表达式的表达式宏"来用，写不了正经逻辑。

---

## constexpr 函数也能运行期调用

这是最容易误解的一点。`constexpr` 修饰的是函数"**可以**在编译期求值"的能力，不代表它只在编译期跑：

```cpp
constexpr int square(int x) { return x * x; }

constexpr int a = square(10);      // 编译期：a 是常量表达式
int n = 5;
int b = square(n);                 // 运行期：退化成普通函数调用
```

但是——一旦把运行期值用在**要求编译期常量**的语境里，编译器不会"退化"，而是直接报错：

```cpp
int n = 4;
int arr[factorial(n)];   // 数组维度必须是编译期常量
```

clang 23.1.0 告警（注意，这里是告警而不是 error，因为 clang 默认开了 VLA 扩展）：

```
warning: variable length arrays in C++ are a Clang extension [-Wvla-cxx-extension]
   11 |     int arr[factorial(n)];
      |             ^~~~~~~~~~~~
note: read of non-const variable 'n' is not allowed in a constant expression
note: declared here
```

可移植代码不该依赖 VLA——要么把 `n` 声明成 `constexpr`，要么改用 `std::vector`。

---

## const vs constexpr

| 维度 | `const` | `constexpr` |
|------|---------|-------------|
| 含义 | 初始化后不可修改 | 必须是编译期可求值的常量表达式 |
| 初值来源 | 可以是运行期值 | **必须**是编译期可求值 |
| 蕴含关系 | 不蕴含 `constexpr` | **蕴含**顶层 `const` |
| 可用位置 | 只读变量 | 数组维度、模板实参、`case` 标签、`static_assert` |

```cpp
int x = 42;
const int cx = x;         // OK：const，但值运行期才确定
// constexpr int ce = x;  // 编译失败：x 不是常量表达式
```

**指针上的坑**：`constexpr` 修饰的是**指针本身**，不是指向的对象：

```cpp
constexpr int* p = nullptr;   // ≡ int* const p（指针是 const）
// 而不是 const int* p（指向 const int）
```

想让指针指向编译期常量，得写两层：`constexpr const int* p = &some_constexpr_int;`

---

## 完整可运行例子

编译期常量 + 编译期生成查表 + 运行期调用，一次跑完（`-std=c++14 -Wall`）：

```cpp
#include <iostream>

// ---- 1. constexpr 函数：编译期能算，运行期也能跑 ----
constexpr int factorial(int n) {
    int r = 1;
    for (int i = 1; i <= n; ++i) r *= i;
    return r;
}

// ---- 2. constexpr 对象 = const + 编译期初值 ----
constexpr int kMaxLevels = factorial(4);      // 24，编译期确定
int arr[kMaxLevels];                          // 数组维度证据
static_assert(kMaxLevels == 24, "4! == 24");

// ---- 3. 编译期生成查表（原生数组，C++14 可用）----
struct TickTable {
    int v[8] = {};
    constexpr TickTable() {
        for (int i = 0; i < 8; ++i) v[i] = i * i;
    }
};
constexpr TickTable kt{};
static_assert(kt.v[7] == 49, "7^2 == 49");

int main(int argc, char**) {
    std::cout << "kMaxLevels = " << kMaxLevels << "\n";
    std::cout << "sizeof(arr)/sizeof(int) = " << sizeof(arr) / sizeof(int) << "\n";
    std::cout << "kt.v = ";
    for (int x : kt.v) std::cout << x << ' ';
    std::cout << "\n";
    // constexpr 函数传入运行期值 → 退化成普通函数
    int n = argc + 3;   // 运行期才知道（argc == 1 → n == 4）
    std::cout << "factorial(" << n << ") = " << factorial(n) << "  (runtime call)\n";
    return 0;
}
```

clang 23.1.0 实测输出：

```
kMaxLevels = 24
sizeof(arr)/sizeof(int) = 24
kt.v = 0 1 4 9 16 25 36 49
factorial(4) = 24  (runtime call)
```

`kMaxLevels` 能被用作数组维度、`static_assert` 的条件，说明它确实是编译期常量；而 `factorial(n)` 在 `n` 来自 `argc` 时照常运行——同一个函数，两种语境。

---

## 常见错误（新手踩坑）

1. **以为 `const` 就是 `constexpr`**。`const int n = get();` 完全合法，但它不是常量表达式，不能当数组维度。需要编译期常量就显式写 `constexpr`。
2. **在 C++11 下写多语句 `constexpr` 函数**。局部变量只是 `-Wc++14-extensions` 告警（容易误以为能过），`for`/`while` 直接 error。要么改写为单 `return` + 三元/递归，要么升级标准。
3. **`constexpr` 函数里调了非 `constexpr` 函数**。比如 `std::rand()`、`std::chrono::now()`——这会让函数在编译期语境下不可用，报错信息通常指向"不是常量表达式"。
4. **把 `constexpr int* p` 当成 `const int* p`**。`constexpr` 修饰的是指针自身（`int* const`），指向的对象不是 const。想指常量要写 `constexpr const int*`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 常量 | `#define` 或 `const` | `constexpr` | 类型安全 + 编译期求值 |
| 编译期计算 | `#define`（仅替换） | `constexpr` 函数 | 可写复杂逻辑 |
| 数组大小 | `#define MAX 100` | `constexpr int MAX = 100;` | 有类型 |
| 编译期断言 | 不适用（C 无） | `static_assert(cond, msg)` | 编译期检查 |

**一句话总结：** `constexpr` = `#define` 的编译期特性 + `const` 的类型安全。它修饰函数时表示"**能**在编译期求值"；只有当你把它用在要求常量表达式的位置（数组维度、模板实参、`static_assert`）时，编译期求值才会被强制发生。

---

## HFT 关联

- **编译期查表**：协议字段偏移、tick size 表、费率表用 `constexpr` 在编译期算好，放进只读数据段；运行时零计算、零初始化开销，也没有静态初始化顺序问题。数据连续、cache 友好。
- **`static_assert` 编译期校验**：`static_assert(sizeof(Order) == 64);` 确保订单结构体符合 cache 行对齐；`static_assert(TableSize == 256);` 校验查表长度。出错在编译期暴露，而不是变成线上的一次越界。
- **编译期溢出检查**：`static_assert(factorial(13) == 6227020800);` 实测 clang 直接报 `value 6227020800 is outside the range of representable values of type 'int'`——编译期算术会检查未定义行为，这是手写常量表拿不到的保护。
- **模板参数**：`Buffer<MAX_ORDERS>` 在编译期确定大小，编译器能生成最优代码（无间接寻址、可完全展开）。

---

## 自测题

**1. `const` 和 `constexpr` 的区别是什么？`constexpr` 蕴含 `const` 吗？**
<details><summary>答案</summary>
`const` 表达"初始化后不可修改"，初值可以在运行期确定；`constexpr` 表达"必须是编译期可求值的常量表达式"，可用于数组维度、模板实参、`case` 标签。`constexpr` **蕴含顶层 const**（constexpr 变量本身一定 const）；反过来不成立。指针上要注意：`constexpr int* p` ≡ `int* const p`。
</details>

**2. C++14 的 `constexpr` 函数比 C++11 强在哪里？**
<details><summary>答案</summary>
C++11 把函数体限制为"单条 `return` 语句"：不能声明局部变量、不能有 `if`/循环/多条语句，复杂逻辑只能靠递归或三元表达式硬凑；且 `constexpr` 成员函数隐含 `const`。C++14 全部放开：局部变量、多条语句、`if`、`for`/`while`、多个 `return`，还能修改自己创建的局部对象，成员函数也不再隐含 const。这让"编译期生成查表"真正可行。
</details>

**3. `constexpr` 函数传入运行期值会怎样？**
<details><summary>答案</summary>
在运行期求值，退化成一次普通函数调用，返回值不再是常量表达式——因此不能用在数组维度、模板实参、`case` 标签等要求编译期常量的位置（那些地方会报错或退化成 VLA 扩展并告警）。也就是说 `constexpr` 是"**有能力**编译期求值"，是否真的编译期求值取决于实参是否为常量表达式以及结果是否被用在需要常量的语境。注意函数体仍必须满足 constexpr 的语法约束，与实参无关。
</details>

**4. 为什么 HFT 喜欢用 `constexpr` 做查表？**
<details><summary>答案</summary>
表在编译期算好并放进只读数据段，运行时零计算、零初始化开销（没有静态初始化顺序问题、没有构造/加锁成本），数据布局连续、cache 友好，还能用 `static_assert` 在编译期校验表内容（表长、校验和），出错在编译期暴露。对延迟敏感的路径，"查表"这件事被完全移出运行时。
</details>

**5. `constexpr int* p` 和 `const int* p` 是一回事吗？**
<details><summary>答案</summary>
不是。`constexpr int* p` 等价于 `int* const p`——const 的是**指针本身**（不能改指向），指向的 int 仍可修改。`const int* p` 是"指向 const int 的指针"（不能通过 p 改值，但可改指向）。想让指针既不可改指向、指向的也不可改，写 `constexpr const int* p`。
</details>

**6. 下面代码能编译吗？**
```cpp
int x = 42;
const int cx = x;
constexpr int ce = x;
int arr1[cx];
int arr2[ce];
```
<details><summary>答案</summary>
不能，卡在第 3 行。`const int cx = x;` 合法（const 可以用运行期值初始化）；`constexpr int ce = x;` 非法：`x` 是非 const 局部变量，对它做左值到右值转换不属于常量表达式。正确写法是先把 `x` 声明为 `const int x = 42;`（或 `constexpr`），之后才能用它初始化 `constexpr` 变量。另外 `int arr1[cx];` 按标准也不合法——clang 会当成 VLA 扩展并给 `-Wvla-cxx-extension` 告警，可移植代码不该依赖。
</details>

**7. `constexpr` 函数在 C++11 下是不是完全不能写分支？**
<details><summary>答案</summary>
不是"完全不能"，而是不能用 `if`/`switch` 语句——必须用**三元条件表达式**或**递归**来模拟。例如 `constexpr int abs11(int x) { return x < 0 ? -x : x; }`、`constexpr int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }`。代价是代码可读性差、递归深度受编译器常量求值上限约束（clang 默认 `-fconstexpr-depth=512`）。所以 C++11 的 `constexpr` 函数适合写"表达式级"的小工具，不适合写算法。
</details>

---

## 参考与延伸

- 下一节：[Item 16 const 线程安全](item16-const-thread-safety.md)
- 回到：[第 3 章 移步现代 C++](README.md)
