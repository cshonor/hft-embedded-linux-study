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
