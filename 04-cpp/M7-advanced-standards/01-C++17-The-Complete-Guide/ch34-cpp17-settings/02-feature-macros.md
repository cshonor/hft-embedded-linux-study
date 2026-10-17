# 特性特征宏

## 版本检测

```cpp
// 方法1：__cplusplus（MSVC 不准）
#if __cplusplus >= 201703L
    // C++17 或更高
#endif

// 方法2：_MSVC_LANG（MSVC 专用）
#if defined(_MSVC_LANG)
    #if _MSVC_LANG >= 201703L
        // MSVC C++17
    #endif
#else
    #if __cplusplus >= 201703L
        // GCC/Clang C++17
    #endif
#endif

// 方法3：统一宏
#if (defined(_MSVC_LANG) ? _MSVC_LANG : __cplusplus) >= 201703L
    // C++17
#endif
```

## 特性特征宏

```cpp
// 语言特性
__cpp_structured_bindings      // 结构化绑定
__cpp_if_constexpr             // if constexpr
__cpp_fold_expressions         // 折叠表达式
__cpp_template_auto            // auto 模板参数
__cpp_inline_variables         // inline 变量
__cpp_nontype_auto             // auto NTTP

// 库特性
__cpp_lib_variant              // std::variant
__cpp_lib_optional             // std::optional
__cpp_lib_any                  // std::any
__cpp_lib_byte                 // std::byte
__cpp_lib_string_view          // std::string_view
__cpp_lib_filesystem           // <filesystem>
__cpp_lib_parallel_algorithm   // 并行 STL
__cpp_lib_to_chars             // to_chars/from_chars
__cpp_lib_pmr                  // std::pmr
__cpp_lib_type_trait_variable_templates  // _v 变量模板
```

## 使用示例

```cpp
// 条件编译：有 to_chars 就用，没有就降级
#ifdef __cpp_lib_to_chars
    auto [ptr, ec] = std::to_chars(buf, buf+32, val);
#else
    snprintf(buf, sizeof(buf), "%d", val);
#endif

// 检测结构化绑定
#ifdef __cpp_structured_bindings
    auto [x, y] = get_pair();
#else
    auto p = get_pair();
    auto x = p.first;
    auto y = p.second;
#endif
```

## 检查编译器支持矩阵

```cpp
// 打印支持的特性
#include <version>  // C++20，C++17 用 <ciso646>

void print_features() {
#ifdef __cpp_structured_bindings
    std::cout << "structured bindings: yes\n";
#endif
#ifdef __cpp_lib_filesystem
    std::cout << "filesystem: yes\n";
#endif
#ifdef __cpp_lib_to_chars
    std::cout << "to_chars: yes\n";
#endif
#ifdef __cpp_lib_pmr
    std::cout << "pmr: yes\n";
#endif
}
```

## HFT 应用

```cpp
// 降级策略：编译期检测，选择最优实现
template <typename T>
std::string to_str_fast(T val) {
#ifdef __cpp_lib_to_chars
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf+32, val);
    return std::string(buf, ptr);
#else
    return std::to_string(val);  // 降级
#endif
}
```

## 自测题

1. `__cplusplus` 在 MSVC 上的问题是什么？怎么解决？
2. 特性特征宏的命名规则是什么？（语言 vs 库）
3. 如何用特征宏做条件编译降级？
4. `__cpp_lib_to_chars` 检测的是什么？
5. C++20 的 `<version>` 头有什么用？

<details>
<summary>参考答案</summary>

1. 问题是 **MSVC 默认不更新 `__cplusplus`**：即使开了 `/std:c++17`，它仍可能报告 `199711L`（C++98），导致 `#if __cplusplus >= 201703L` 判断失效。
两种解决：
   1. 编译时加 **`/Zc:__cplusplus`**（VS2017 15.7 起），让 `__cplusplus` 反映真实标准版本；
   2. 用 MSVC 专有宏 **`_MSVC_LANG`** 代替（如 `#if defined(_MSVC_LANG) && _MSVC_LANG >= 201703L`），跨平台代码常写成 `#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)`。
2. 两类前缀区分「语言特性」和「库特性」：
   - **语言特性**：`__cpp_` + 特性名，如 `__cpp_structured_bindings`、`__cpp_if_constexpr`、`__cpp_inline_variables`、`__cpp_fold_expressions`、`__cpp_deduction_guides`。
   - **标准库特性**：`__cpp_lib_` + 特性名，如 `__cpp_lib_filesystem`、`__cpp_lib_to_chars`、`__cpp_lib_parallel_algorithm`、`__cpp_lib_pmr`、`__cpp_lib_optional`。
值通常是 `YYYYMM` 形式的日期（如 `201611L`），表示该特性被采纳的版本；宏被定义即表示支持。
3. 用 `#ifdef` / `#if` 判断宏是否存在，为支持和不支持两种环境各写一份实现：
```cpp
template <typename T>
std::string to_str_fast(T val) {
#ifdef __cpp_lib_to_chars
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + 32, val);
    return std::string(buf, ptr);      // 快路径：无分配、无 locale
#else
    return std::to_string(val);        // 降级路径
#endif
}
```
这样同一份源码在老工具链上仍能编译，在新工具链上自动用上最优实现；也可配合 `#if __cpp_lib_to_chars >= 201611L` 做更细的版本判断。
4. 它检测的是 **`<charconv>` 中的 `std::to_chars` / `std::from_chars` 是否可用**（即 C++17 的低层数值转换设施是否已实现）。
宏被定义（C++17 中值为 `201611L`）就说明可以放心使用这两个函数；未定义则说明标准库尚未提供，需要降级到 `std::to_string` / `strtod` 等方案。注意它反映的是**标准库实现**的支持情况，而不是编译器对 C++17 语言特性的支持。
5. C++20 新增的 `<version>` 头文件**集中定义所有标准库特性测试宏**（`__cpp_lib_*` 等），只要 include 它就能拿到全部宏，不必为了查某个宏而去 include 对应的大头文件。
C++17 时代没有这个头，通常不得不 include `<ciso646>`（一个几乎为空的头）之类来"顺便"拿到版本宏——既不直观也不可靠。有了 `<version>`，做能力检测的代码可以写成 `#include <version>` 加一串 `#ifdef`，轻量且明确。

</details>
