# C++20 constexpr 增强

## constexpr 函数改进

```cpp
// C++20：constexpr 函数可以包含更多
constexpr int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; ++i) {  // 循环
        result *= i;
    }
    return result;
}

// C++20：constexpr 可以用 try/catch（但不能抛异常）
constexpr int safe_div(int a, int b) {
    if (b == 0) return 0;
    return a / b;
}

// C++20：constexpr 可以使用 std::string 和 std::vector
// （但需要在编译期销毁，不能跨编译期/运行期）
constexpr int sum_vector() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int x : v) sum += x;
    return sum;  // v 在函数结束时销毁
}
static_assert(sum_vector() == 15);
```

## consteval：立即函数

```cpp
// consteval：必须在编译期执行（不像 constexpr 可以运行期执行）
consteval int compile_time_only(int n) {
    return n * 2;
}

int x = compile_time_only(21);  // ✅ 编译期
// int y = compile_time_only(get_runtime_val());  // ❌ 运行期值不能传入

// constexpr 可以运行期
constexpr int maybe_compile(int n) { return n * 2; }
int z = maybe_compile(21);          // 编译期或运行期
int w = maybe_compile(get_runtime()); // 运行期
```

## constinit

```cpp
// constinit：变量必须有常量初始化（但不是 const）
constinit int counter = 0;  // 编译期初始化，但可运行期修改

// 对比
constexpr int a = 42;  // 编译期初始化 + 不可修改
constinit int b = 42;  // 编译期初始化 + 可修改
int c = func();        // 运行期初始化（可能静态初始化顺序问题）

// 用途：避免静态初始化顺序问题
// constinit 保证在所有其他静态变量之前初始化
```

## constexpr 容器和算法

```cpp
// C++20：更多 STL 在 constexpr 中可用
constexpr bool is_sorted_constexpr() {
    int arr[] = {1, 2, 3, 4, 5};
    return std::is_sorted(arr, arr + 5);
}
static_assert(is_sorted_constexpr());

// constexpr std::string
constexpr size_t str_len() {
    std::string s = "hello";
    return s.size();
}
static_assert(str_len() == 5);
```

## HFT 应用

```cpp
// 编译期计算配置表
consteval auto build_symbol_table() {
    std::array<std::pair<std::string_view, int>, 3> table{{
        {"AAPL", 1},
        {"GOOG", 2},
        {"MSFT", 3},
    }};
    return table;
}

// 编译期生成查找表
constexpr auto symbol_table = build_symbol_table();

// 编译期校验
consteval bool validate_config() {
    // 检查配置一致性
    return true;
}
static_assert(validate_config());
```

## 自测题

1. `constexpr`、`consteval`、`constinit` 的区别？
2. C++20 的 `constexpr` 函数可以包含什么？（循环、try/catch、vector...）
3. `consteval` 函数能在运行期调用吗？
4. `constinit` 解决什么问题？
5. HFT 中如何用 `consteval` 做编译期配置表？

<details>
<summary>参考答案</summary>

1. 三者层次不同：
   - **`constexpr`**（C++11）：修饰**函数**表示"可以在常量表达式中求值"（但也可以在运行时调用）；修饰**变量**表示它是编译期常量。
   - **`consteval`**（C++20）：**立即函数**，它的**每一次调用都必须**产生编译期常量，否则直接编译错误；因此它根本不会在运行期被调用，也不能取地址/做函数指针。
   - **`constinit`**（C++20）：修饰**变量**，强制该变量必须**常量初始化**（静态初始化阶段完成），用来消灭动态初始化的顺序问题；它**不**要求变量是 `const`，也不要求类型是字面类型。
一句话：`constexpr` 是"能用于编译期"，`consteval` 是"只能用于编译期"，`constinit` 是"必须在编译期初始化"。
2. C++20 的 `constexpr` 函数体已经相当接近普通函数，可以包含：
   - 所有控制流：`if`、`switch`、以及全部循环（`for`、range-`for`、`while`、`do-while`）；
   - **`try` / `catch`**（C++20 新增，但不允许真的 `throw` 出常量求值）；
   - 局部变量（含可变局部变量）、`static_assert`；
   - `new` / `delete`（临时分配必须在常量求值结束前释放）；
   - **`std::vector` / `std::string`** 等 constexpr 容器，以及 `<algorithm>`、`<numeric>` 中大部分算法（C++20 起大量算法也变成 constexpr）；
   - `dynamic_cast` / `typeid`、**虚函数调用**（C++20 新增）。
仍不允许：`goto`、非字面类型的变量、未初始化的变量、`asm`、以及调用任何非 constexpr 函数。
3. **不能**。`consteval` 函数（immediate function）的任何调用都必须在常量表达式语境中完成——一旦编译器发现某次调用无法在编译期求值，就是**编译错误**（而不是退化成运行期调用）。
这也意味着不能取它的地址、不能把它赋给函数指针。若只想要"能编译期求值但不强制"，就用 `constexpr`。
4. 它解决**静态初始化顺序问题（Static Initialization Order Fiasco）**：命名空间作用域或静态存储期的变量，如果初始化不是常量初始化，就会变成运行期的动态初始化，而**跨翻译单元的初始化顺序是未定义的**——A 的初始化用到 B 时，B 可能还没初始化。
加上 `constinit` 后，编译器**强制**该变量必须常量初始化（否则编译错误），从而保证它在程序开始前就已完成初始化，彻底消除顺序不确定性：
```cpp
constinit int counter = 0;                 // ✅ 常量初始化
// constinit int x = runtime_value();     // ❌ 编译错误（这正是它的价值）
```
它不要求 `const`（变量仍可变），也不要求字面类型——只保证"初始化是编译期完成的"。
5. 把配置表在编译期算好，运行时只读，零初始化成本：
```cpp
consteval std::array<double, 256> make_fee_table() {
    std::array<double, 256> t{};
    for (std::size_t i = 0; i < t.size(); ++i)
        t[i] = /* 手续费档位公式 */;
    return t;
}
constinit auto fee_table = make_fee_table();   // 编译期即完成，无动态初始化

inline double fee(std::uint8_t tier) { return fee_table[tier]; }  // 运行期只是一次查表
```
好处：表在编译期生成（可用循环、`std::array`、算法），放进只读段；启动阶段**没有任何**初始化代码，也没有初始化顺序风险；运行期只是一次数组索引。

</details>
